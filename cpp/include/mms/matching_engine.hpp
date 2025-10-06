#pragma once

#include "types.hpp"
#include "order_book.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace mms {

// Event structure for the matching engine
struct Event {
    EventType type;
    OrderId order_id;
    Side side;
    Price price;
    Qty quantity;
    Timestamp timestamp;
    OrderId agent_id; // Which agent generated this event
    
    Event() = default;
    Event(EventType type, OrderId order_id, Side side, Price price, Qty quantity, 
          Timestamp timestamp, OrderId agent_id)
        : type(type), order_id(order_id), side(side), price(price), 
          quantity(quantity), timestamp(timestamp), agent_id(agent_id) {}
};

// Matching engine that processes events and maintains order book
class MatchingEngine {
public:
    MatchingEngine() = default;
    
    // Process a single event and return any trades that occurred
    std::vector<Trade> process_event(const Event& event);
    
    // Process multiple events in sequence
    std::vector<Trade> process_events(const std::vector<Event>& events);
    
    // Get current market snapshot
    MarketSnapshot get_market_snapshot(Timestamp timestamp) const {
        return order_book_.top_of_book(timestamp);
    }
    
    // Get order book depth
    std::vector<PriceLevel> get_depth(int levels = 10) const {
        return order_book_.get_depth(levels);
    }
    
    // Get order book reference (for direct access if needed)
    const OrderBook& get_order_book() const { return order_book_; }
    OrderBook& get_order_book() { return order_book_; }
    
    // Get statistics
    size_t order_count() const { return order_book_.size(); }
    Price last_trade_price() const { return order_book_.last_trade_price(); }
    Qty total_volume() const { return order_book_.total_volume(); }
    size_t trade_count() const { return order_book_.trade_count(); }
    
    // Clear the engine (reset state)
    void clear() { order_book_.clear(); }
    
    // Set trade callback for external monitoring
    void set_trade_callback(std::function<void(const Trade&)> callback) {
        trade_callback_ = std::move(callback);
    }
    
    // Set order callback for external monitoring
    void set_order_callback(std::function<void(const Order&)> callback) {
        order_callback_ = std::move(callback);
    }

private:
    OrderBook order_book_;
    std::vector<Trade> trades_;
    
    // Callbacks for external monitoring
    std::function<void(const Trade&)> trade_callback_;
    std::function<void(const Order&)> order_callback_;
    
    // Helper methods
    std::vector<Trade> process_limit_order(const Event& event);
    std::vector<Trade> process_market_order(const Event& event);
    std::vector<Trade> process_cancel_order(const Event& event);
    
    void notify_trade(const Trade& trade);
    void notify_order(const Order& order);
};

} // namespace mms
