#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <iostream>
#include "order.hpp"

using boost::asio::ip::tcp;
namespace json = boost::json;

void send_order(tcp::socket &socket, OrderID id, const std::string &type, const std::string &side, int price, int quantity)
{
    json::object order_msg;
    order_msg["id"] = std::to_string(id);
    order_msg["type"] = type;
    order_msg["side"] = side;
    order_msg["price"] = price;
    order_msg["quantity"] = quantity;

    std::string message = json::serialize(order_msg) + "\n";
    boost::asio::write(socket, boost::asio::buffer(message));

    // Receive response
    boost::asio::streambuf response_buffer;
    boost::asio::read_until(socket, response_buffer, "\n");

    std::istream response_stream(&response_buffer);
    std::string response;
    std::getline(response_stream, response);
    std::cout << "Server response: " << response << std::endl;
}

int main()
{
    try
    {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "8080");

        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);
        std::cout << "Connected to server" << std::endl;

        send_order(socket, 1, "GTC", "buy", 100, 10);
        send_order(socket, 2, "GTC", "sell", 95, 6);

        socket.close();
    }
    catch (std::exception &e)
    {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}
