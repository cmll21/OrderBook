/*
TO DO:
- Testing
- Fix edge cases
- Add order modification

Eventually:
- Add client/server communication
- Add order book persistence
- Add order book visualisation
- Parallelise matching
*/

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class Logger
{
public:
    virtual ~Logger() = default;
    virtual void log(const std::string &msg) = 0;
};

class ConsoleLogger : public Logger
{
public:
    void log(const std::string &msg) override
    {
        std::cout << "[LOG] " << msg << std::endl;
    }
};

class NullLogger : public Logger
{
public:
    void log(const std::string &msg) override
    {
    }
};

static Logger &get_default_logger()
{
    static NullLogger default_logger;
    return default_logger;
}

// Enums with snake_case values.
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

struct TradeInfo
{
    OrderID order_id;
    Price price;
    Quantity quantity;
};

class Trade
{
public:
    Trade(const TradeInfo &bid_trade, const TradeInfo &ask_trade)
        : bid_trade_(bid_trade), ask_trade_(ask_trade) {}

    const TradeInfo &get_bid_trade() const { return bid_trade_; }
    const TradeInfo &get_ask_trade() const { return ask_trade_; }

private:
    TradeInfo bid_trade_;
    TradeInfo ask_trade_;
};

using Trades = std::vector<Trade>;

class Order
{
public:
    Order(OrderID id, OrderType type, OrderSide side, Price price, Quantity initial_quantity)
        : id_(id), type_(type), side_(side), price_(price),
          initial_quantity_(initial_quantity), remaining_quantity_(initial_quantity),
          timestamp_(std::chrono::steady_clock::now()), status_(OrderStatus::open) {}

    OrderID get_id() const { return id_; }
    OrderType get_type() const { return type_; }
    OrderSide get_side() const { return side_; }
    Price get_price() const { return price_; }
    Quantity get_initial_quantity() const { return initial_quantity_; }
    Quantity get_remaining_quantity() const { return remaining_quantity_; }
    Quantity get_filled_quantity() const { return initial_quantity_ - remaining_quantity_; }
    OrderStatus get_status() const { return status_; }
    auto get_timestamp() const { return timestamp_; }

    void cancel()
    {
        if (status_ == OrderStatus::filled)
            throw std::runtime_error("Cannot cancel a filled order");
        status_ = OrderStatus::canceled;
    }
    void modify(Price new_price, Quantity new_quantity)
    {
        if (status_ == OrderStatus::filled)
        {
            throw std::runtime_error("Cannot modify a filled order");
        }
        else if (status_ == OrderStatus::canceled)
        {
            throw std::runtime_error("Cannot modify a canceled order");
        }
        if (new_quantity < get_filled_quantity())
        {
            throw std::runtime_error("Cannot reduce quantity below filled quantity");
        }
        price_ = new_price;
        remaining_quantity_ += new_quantity - initial_quantity_;
        initial_quantity_ = new_quantity;

        if (remaining_quantity_ == 0)
        {
            status_ = OrderStatus::filled;
        }
        else if (get_filled_quantity() > 0)
        {
            status_ = OrderStatus::partially_filled;
        }
        else
        {
            status_ = OrderStatus::open;
        }
    }
    void fill(Quantity quantity)
    {
        if (quantity > remaining_quantity_)
            throw std::runtime_error("Cannot fill more than remaining quantity");

        Quantity old_remaining = remaining_quantity_;
        OrderStatus old_status = status_;
        try
        {
            remaining_quantity_ -= quantity;
            status_ = (remaining_quantity_ == 0) ? OrderStatus::filled : OrderStatus::partially_filled;
        }
        catch (...)
        {
            remaining_quantity_ = old_remaining;
            status_ = old_status;
            throw;
        }
    }

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

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::deque<OrderPointer>;

// MatchingEngine handles the matching logic and logs key events.
class MatchingEngine
{
public:
    MatchingEngine(std::unordered_map<OrderID, OrderPointer> &order_lookup,
                   Trades &trade_history,
                   std::function<void(OrderID)> cancel_func,
                   Logger &logger)
        : order_lookup_(order_lookup), trade_history_(trade_history),
          cancel_order_(cancel_func), logger_(logger) {}

