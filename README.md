# Order Book Project

This project implements a real-time order book with a C++ backend (server, tester, and client) and a React frontend. The system supports order matching, live updates, graphical visualizations (depth charts and time-series charts), and various metrics calculations (spread, mid-price, volatility, etc.).

## Components

- **C++ Server:**  
  Uses Boost.Asio and Boost.Beast to handle WebSocket connections and processes orders using an order matching engine.

- **Tester:**  
  A standalone C++ program that simulates trades by sending randomized orders to the server for testing purposes.

- **React Client:**  
  A web-based UI built with React and Chart.js (via react-chartjs-2) that displays the order book, including:
    - Depth charts (bids and asks with different colors).
    - A live time-series chart (tracking mid prices).
    - Key order book metrics (spread, mid-price, volumes, volatility).

## Prerequisites

### C++ Backend

- C++20 compatible compiler (g++/clang++)
- [Boost Libraries](https://www.boost.org) (Asio, Beast, JSON)
- Make (or CMake) for building

### React Frontend

- [Node.js](https://nodejs.org/) and npm (or yarn)
- Create React App (or your preferred React setup)

## Build and Run

### C++ Server and Tester

A sample `Makefile` is provided. To build all executables, run:

```bash
make
```
This will compile:
- ```order_server``` – the C++ WebSocket server.
- ```order_client``` – a C++ client.
- ```tester_app``` – the trade simulator that connects to the server and performs simulated trades.

### React Client
1. Navigate to directory:
```bash
cd orderbook-app
```
2. Install dependencies:
```bash
npm install
```
3. Start development server:
```bash
npm start
```
The app will typically open at ```http://localhost:3000```.

## Project Structure
```php
OrderBookProject/
├── backend/
│   ├── include/          # Header files
│   │   ├── logger.hpp
│   │   ├── matching_engine.hpp
│   │   ├── order.hpp
│   │   ├── order_book.hpp
│   │   └── trade.hpp
│   ├── src/              # Source files
│   │   ├── client.cpp
│   │   ├── logger.cpp      // if applicable
│   │   ├── matching_engine.cpp
│   │   ├── order.cpp
│   │   ├── order_book.cpp
│   │   ├── server.cpp
│   │   └── tester.cpp
│   └── Makefile          # Backend build file
├── frontend/
│   ├── public/           # Public assets (index.html, favicon.ico, manifest.json, etc.)
│   └── src/              # React source code
│       ├── OrderBookClient.jsx
│       ├── OrderBookTimeSeriesChart.jsx
│       └── ... (other components)
└── README.md             # Project documentation
```
## Order Book Metrics
The React client computes and displays several key metrics:
- **Spread**: Difference between the best ask and best bid.
- **Mid Price**: Average of the best bid and best ask.
- **Total Volume**: Sum of quantities for bids and asks.
- **Volatility**: Standard deviation of mid prices over time.
  
## License
This project is licensed under the MIT License.

