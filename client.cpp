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

void send_summary_request(tcp::socket &socket)
{
    json::object summary_cmd;
    summary_cmd["command"] = "summary";
    std::string message = json::serialize(summary_cmd) + "\n";
    boost::asio::write(socket, boost::asio::buffer(message));

    // Receive response containing the full order book
    boost::asio::streambuf response_buffer;
    boost::asio::read_until(socket, response_buffer, "\n");

    std::istream response_stream(&response_buffer);
    std::string response;
    std::getline(response_stream, response);

    // Parse the response
    auto parsed = json::parse(response);
    auto obj = parsed.as_object();

    std::cout << "\nOrder Book Summary:\n";

    if (obj.contains("bids")) {
        auto bids = obj["bids"].as_array();
        int total_bid_volume = 0;
        std::cout << "\nBids:\n";
        for (const auto& bid : bids) {
            auto bid_obj = bid.as_object();
            int price = bid_obj["price"].as_int64();
            int quantity = bid_obj["quantity"].as_int64();
            total_bid_volume += quantity;
            std::cout << "Price: " << price << ", Quantity: " << quantity << "\n";
        }
        std::cout << "Total Bid Volume: " << total_bid_volume << "\n";
    }

    if (obj.contains("asks")) {
        auto asks = obj["asks"].as_array();
        int total_ask_volume = 0;
        std::cout << "\nAsks:\n";
        for (const auto& ask : asks) {
            auto ask_obj = ask.as_object();
            int price = ask_obj["price"].as_int64();
            int quantity = ask_obj["quantity"].as_int64();
            total_ask_volume += quantity;
            std::cout << "Price: " << price << ", Quantity: " << quantity << "\n";
        }
        std::cout << "Total Ask Volume: " << total_ask_volume << "\n";
    }
}

void print_usage() {
    std::cout << "\nAvailable commands:\n"
              << "send <type> <side> <price> <quantity> - Send a new order\n"
              << "summary - Request order book summary\n"
              << "quit - Exit the program\n"
              << "\nOrder types: GTC, IOC, FOK\n"
              << "Order sides: buy, sell\n";
}

bool get_order_input(std::string& command, std::string& type, std::string& side, int& price, int& quantity) {
    std::cout << "\nEnter command: ";
    std::cin >> command;
    if (command == "quit") return false;
    if (command == "send")
        std::cin >> type >> side >> price >> quantity;
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
            std::string command, type, side;
            int price = 0, quantity = 0;
            if (!get_order_input(command, type, side, price, quantity))
                break;
            if (command == "send") {
                try {
                    send_order(socket, order_id++, type, side, price, quantity);
                } catch (const std::exception& e) {
                    std::cerr << "Error sending order: " << e.what() << std::endl;
                }
            } else if (command == "summary") {
                try {
                    send_summary_request(socket);
                } catch (const std::exception& e) {
                    std::cerr << "Error requesting summary: " << e.what() << std::endl;
                }
            } else {
                std::cout << "Invalid command\n";
                print_usage();
            }
        }

        socket.close();
    } catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
