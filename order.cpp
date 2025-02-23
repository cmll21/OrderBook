#include "order.hpp"

Order::Order(OrderID id, OrderType type, OrderSide side, Price price, Quantity initial_quantity)
    : id_(id), type_(type), side_(side), price_(price),
      initial_quantity_(initial_quantity), remaining_quantity_(initial_quantity),
      timestamp_(std::chrono::steady_clock::now()), status_(OrderStatus::open) {}

OrderID Order::get_id() const { return id_; }
OrderType Order::get_type() const { return type_; }
OrderSide Order::get_side() const { return side_; }
Price Order::get_price() const { return price_; }
Quantity Order::get_initial_quantity() const { return initial_quantity_; }
Quantity Order::get_remaining_quantity() const { return remaining_quantity_; }
Quantity Order::get_filled_quantity() const { return initial_quantity_ - remaining_quantity_; }
OrderStatus Order::get_status() const { return status_; }
auto Order::get_timestamp() const { return timestamp_; }

void Order::cancel()
{
    if (status_ == OrderStatus::filled)
        throw std::runtime_error("Cannot cancel a filled order");
    status_ = OrderStatus::canceled;
}

void Order::modify(Price new_price, Quantity new_quantity)
{
    if (status_ == OrderStatus::filled || status_ == OrderStatus::canceled)
        throw std::runtime_error("Cannot modify a filled or canceled order");

    if (new_quantity < get_filled_quantity())
        throw std::runtime_error("Cannot reduce quantity below filled quantity");

    price_ = new_price;
    remaining_quantity_ += new_quantity - initial_quantity_;
    initial_quantity_ = new_quantity;

    if (remaining_quantity_ == 0)
        status_ = OrderStatus::filled;
    else if (get_filled_quantity() > 0)
        status_ = OrderStatus::partially_filled;
    else
        status_ = OrderStatus::open;
}

void Order::fill(Quantity quantity)
{
    if (quantity > remaining_quantity_)
        throw std::runtime_error("Cannot fill more than remaining quantity");

    remaining_quantity_ -= quantity;
    status_ = (remaining_quantity_ == 0) ? OrderStatus::filled : OrderStatus::partially_filled;
}
