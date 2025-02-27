#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <string>
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

void print_usage() {
    std::cout << "\nAvailable commands:\n"
              << "send <type> <side> <price> <quantity> - Send a new order\n"
              << "quit - Exit the program\n"
              << "\nOrder types: GTC, IOC, FOK\n"
              << "Order sides: buy, sell\n";
}

bool get_order_input(std::string& type, std::string& side, int& price, int& quantity) {
    std::string command;
    std::cout << "\nEnter command: ";
    std::cin >> command;

    if (command == "quit") return false;
    if (command != "send") {
        std::cout << "Invalid command\n";
        print_usage();
        return true;
    }

    std::cin >> type >> side >> price >> quantity;

    // Validate input
    bool valid = true;
    if (type != "GTC" && type != "IOC" && type != "FOK") {
        std::cout << "Invalid order type\n";
        valid = false;
    }
    if (side != "buy" && side != "sell") {
        std::cout << "Invalid order side\n";
        valid = false;
    }
    if (price <= 0 || quantity <= 0) {
        std::cout << "Price and quantity must be positive\n";
        valid = false;
    }

    if (!valid) print_usage();
    return true;
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve("127.0.0.1", "8080");

        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);
        std::cout << "Connected to server" << std::endl;

        print_usage();
        OrderID order_id = 1;

        while (true) {
            std::string type, side;
            int price, quantity;

            if (!get_order_input(type, side, price, quantity)) break;

            try {
                send_order(socket, order_id++, type, side, price, quantity);
            } catch (const std::exception& e) {
                std::cerr << "Error sending order: " << e.what() << std::endl;
            }
        }

        socket.close();
    } catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
