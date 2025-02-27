#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <sstream>
#include <deque>
#include <thread>
#include <mutex>
#include "order.hpp"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

class AsyncWebSocketClient : public std::enable_shared_from_this<AsyncWebSocketClient>
{
    net::io_context& ioc_;
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::deque<std::string> write_msgs_;
    std::mutex write_mutex_;

public:
    AsyncWebSocketClient(net::io_context &ioc)
        : ioc_(ioc), ws_(ioc) {}

    void run(const std::string &host, const std::string &port, const std::string &target = "/")
    {
        tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host, port);
        net::async_connect(ws_.next_layer(), results,
            [self = shared_from_this(), target](boost::system::error_code ec, auto /*endpoint*/) {
                if (!ec)
                {
                    self->ws_.async_handshake("127.0.0.1", target,
                        [self](boost::system::error_code ec) {
                            if (!ec)
                            {
                                self->do_read();
                            }
                            else
                            {
                                std::cerr << "Handshake error: " << ec.message() << std::endl;
                            }
                        });
                }
                else
                {
                    std::cerr << "Connect error: " << ec.message() << std::endl;
                }
            });
    }

    void send(const std::string &message)
    {
        net::post(ioc_,
            [self = shared_from_this(), message]()
            {
                bool write_in_progress;
                {
                    std::lock_guard<std::mutex> lock(self->write_mutex_);
                    write_in_progress = !self->write_msgs_.empty();
                    self->write_msgs_.push_back(message);
                }
                if(!write_in_progress)
                {
                    self->do_write();
                }
            });
    }

private:
    void do_read()
    {
        ws_.async_read(buffer_,
            [self = shared_from_this()](boost::system::error_code ec, std::size_t /*bytes_transferred*/)
            {
                if(!ec)
                {
                    std::string response = beast::buffers_to_string(self->buffer_.data());
                    self->buffer_.consume(self->buffer_.size());
                    std::cout << "Received: " << response << std::endl;
                    self->do_read();
                }
                else
                {
                    std::cerr << "Read error: " << ec.message() << std::endl;
                }
            });
    }

    void do_write()
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        ws_.async_write(net::buffer(write_msgs_.front()),
            [self = shared_from_this()](boost::system::error_code ec, std::size_t /*bytes_transferred*/)
            {
                if(!ec)
                {
                    std::lock_guard<std::mutex> lock(self->write_mutex_);
                    self->write_msgs_.pop_front();
                    if(!self->write_msgs_.empty())
                    {
                        self->do_write();
                    }
                }
                else
                {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
            });
    }
};

int main()
{
    net::io_context ioc;
    auto client = std::make_shared<AsyncWebSocketClient>(ioc);
    client->run("127.0.0.1", "8080");

    // Run the io_context in a separate thread.
    std::thread io_thread([&ioc]() { ioc.run(); });

    std::cout << "Async WebSocket client. Enter commands:" << std::endl;
    std::string line;
    OrderID order_id = 1;
    while(std::getline(std::cin, line))
    {
        if(line == "quit")
            break;
        else if(line == "summary")
        {
            json::object summary_cmd;
            summary_cmd["command"] = "summary";
            client->send(json::serialize(summary_cmd));
        }
        else if(line.find("send") == 0)
        {
            // Expected format: send <type> <side> <price> <quantity>
            std::istringstream iss(line);
            std::string command, type, side;
            int price, quantity;
            iss >> command >> type >> side >> price >> quantity;
            json::object order_msg;
            order_msg["id"] = std::to_string(order_id++);
            order_msg["type"] = type;
            order_msg["side"] = side;
            order_msg["price"] = price;
            order_msg["quantity"] = quantity;
            client->send(json::serialize(order_msg));
        }
        else
        {
            std::cout << "Unknown command. Use 'send <type> <side> <price> <quantity>', 'summary', or 'quit'." << std::endl;
        }
    }

    // Optionally, send a close message if needed.
    client->send("{\"command\": \"quit\"}");
    ioc.stop();
    io_thread.join();
    return 0;
}
