#include "order_book.hpp"

OrderBook::OrderBook(Logger *logger) : logger_(logger ? *logger : get_default_logger()) {}

const Trades &OrderBook::get_trade_history() const { return trade_history_; }

OrderLevels OrderBook::get_bids() const {
    OrderLevels levels;
    for (const auto& [price, orders] : bids_) {
        if (!orders.empty()) {
            Quantity total = 0;
            for (const auto& order : orders) {
                total += order->get_remaining_quantity();
            }
            levels.push_back({price, total});
        }
    }
    return levels;
}

OrderLevels OrderBook::get_asks() const {
    OrderLevels levels;
    for (const auto& [price, orders] : asks_) {
        if (!orders.empty()) {
            Quantity total = 0;
            for (const auto& order : orders) {
                total += order->get_remaining_quantity();
            }
            levels.push_back({price, total});
        }
    }
    return levels;
}

OrderPointer OrderBook::add_order(OrderID id, OrderType type, OrderSide side, Price price, Quantity quantity)
{
    OrderPointer order = std::make_shared<Order>(id, type, side, price, quantity);
    order_lookup_[id] = order;
    logger_.log("Added order " + std::to_string(id));

    auto cancel_lambda = [this](OrderID order_id)
    { this->cancel_order(order_id); };
    MatchingEngine matching_engine(order_lookup_, trade_history_, cancel_lambda, logger_);

    if (side == OrderSide::buy)
    {
        matching_engine.match_order(order, asks_);
        if (order->get_remaining_quantity() > 0)
            bids_[price].push_back(order);
    }
    else
    {
        matching_engine.match_order(order, bids_);
        if (order->get_remaining_quantity() > 0)
            asks_[order->get_price()].push_back(order);
    }
    return order;
}

void OrderBook::cancel_order(OrderID id)
{
    OrderPointer order = find_order(id);
    if (order->get_status() == OrderStatus::filled)
        throw std::runtime_error("Cannot cancel a filled order");

    cancel_order_impl(order);
}

void OrderBook::modify_order(OrderID id, Price new_price, Quantity new_total_quantity)
{
    OrderPointer order = find_order(id);
    if (order->get_status() == OrderStatus::filled || order->get_status() == OrderStatus::canceled)
        throw std::runtime_error("Cannot modify a filled or canceled order");

    // Remove order from its current container.
    remove_order_impl(order);

    // Modify the order.
    order->modify(new_price, new_total_quantity);

    logger_.log("Modified order " + std::to_string(id) +
                " to new price " + std::to_string(new_price) +
                " and new total quantity " + std::to_string(new_total_quantity));

    // If modification makes the order fully filled, remove from lookup and log.
    if (order->get_status() == OrderStatus::filled)
    {
        order_lookup_.erase(id);
        logger_.log("Order " + std::to_string(id) + " fully filled after modification.");
        return;
    }

    // Attempt to re-match the modified order against the opposite book.
    auto cancel_lambda = [this](OrderID order_id)
    { this->cancel_order(order_id); };
    MatchingEngine matching_engine(order_lookup_, trade_history_, cancel_lambda, logger_);

    if (order->get_side() == OrderSide::buy)
    {
        matching_engine.match_order(order, asks_);
        if (order->get_remaining_quantity() > 0)
            bids_[order->get_price()].push_back(order);
    }
    else
    {
        matching_engine.match_order(order, bids_);
        if (order->get_remaining_quantity() > 0)
            asks_[order->get_price()].push_back(order);
    }
}

OrderPointer OrderBook::find_order(OrderID id)
{
    auto it = order_lookup_.find(id);
    if (it == order_lookup_.end())
        throw std::runtime_error("Order not found (find_order)");
    return it->second;
}

// Cancel an order and remove it from the order book
void OrderBook::cancel_order_impl(OrderPointer order)
{
    Price price = order->get_price();
    if (order->get_side() == OrderSide::buy)
    {
        auto &order_list = bids_[price];
        order_list.erase(std::remove(order_list.begin(), order_list.end(), order), order_list.end());
        if (order_list.empty())
            bids_.erase(price);
    }
    else
    {
        auto &order_list = asks_[price];
        order_list.erase(std::remove(order_list.begin(), order_list.end(), order), order_list.end());
        if (order_list.empty())
            asks_.erase(price);
    }
    order->cancel();
    order_lookup_.erase(order->get_id());
    logger_.log("Canceled order " + std::to_string(order->get_id()));
}

// Remove an order without canceling it (for modification)
void OrderBook::remove_order_impl(OrderPointer order)
{
    Price price = order->get_price();
    if (order->get_side() == OrderSide::buy)
    {
        auto &order_list = bids_[price];
        order_list.erase(std::remove(order_list.begin(), order_list.end(), order), order_list.end());
        if (order_list.empty())
            bids_.erase(price);
    }
    else
    {
        auto &order_list = asks_[price];
        order_list.erase(std::remove(order_list.begin(), order_list.end(), order), order_list.end());
        if (order_list.empty())
            asks_.erase(price);
    }
}