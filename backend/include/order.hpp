#ifndef ORDER_HPP
#define ORDER_HPP

#include <chrono>
#include <cstdint>
#include <stdexcept>

enum class OrderType
{
    good_till_cancel,
    immediate_or_cancel,
    fill_or_kill
};

enum class OrderSide
{
    buy,
    sell
};

enum class OrderStatus
{
    open,
    partially_filled,
    filled,
    canceled
};

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderID = std::uint64_t;

class Order
{
public:
    Order(OrderID id, OrderType type, OrderSide side, Price price, Quantity initial_quantity);

    OrderID get_id() const;
    OrderType get_type() const;
    OrderSide get_side() const;
    Price get_price() const;
    Quantity get_initial_quantity() const;
    Quantity get_remaining_quantity() const;
    Quantity get_filled_quantity() const;
    OrderStatus get_status() const;
    auto get_timestamp() const;

    void cancel();
    void modify(Price new_price, Quantity new_quantity);
    void fill(Quantity quantity);

private:
    OrderID id_;
    OrderType type_;
    OrderSide side_;
    Price price_;
    Quantity initial_quantity_;
    Quantity remaining_quantity_;
    std::chrono::steady_clock::time_point timestamp_;
    OrderStatus status_;
};

#endif // ORDER_HPP
