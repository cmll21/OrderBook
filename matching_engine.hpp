#ifndef MATCHING_ENGINE_HPP
#define MATCHING_ENGINE_HPP

#include "order.hpp"
#include "trade.hpp"
#include "logger.hpp"
#include <unordered_map>
#include <deque>
#include <functional>
#include <map>

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::deque<OrderPointer>;

class MatchingEngine
{
public:
    MatchingEngine(std::unordered_map<OrderID, OrderPointer> &order_lookup,
                   Trades &trade_history,
                   std::function<void(OrderID)> cancel_func,
                   Logger &logger);

    template <typename OppositeMap>
    void match_order(OrderPointer aggressive_order, OppositeMap &opposite_book);

private:
    template <typename OppositeMap>
    bool has_sufficient_liquidity(OrderPointer aggressive_order, const OppositeMap &opposite_book) const;

    bool is_price_acceptable(OrderPointer aggressive_order, Price best_price) const;
    void process_price_level(OrderPointer aggressive_order, OrderPointers &order_list);

    template <typename OppositeMap>
    Quantity get_available_quantity(OrderPointer aggressive_order, const OppositeMap &opposite_book) const;

    void execute_trade(OrderPointer aggressive_order, OrderPointer resting_order);

    std::unordered_map<OrderID, OrderPointer> &order_lookup_;
    Trades &trade_history_;
    std::function<void(OrderID)> cancel_order_;
    Logger &logger_;
};

// === IMPLEMENTATION OF TEMPLATE FUNCTIONS ===

template <typename OppositeMap>
void MatchingEngine::match_order(OrderPointer aggressive_order, OppositeMap &opposite_book)
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

template <typename OppositeMap>
bool MatchingEngine::has_sufficient_liquidity(OrderPointer aggressive_order, const OppositeMap &opposite_book) const
{
    Quantity available = get_available_quantity(aggressive_order, opposite_book);
    return available >= aggressive_order->get_remaining_quantity();
}

template <typename OppositeMap>
Quantity MatchingEngine::get_available_quantity(OrderPointer aggressive_order, const OppositeMap &opposite_book) const
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

#endif // MATCHING_ENGINE_HPP
