#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>

namespace beast   = boost::beast;           // Boost.Beast
namespace websocket = beast::websocket;      // WebSocket
namespace net     = boost::asio;             // Boost.Asio
namespace json    = boost::json;             // Boost.JSON
using tcp         = net::ip::tcp;

int main() {
    try {
        // Create an io_context
        net::io_context ioc;

        // Resolve the server address.
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve("127.0.0.1", "8080");

        // Create the WebSocket stream and connect
        websocket::stream<tcp::socket> ws(ioc);
        net::connect(ws.next_layer(), results.begin(), results.end());
        ws.handshake("127.0.0.1", "/");
        std::cout << "Connected to WebSocket server." << std::endl;

        // Setup random generator for simulated orders
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> priceDist(90, 110);    // Price between 90 and 110
        std::uniform_int_distribution<int> quantityDist(1, 10);     // Quantity between 1 and 10
        std::uniform_int_distribution<int> typeDist(0, 1);          // 0: GTC, 1: IOC
        std::uniform_int_distribution<int> sideDist(0, 1);          // 0: buy, 1: sell

        const int numOrders = 1000; // Number of simulated orders

        // Loop to send simulated orders
        for (int i = 0; i < numOrders; i++) {
            int price    = priceDist(gen);
            int quantity = quantityDist(gen);
            std::string orderType = (typeDist(gen) == 0) ? "GTC" : "IOC";
            std::string side = (sideDist(gen) == 0) ? "buy" : "sell";

            // Construct the order JSON
            json::object orderMsg;
            orderMsg["id"] = std::to_string(i + 1);
            orderMsg["type"] = orderType;
            orderMsg["side"] = side;
            orderMsg["price"] = price;
            orderMsg["quantity"] = quantity;
            std::string message = json::serialize(orderMsg);

            // Send the order over the WebSocket
            ws.write(net::buffer(message));

            // Read the server response
            beast::flat_buffer buffer;
            ws.read(buffer);
            std::string response = beast::buffers_to_string(buffer.data());
            std::cout << "Order " << (i + 1) << " response: " << response << std::endl;

            // Pause briefly between orders
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Request a summary of the order book
        json::object summaryCmd;
        summaryCmd["command"] = "summary";
        std::string summaryMsg = json::serialize(summaryCmd);
        ws.write(net::buffer(summaryMsg));

        beast::flat_buffer summaryBuffer;
        ws.read(summaryBuffer);
        std::string summaryResponse = beast::buffers_to_string(summaryBuffer.data());
        std::cout << "Order Book Summary: " << summaryResponse << std::endl;

        // Close the WebSocket connection normally.
        ws.close(websocket::close_code::normal);
    }
    catch (const std::exception &e) {
        std::cerr << "Tester error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
