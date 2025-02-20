#include <iostream>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <format>

enum class OrderType
{
    GoodTillCancel,
    ImmediateOrCancel,
    FillOrKill
};
enum class OrderSide
{
    Buy,
    Sell
};
enum class OrderStatus
{
    Open,
    Filled,
    Canceled
};

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderID = std::uint64_t;

struct LevelInfo
{
    Price price;
    Quantity quantity;
};

using LevelInfos = std::vector<LevelInfo>;

class Order
{
public:
    Order(OrderID id, OrderType type, OrderSide side, Price price, Quantity initial_quantity)
        : id_(id), type_(type), side_(side), price_(price), initial_quantity_(initial_quantity),
          remaining_quantity_(initial_quantity), status_(OrderStatus::Open) {}

    OrderID get_id() const { return id_; }
    OrderType get_type() const { return type_; }
    OrderSide get_side() const { return side_; }
    Price get_price() const { return price_; }
    Quantity get_initial_quantity() const { return initial_quantity_; }
    Quantity get_remaining_quantity() const { return remaining_quantity_; }
    Quantity get_filled_quantity() const { return initial_quantity_ - remaining_quantity_; }
    OrderStatus get_status() const { return status_; }

    void set_status(OrderStatus status) { status_ = status; }

    void fill_order(Quantity quantity)
    {
        if (quantity > remaining_quantity_)
        {
            throw std::runtime_error("Cannot fill more than remaining quantity");
        }
        remaining_quantity_ -= quantity;
        if (remaining_quantity_ == 0)
        {
            status_ = OrderStatus::Filled;
        }
    }

private:
    OrderID id_;
    OrderType type_;
    OrderSide side_;
    Price price_;
    Quantity initial_quantity_;
    Quantity remaining_quantity_;
    OrderStatus status_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::deque<OrderPointer>;

struct TradeInfo
{
    OrderID order_id;
    Price price;
    Quantity quantity;
};

class Trade
{
public:
    Trade(const TradeInfo &bid_trade, const TradeInfo &ask_trade) : bid_trade_(bid_trade), ask_trade_(ask_trade) {}

    const TradeInfo &get_bid_trade() const { return bid_trade_; }
    const TradeInfo &get_ask_trade() const { return ask_trade_; }

private:
    TradeInfo bid_trade_;
    TradeInfo ask_trade_;
};

using Trades = std::vector<Trade>;

class OrderBook
{
public:
    OrderBook() {}

    OrderPointer add_order(OrderID id, OrderType type, OrderSide side, Price price, Quantity quantity)
    {
        OrderPointer order = std::make_shared<Order>(id, type, side, price, quantity);
        order_lookup_[id] = order;

        if (side == OrderSide::Buy)
        {
            bids_[price].push_back(order);
        }
        else
        {
            asks_[price].push_back(order);
        }

        return order;
    }

    void cancel_order(OrderID id)
    {
        auto it = order_lookup_.find(id);
        if (it == order_lookup_.end())
        {
            throw std::runtime_error("Order not found");
        }

        OrderPointer order = it->second;
        if (order->get_status() == OrderStatus::Filled)
        {
            throw std::runtime_error("Cannot cancel a filled order");
        }

        if (order->get_side() == OrderSide::Buy)
        {
            auto &orders = bids_[order->get_price()];
            orders.erase(std::remove(orders.begin(), orders.end(), order), orders.end());
            if (orders.empty())
            {
                bids_.erase(order->get_price());
            }
        }
        else
        {
            auto &orders = asks_[order->get_price()];
            orders.erase(std::remove(orders.begin(), orders.end(), order), orders.end());
            if (orders.empty())
            {
                asks_.erase(order->get_price());
            }
        }

        order->set_status(OrderStatus::Canceled);
        order_lookup_.erase(it);
    }

    void match()
    {
        while (!bids_.empty() && !asks_.empty())
        {
            auto best_bid = bids_.begin();
            auto best_ask = asks_.begin();
            if (best_bid->first < best_ask->first)
            {
                break;
            }

            auto &bid_orders = best_bid->second;
            auto &ask_orders = best_ask->second;
            auto bid_order = bid_orders.front();
            auto ask_order = ask_orders.front();
            auto trade_quantity = std::min(bid_order->get_remaining_quantity(), ask_order->get_remaining_quantity());

            bid_order->fill_order(trade_quantity);
            ask_order->fill_order(trade_quantity);

            TradeInfo bid_trade{bid_order->get_id(), best_bid->first, trade_quantity};
            TradeInfo ask_trade{ask_order->get_id(), best_ask->first, trade_quantity};
            trade_history_.push_back(Trade{bid_trade, ask_trade});

            if (bid_order->get_remaining_quantity() == 0)
            {
                bid_orders.pop_front();
                if (bid_orders.empty())
                {
                    bids_.erase(best_bid);
                }
            }
            if (ask_order->get_remaining_quantity() == 0)
            {
                ask_orders.pop_front();
                if (ask_orders.empty())
                {
                    asks_.erase(best_ask);
                }
            }
        }
    }

    size_t get_bid_count() const { return bids_.size(); }
    size_t get_ask_count() const { return asks_.size(); }

private:
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderID, OrderPointer> order_lookup_;
    Trades trade_history_;
};

int main()
{
    OrderBook order_book;
    order_book.add_order(1, OrderType::GoodTillCancel, OrderSide::Buy, 100, 10);
    order_book.add_order(2, OrderType::GoodTillCancel, OrderSide::Sell, 100, 10);

    std::cout << std::format("OrderBook: bids={}, asks={}\n", order_book.get_bid_count(), order_book.get_ask_count());

    order_book.match();

    std::cout << std::format("OrderBook: bids={}, asks={}\n", order_book.get_bid_count(), order_book.get_ask_count());

    return 0;
}
