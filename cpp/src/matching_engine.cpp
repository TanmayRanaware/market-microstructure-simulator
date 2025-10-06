#include "mms/matching_engine.hpp"
#include <algorithm>

namespace mms {

std::vector<Trade> MatchingEngine::process_event(const Event& event) {
    switch (event.type) {
        case EventType::LIMIT:
            return process_limit_order(event);
        case EventType::MARKET:
            return process_market_order(event);
        case EventType::CANCEL:
            return process_cancel_order(event);
        default:
            return {}; // Unknown event type
    }
}

std::vector<Trade> MatchingEngine::process_events(const std::vector<Event>& events) {
    std::vector<Trade> all_trades;
    
    for (const auto& event : events) {
        auto trades = process_event(event);
        all_trades.insert(all_trades.end(), trades.begin(), trades.end());
    }
    
    return all_trades;
}

std::vector<Trade> MatchingEngine::process_limit_order(const Event& event) {
    std::vector<Trade> trades;
    
    // Create order from event
    Order order(event.order_id, event.side, event.price, event.quantity, event.timestamp);
    
    // Add to order book
    if (order_book_.add_limit_order(order)) {
        notify_order(order);
        
        // Check for immediate crossing
        if (event.side == Side::BUY) {
            // Check if buy order crosses with existing sell orders
            auto best_ask = order_book_.best_ask_price();
            if (best_ask && event.price >= best_ask.value()) {
                // Market order against sell side
                auto cross_trades = order_book_.add_market_order(
                    Side::SELL, event.quantity, event.order_id, event.timestamp
                );
                trades.insert(trades.end(), cross_trades.begin(), cross_trades.end());
            }
        } else {
            // Check if sell order crosses with existing buy orders
            auto best_bid = order_book_.best_bid_price();
            if (best_bid && event.price <= best_bid.value()) {
                // Market order against buy side
                auto cross_trades = order_book_.add_market_order(
                    Side::BUY, event.quantity, event.order_id, event.timestamp
                );
                trades.insert(trades.end(), cross_trades.begin(), cross_trades.end());
            }
        }
    }
    
    // Notify trades
    for (const auto& trade : trades) {
        notify_trade(trade);
    }
    
    return trades;
}

std::vector<Trade> MatchingEngine::process_market_order(const Event& event) {
    std::vector<Trade> trades;
    
    // Execute market order immediately
    trades = order_book_.add_market_order(
        event.side, event.quantity, event.order_id, event.timestamp
    );
    
    // Notify trades
    for (const auto& trade : trades) {
        notify_trade(trade);
    }
    
    return trades;
}

std::vector<Trade> MatchingEngine::process_cancel_order(const Event& event) {
    std::vector<Trade> trades; // Cancels don't generate trades
    
    // Cancel the order
    order_book_.cancel_order(event.order_id);
    
    return trades;
}

void MatchingEngine::notify_trade(const Trade& trade) {
    if (trade_callback_) {
        trade_callback_(trade);
    }
}

void MatchingEngine::notify_order(const Order& order) {
    if (order_callback_) {
        order_callback_(order);
    }
}

} // namespace mms