    template <typename OppositeMap>
    void match_order(OrderPointer aggressive_order, OppositeMap &opposite_book)
    {
        if (aggressive_order->get_type() == OrderType::fill_or_kill &&
            !has_sufficient_liquidity(aggressive_order, opposite_book))
        {
            logger_.log("Insufficient liquidity for fill_or_kill order " +
                        std::to_string(aggressive_order->get_id()));
            cancel_order_(aggressive_order->get_id());
            return;
        }

        while (!opposite_book.empty() && aggressive_order->get_remaining_quantity() > 0)
        {
            auto best_it = opposite_book.begin();
            Price best_price = best_it->first;
            if (!is_price_acceptable(aggressive_order, best_price))
            {
                if (aggressive_order->get_type() == OrderType::immediate_or_cancel ||
                    aggressive_order->get_type() == OrderType::fill_or_kill)
                {
                    logger_.log("Price not acceptable for order " +
                                std::to_string(aggressive_order->get_id()));
                    cancel_order_(aggressive_order->get_id());
                }
                break;
            }
            auto &order_list = best_it->second;
            process_price_level(aggressive_order, order_list);
            if (order_list.empty())
                opposite_book.erase(best_it);
        }

        if (aggressive_order->get_type() == OrderType::immediate_or_cancel &&
            aggressive_order->get_remaining_quantity() > 0)
        {
            aggressive_order->cancel();
            logger_.log("ImmediateOrCancel order " +
                        std::to_string(aggressive_order->get_id()) +
                        " canceled due to remaining quantity");
            cancel_order_(aggressive_order->get_id());
        }
    }

private:
    template <typename OppositeMap>
    bool has_sufficient_liquidity(OrderPointer aggressive_order, const OppositeMap &opposite_book) const
    {
        Quantity available = get_available_quantity(aggressive_order, opposite_book);
        return available >= aggressive_order->get_remaining_quantity();
    }

    bool is_price_acceptable(OrderPointer aggressive_order, Price best_price) const
    {
        return (aggressive_order->get_side() == OrderSide::buy)
                   ? (aggressive_order->get_price() >= best_price)
                   : (aggressive_order->get_price() <= best_price);
    }

    void process_price_level(OrderPointer aggressive_order, OrderPointers &order_list)
    {
        while (!order_list.empty() && aggressive_order->get_remaining_quantity() > 0)
        {
            OrderPointer resting_order = order_list.front();
            execute_trade(aggressive_order, resting_order);
            if (resting_order->get_remaining_quantity() == 0)
            {
                order_list.pop_front();
                order_lookup_.erase(resting_order->get_id());
            }
        }
    }

    template <typename OppositeMap>
    Quantity get_available_quantity(OrderPointer aggressive_order, const OppositeMap &opposite_book) const
    {
        Quantity total = 0;
        for (auto it = opposite_book.begin(); it != opposite_book.end(); ++it)
        {
            Price level_price = it->first;
            bool level_matches = (aggressive_order->get_side() == OrderSide::buy)
                                     ? (aggressive_order->get_price() >= level_price)
                                     : (aggressive_order->get_price() <= level_price);
            if (!level_matches)
                break;
            for (const auto &order : it->second)
            {
                total += order->get_remaining_quantity();
                if (total >= aggressive_order->get_remaining_quantity())
                    return total;
            }
        }
        return total;
    }

    void execute_trade(OrderPointer aggressive_order, OrderPointer resting_order)
    {
        Quantity trade_quantity = std::min(aggressive_order->get_remaining_quantity(),
                                           resting_order->get_remaining_quantity());
        aggressive_order->fill(trade_quantity);
        resting_order->fill(trade_quantity);

        Price execution_price = resting_order->get_price();
        TradeInfo bid_trade, ask_trade;
        if (aggressive_order->get_side() == OrderSide::buy)
        {
            bid_trade = {aggressive_order->get_id(), execution_price, trade_quantity};
            ask_trade = {resting_order->get_id(), execution_price, trade_quantity};
        }
        else
        {
            bid_trade = {resting_order->get_id(), execution_price, trade_quantity};
            ask_trade = {aggressive_order->get_id(), execution_price, trade_quantity};
        }
        trade_history_.push_back(Trade(bid_trade, ask_trade));
        logger_.log("Trade executed between orders " +
                    std::to_string(aggressive_order->get_id()) + " and " +
                    std::to_string(resting_order->get_id()));
    }

