#include "trade.hpp"

Trade::Trade(const TradeInfo &bid_trade, const TradeInfo &ask_trade)
    : bid_trade_(bid_trade), ask_trade_(ask_trade) {}

const TradeInfo &Trade::get_bid_trade() const { return bid_trade_; }
const TradeInfo &Trade::get_ask_trade() const { return ask_trade_; }
