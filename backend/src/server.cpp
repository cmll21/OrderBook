#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <memory>
#include <iostream>
#include "order_book.hpp"
#include "logger.hpp"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    OrderBook &order_book_;
    Logger &logger_;

public:
    WebSocketSession(tcp::socket socket, OrderBook &order_book, Logger &logger)
        : ws_(std::move(socket)), order_book_(order_book), logger_(logger) {}

    void start() {
        ws_.async_accept(
            [self = shared_from_this()](boost::system::error_code ec) {
                self->on_accept(ec);
            });
    }

private:
    void on_accept(boost::system::error_code ec) {
        if (ec) {
            logger_.log("WebSocket accept error: " + ec.message());
            return;
        }
        do_read();
    }

    void do_read() {
        ws_.async_read(buffer_,
            [self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
                self->on_read(ec, bytes_transferred);
            });
    }

    void on_read(boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
        if (ec) {
            logger_.log("WebSocket read error: " + ec.message());
            return;
        }

        std::string request = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());

        json::object response_obj;
        try {
            auto parsed = json::parse(request);
            auto obj = parsed.as_object();

            if(obj.contains("command") && obj["command"].as_string() == "summary") {
                json::array bids;
                for(const auto &level : order_book_.get_bids()) {
                    json::object level_obj;
                    level_obj["price"] = level.price;
                    level_obj["quantity"] = level.quantity;
                    bids.push_back(level_obj);
                }
                json::array asks;
                for(const auto &level : order_book_.get_asks()) {
                    json::object level_obj;
                    level_obj["price"] = level.price;
                    level_obj["quantity"] = level.quantity;
                    asks.push_back(level_obj);
                }
                response_obj["bids"] = bids;
                response_obj["asks"] = asks;
            } else {
                OrderID id = std::stoull(std::string(obj.at("id").as_string()));
                OrderType type = (obj.at("type").as_string() == "GTC")
                                    ? OrderType::good_till_cancel
                                    : OrderType::immediate_or_cancel;
                OrderSide side = (obj.at("side").as_string() == "buy")
                                    ? OrderSide::buy
                                    : OrderSide::sell;
                Price price = static_cast<Price>(obj.at("price").as_int64());
                Quantity quantity = static_cast<Quantity>(obj.at("quantity").as_int64());

                order_book_.add_order(id, type, side, price, quantity);
                response_obj["message"] = "Order received: " + std::to_string(id);
            }
        } catch(const std::exception &e) {
            response_obj["error"] = std::string("Error processing request: ") + e.what();
        }

        std::string response = json::serialize(response_obj);
        ws_.async_write(net::buffer(response),
            [self = shared_from_this()](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
                self->on_write(ec);
            });
    }

    void on_write(boost::system::error_code ec) {
        if (ec) {
            logger_.log("WebSocket write error: " + ec.message());
            return;
        }
        do_read();
    }
};

class WebSocketServer
{
    net::io_context &ioc_;
    tcp::acceptor acceptor_;
    OrderBook order_book_;
    Logger &logger_;

public:
    WebSocketServer(net::io_context &ioc, tcp::endpoint endpoint, Logger &logger)
        : ioc_(ioc), acceptor_(ioc, endpoint), order_book_(&logger), logger_(logger)
    {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<WebSocketSession>(std::move(socket), order_book_, logger_)->start();
                } else {
                    logger_.log("Accept error: " + ec.message());
                }
                do_accept();
            });
    }
};

int main() {
    try {
        net::io_context ioc{1};
        tcp::endpoint endpoint(tcp::v4(), 8080);

        // Create a logger instance (e.g., ConsoleLogger)
        ConsoleLogger logger; // Make sure ConsoleLogger is defined in logger.hpp
        WebSocketServer server(ioc, endpoint, logger);

        logger.log("Async WebSocket server started on port 8080");
        ioc.run();
    } catch (std::exception &e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
    return 0;
}
