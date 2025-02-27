#include "order_book.hpp"
#include "logger.hpp"
#include <iostream>

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

    ob.modify_order(6, 105, 8);

    try
    {
        ob.modify_order(6, 105, 5);
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception: " << e.what() << "\n";
    }

    print_trade_history(ob.get_trade_history());
}
