#ifndef ORDER_BOOK_HPP
#define ORDER_BOOK_HPP

#include "order.hpp"
#include "trade.hpp"
#include "matching_engine.hpp"
#include "logger.hpp"
#include <map>
#include <unordered_map>
#include <memory>

class OrderBook
{
public:
    OrderBook(Logger *logger = nullptr);
    const Trades &get_trade_history() const;
    OrderPointer add_order(OrderID id, OrderType type, OrderSide side, Price price, Quantity quantity);
    void cancel_order(OrderID id);
    void modify_order(OrderID id, Price new_price, Quantity new_total_quantity);

private:
    OrderPointer find_order(OrderID id);
    void cancel_order_impl(OrderPointer order);
    void remove_order_impl(OrderPointer order);

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderID, OrderPointer> order_lookup_;
    Trades trade_history_;
    Logger &logger_;
};

#endif // ORDER_BOOK_HPP
