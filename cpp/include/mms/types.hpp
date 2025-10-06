#pragma once

#include <cstdint>
#include <string>

namespace mms {

// Core type definitions
using OrderId = uint64_t;
using Price = int64_t;
using Qty = int64_t;
using Timestamp = int64_t;

// Order side enumeration
enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

// Event types
enum class EventType : uint8_t {
    LIMIT = 0,
    MARKET = 1,
    CANCEL = 2
};

// Order structure
struct Order {
    OrderId id;
    Side side;
    Price price;
    Qty quantity;
    Timestamp timestamp;
    
    Order() = default;
    Order(OrderId id, Side side, Price price, Qty quantity, Timestamp timestamp)
        : id(id), side(side), price(price), quantity(quantity), timestamp(timestamp) {}
};

// Trade structure
struct Trade {
    OrderId maker_id;
    OrderId taker_id;
    Price price;
    Qty quantity;
    Timestamp timestamp;
    
    Trade() = default;
    Trade(OrderId maker_id, OrderId taker_id, Price price, Qty quantity, Timestamp timestamp)
        : maker_id(maker_id), taker_id(taker_id), price(price), quantity(quantity), timestamp(timestamp) {}
};

// Price level for depth snapshots
struct PriceLevel {
    Price price;
    Qty bid_quantity;
    Qty ask_quantity;
    
    PriceLevel() = default;
    PriceLevel(Price price, Qty bid_qty, Qty ask_qty)
        : price(price), bid_quantity(bid_qty), ask_quantity(ask_qty) {}
};

// Market data snapshot
struct MarketSnapshot {
    Price best_bid;
    Price best_ask;
    Qty best_bid_qty;
    Qty best_ask_qty;
    Price last_trade_price;
    Timestamp timestamp;
    
    MarketSnapshot() = default;
    MarketSnapshot(Price best_bid, Price best_ask, Qty best_bid_qty, Qty best_ask_qty, 
                   Price last_trade_price, Timestamp timestamp)
        : best_bid(best_bid), best_ask(best_ask), best_bid_qty(best_bid_qty), 
          best_ask_qty(best_ask_qty), last_trade_price(last_trade_price), timestamp(timestamp) {}
};

// Helper functions
inline const char* side_to_string(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

inline const char* event_type_to_string(EventType type) {
    switch (type) {
        case EventType::LIMIT: return "LIMIT";
        case EventType::MARKET: return "MARKET";
        case EventType::CANCEL: return "CANCEL";
        default: return "UNKNOWN";
    }
}

inline bool is_valid_price(Price price) {
    return price > 0;
}

inline bool is_valid_quantity(Qty quantity) {
    return quantity > 0;
}

inline Price get_mid_price(Price best_bid, Price best_ask) {
    if (best_bid > 0 && best_ask > 0) {
        return (best_bid + best_ask) / 2;
    }
    return 0;
}

inline Price get_spread(Price best_bid, Price best_ask) {
    if (best_bid > 0 && best_ask > 0) {
        return best_ask - best_bid;
    }
    return 0;
}

} // namespace mms
