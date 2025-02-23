#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <unordered_map>
#include <memory>
#include "order_book.hpp"

using boost::asio::ip::tcp;
namespace json = boost::json;

class OrderServer
{
public:
    OrderServer(boost::asio::io_context &io_context, int port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          socket_(io_context),
          order_book_(&console_logger_)
    {
        start_accept();
    }

private:
    void start_accept()
    {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
                               {
            if (!ec) {
                std::make_shared<OrderSession>(std::move(socket_), order_book_)->start();
            }
            start_accept(); });
    }

    class OrderSession : public std::enable_shared_from_this<OrderSession>
    {
    public:
        OrderSession(tcp::socket socket, OrderBook &order_book)
            : socket_(std::move(socket)), order_book_(order_book) {}

        void start()
        {
            read_request();
        }

    private:
        void read_request()
        {
            auto self(shared_from_this());
            boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(buffer_), "\n",
                                          [this, self](boost::system::error_code ec, std::size_t length)
                                          {
                if (!ec) {
                    process_request(buffer_.substr(0, length));
                    buffer_.erase(0, length);
                    read_request();
                } else {
                    socket_.close();
                } });
        }

        void process_request(const std::string &request)
        {
            try
            {
                auto parsed = json::parse(request);
                OrderID id = std::stoull(std::string(parsed.at("id").as_string()));
                OrderType type = (parsed.at("type").as_string() == "GTC") ? OrderType::good_till_cancel : OrderType::immediate_or_cancel;
                OrderSide side = (parsed.at("side").as_string() == "buy") ? OrderSide::buy : OrderSide::sell;
                Price price = static_cast<Price>(parsed.at("price").as_int64());
                Quantity quantity = static_cast<Quantity>(parsed.at("quantity").as_int64());

                order_book_.add_order(id, type, side, price, quantity);
                send_response("Order received: " + std::to_string(id));
            }
            catch (const std::exception &e)
            {
                send_response(std::string("Error processing order: ") + e.what());
            }
        }

        void send_response(const std::string &message)
        {
            auto self(shared_from_this());
            std::string msg = message + "\n";
            boost::asio::async_write(socket_, boost::asio::buffer(msg),
                                     [this, self](boost::system::error_code, std::size_t) {});
        }

        tcp::socket socket_;
        std::string buffer_;
        OrderBook &order_book_;
    };

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    ConsoleLogger console_logger_;
    OrderBook order_book_;
};

int main()
{
    try
    {
        boost::asio::io_context io_context;
        OrderServer server(io_context, 8080);
        std::cout << "Server started on port 8080" << std::endl;
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}