    std::unordered_map<OrderID, OrderPointer> &order_lookup_;
    Trades &trade_history_;
    std::function<void(OrderID)> cancel_order_;
    Logger &logger_;
};

// OrderBook handles order placement and cancellation.
class OrderBook
{
public:
    // In the constructor, if no logger is provided, we use a NullLogger.
    OrderBook(Logger *logger = nullptr)
        : logger_(logger ? *logger : get_default_logger()) {}

    const Trades &get_trade_history() const { return trade_history_; }

    OrderPointer add_order(OrderID id, OrderType type, OrderSide side, Price price, Quantity quantity)
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
                asks_[price].push_back(order);
        }
        return order;
    }

    void cancel_order(OrderID id)
    {
        OrderPointer order = find_order(id);
        if (order->get_status() == OrderStatus::filled)
            throw std::runtime_error("Cannot cancel a filled order");

        cancel_order_impl(order);
    }

    // Remove an order without canceling it.
    void remove_order(OrderID id)
    {
        OrderPointer order = find_order(id);
        remove_order_impl(order);
    }

    void modify_order(OrderID id, Price new_price, Quantity new_total_quantity)
    {
        OrderPointer order = find_order(id);
        if (order->get_status() == OrderStatus::filled || order->get_status() == OrderStatus::canceled)
            throw std::runtime_error("Cannot modify a filled or canceled order");

        // Remove order from its current container.
        remove_order(id);

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

private:
    OrderPointer find_order(OrderID id)
    {
        auto it = order_lookup_.find(id);
        if (it == order_lookup_.end())
            throw std::runtime_error("Order not found (find_order)");
        return it->second;
    }

    void cancel_order_impl(OrderPointer order)
    {
        Price price = order->get_price();
        if (order->get_side() == OrderSide::buy)
        {
            auto &order_list = bids_[price];
            std::erase_if(order_list, [&](const OrderPointer &o)
                          { return o == order; });
            if (order_list.empty())
                bids_.erase(price);
        }
        else
        {
            auto &order_list = asks_[price];
            std::erase_if(order_list, [&](const OrderPointer &o)
                          { return o == order; });
            if (order_list.empty())
                asks_.erase(price);
        }
        order->cancel();
        order_lookup_.erase(order->get_id());
        logger_.log("Canceled order " + std::to_string(order->get_id()));
    }

    void remove_order_impl(OrderPointer order)
    {
        Price price = order->get_price();
        if (order->get_side() == OrderSide::buy)
        {
            auto &order_list = bids_[price];
            std::erase_if(order_list, [&](const OrderPointer &o)
                          { return o == order; });
            if (order_list.empty())
                bids_.erase(price);
        }
        else
        {
            auto &order_list = asks_[price];
            std::erase_if(order_list, [&](const OrderPointer &o)
                          { return o == order; });
            if (order_list.empty())
                asks_.erase(price);
        }
        // logger_.log("Removed order " + std::to_string(order->get_id()));
    }

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderID, OrderPointer> order_lookup_;
    Trades trade_history_;
    Logger &logger_;
};

void print_trade_history(const Trades &trades)
{
    for (const auto &trade : trades)
    {
        const auto &bid_trade = trade.get_bid_trade();
        const auto &ask_trade = trade.get_ask_trade();
        std::cout << "Trade: Bid Order " << bid_trade.order_id
                  << " and Ask Order " << ask_trade.order_id
                  << " at Price " << bid_trade.price
                  << " for Quantity " << bid_trade.quantity << "\n";
    }
}

int main()
{

    ConsoleLogger console_logger;

    OrderBook ob(&console_logger);
    ob.add_order(6, OrderType::good_till_cancel, OrderSide::buy, 100, 10);
    ob.add_order(7, OrderType::good_till_cancel, OrderSide::sell, 95, 6);
    print_trade_history(ob.get_trade_history());
    // Valid modification: new_total_quantity = 8 (>= filled amount 6)
    ob.modify_order(6, 105, 8);
    try
    {
        // Invalid modification: new_total_quantity = 5 (< filled amount 6)
        ob.modify_order(6, 105, 5);
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }
    print_trade_history(ob.get_trade_history());
}
