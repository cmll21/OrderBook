#include "matching_engine.hpp"

// Constructor
MatchingEngine::MatchingEngine(std::unordered_map<OrderID, OrderPointer> &order_lookup,
                               Trades &trade_history,
                               std::function<void(OrderID)> cancel_func,
                               Logger &logger)
    : order_lookup_(order_lookup), trade_history_(trade_history),
      cancel_order_(cancel_func), logger_(logger) {}

// Executes a trade between an aggressive order and a resting order.
void MatchingEngine::execute_trade(OrderPointer aggressive_order, OrderPointer resting_order)
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

    trade_history_.emplace_back(bid_trade, ask_trade);
    logger_.log("Trade executed between orders " +
                std::to_string(aggressive_order->get_id()) + " and " +
                std::to_string(resting_order->get_id()));
}

// Checks if the price of an aggressive order is acceptable for trade execution
bool MatchingEngine::is_price_acceptable(OrderPointer aggressive_order, Price best_price) const
{
    return (aggressive_order->get_side() == OrderSide::buy)
               ? (aggressive_order->get_price() >= best_price)
               : (aggressive_order->get_price() <= best_price);
}

// Processes trades at a specific price level
void MatchingEngine::process_price_level(OrderPointer aggressive_order, OrderPointers &order_list)
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
