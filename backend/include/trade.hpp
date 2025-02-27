#ifndef TRADE_HPP
#define TRADE_HPP

#include "order.hpp"
#include <vector>

struct TradeInfo
{
    OrderID order_id;
    Price price;
    Quantity quantity;
};

class Trade
{
public:
    Trade(const TradeInfo &bid_trade, const TradeInfo &ask_trade);
    const TradeInfo &get_bid_trade() const;
    const TradeInfo &get_ask_trade() const;

private:
    TradeInfo bid_trade_;
    TradeInfo ask_trade_;
};

using Trades = std::vector<Trade>;

#endif // TRADE_HPP
