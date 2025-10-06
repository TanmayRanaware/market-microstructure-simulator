#pragma once

#include "types.hpp"
#include <map>
#include <deque>
#include <vector>
#include <optional>
#include <unordered_map>

namespace mms {

// Price level container for maintaining order queues
class OrderBookPriceLevel {
public:
    OrderBookPriceLevel() = default;
    
    void add_order(const Order& order) {
        orders_.push_back(order);
        total_quantity_ += order.quantity;
    }
    
    bool remove_order(OrderId order_id, Order& removed_order) {
        for (auto it = orders_.begin(); it != orders_.end(); ++it) {
            if (it->id == order_id) {
                removed_order = *it;
                total_quantity_ -= it->quantity;
                orders_.erase(it);
                return true;
            }
        }
        return false;
    }
    
    bool get_next_order(Order& order) {
        if (orders_.empty()) {
            return false;
        }
        order = orders_.front();
        return true;
    }
    
    bool consume_order(Qty quantity, Order& consumed_order) {
        if (orders_.empty()) {
            return false;
        }
        
        Order& front_order = orders_.front();
        if (front_order.quantity <= quantity) {
            // Consume entire order
            consumed_order = front_order;
            total_quantity_ -= front_order.quantity;
            orders_.pop_front();
            return true;
        } else {
            // Partial fill
            consumed_order = front_order;
            consumed_order.quantity = quantity;
            front_order.quantity -= quantity;
            total_quantity_ -= quantity;
            return false; // Order not fully consumed
        }
    }
    
    Qty total_quantity() const { return total_quantity_; }
    size_t order_count() const { return orders_.size(); }
    bool empty() const { return orders_.empty(); }
    
    // Get all orders for debugging
    const std::deque<Order>& get_orders() const { return orders_; }

private:
    std::deque<Order> orders_;
    Qty total_quantity_ = 0;
};

// Central Limit Order Book
class OrderBook {
public:
    OrderBook() = default;
    
    // Add a limit order to the book
    bool add_limit_order(const Order& order);
    
    // Add a market order (immediately match against book)
    std::vector<Trade> add_market_order(Side side, Qty quantity, OrderId taker_id, Timestamp timestamp);
    
    // Cancel an order
    bool cancel_order(OrderId order_id);
    
    // Get best bid price and quantity
    std::optional<Price> best_bid_price() const;
    std::optional<Qty> best_bid_quantity() const;
    
    // Get best ask price and quantity
    std::optional<Price> best_ask_price() const;
    std::optional<Qty> best_ask_quantity() const;
    
    // Get top of book (best bid and ask)
    MarketSnapshot top_of_book(Timestamp timestamp) const;
    
    // Get depth snapshot for N levels
    std::vector<PriceLevel> get_depth(int levels = 10) const;
    
    // Get total number of orders in book
    size_t size() const { return order_count_; }
    
    // Check if book is empty
    bool empty() const { return buy_levels_.empty() && sell_levels_.empty(); }
    
    // Get order by ID (for debugging)
    std::optional<Order> get_order(OrderId order_id) const;
    
    // Clear the entire book
    void clear();
    
    // Get last trade price
    Price last_trade_price() const { return last_trade_price_; }
    
    // Get total volume traded
    Qty total_volume() const { return total_volume_; }
    
    // Get number of trades
    size_t trade_count() const { return trade_count_; }

private:
    // Price level maps (buy: descending, sell: ascending)
    std::map<Price, OrderBookPriceLevel, std::greater<Price>> buy_levels_;
    std::map<Price, OrderBookPriceLevel> sell_levels_;
    
    // Order lookup for cancellations
    std::unordered_map<OrderId, std::pair<Price, Side>> order_lookup_;
    
    // Statistics
    size_t order_count_ = 0;
    Price last_trade_price_ = 0;
    Qty total_volume_ = 0;
    size_t trade_count_ = 0;
    
    // Helper methods
    bool add_limit_order_to_side(const Order& order);
    std::vector<Trade> match_market_order_buy(Qty quantity, OrderId taker_id, Timestamp timestamp);
    std::vector<Trade> match_market_order_sell(Qty quantity, OrderId taker_id, Timestamp timestamp);
    void remove_order_from_side(OrderId order_id, Price price, Side side);
};

} // namespace mms
