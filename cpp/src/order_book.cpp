#include "mms/order_book.hpp"
#include <algorithm>
#include <stdexcept>

namespace mms {

bool OrderBook::add_limit_order(const Order& order) {
    if (!is_valid_price(order.price) || !is_valid_quantity(order.quantity)) {
        return false;
    }
    
    return add_limit_order_to_side(order);
}

bool OrderBook::add_limit_order_to_side(const Order& order) {
    // Add to appropriate side
    if (order.side == Side::BUY) {
        buy_levels_[order.price].add_order(order);
    } else {
        sell_levels_[order.price].add_order(order);
    }
    
    // Update order lookup for cancellations
    order_lookup_[order.id] = std::make_pair(order.price, order.side);
    order_count_++;
    
    return true;
}

std::vector<Trade> OrderBook::add_market_order(Side side, Qty quantity, OrderId taker_id, Timestamp timestamp) {
    if (side == Side::BUY) {
        return match_market_order_buy(quantity, taker_id, timestamp);
    } else {
        return match_market_order_sell(quantity, taker_id, timestamp);
    }
}

std::vector<Trade> OrderBook::match_market_order_buy(Qty quantity, OrderId taker_id, Timestamp timestamp) {
    std::vector<Trade> trades;
    Qty remaining_qty = quantity;
    
    // Match against sell levels (ascending price order)
    for (auto it = sell_levels_.begin(); it != sell_levels_.end() && remaining_qty > 0; ) {
        Price price = it->first;
        OrderBookPriceLevel& level = it->second;
        
        while (!level.empty() && remaining_qty > 0) {
            Order maker_order;
            bool fully_consumed = level.consume_order(remaining_qty, maker_order);
            
            if (fully_consumed) {
                // Order fully consumed
                remaining_qty -= maker_order.quantity;
                trades.emplace_back(maker_order.id, taker_id, price, maker_order.quantity, timestamp);
                
                // Remove from order lookup
                order_lookup_.erase(maker_order.id);
                order_count_--;
                
                // Update statistics
                last_trade_price_ = price;
                total_volume_ += maker_order.quantity;
                trade_count_++;
            } else {
                // Partial fill - remaining_qty is now 0
                trades.emplace_back(maker_order.id, taker_id, price, remaining_qty, timestamp);
                remaining_qty = 0;
                
                // Update statistics
                last_trade_price_ = price;
                total_volume_ += remaining_qty;
                trade_count_++;
            }
        }
        
        // Remove empty price level
        if (level.empty()) {
            it = sell_levels_.erase(it);
        } else {
            ++it;
        }
    }
    
    return trades;
}

std::vector<Trade> OrderBook::match_market_order_sell(Qty quantity, OrderId taker_id, Timestamp timestamp) {
    std::vector<Trade> trades;
    Qty remaining_qty = quantity;
    
    // Match against buy levels (descending price order)
    for (auto it = buy_levels_.begin(); it != buy_levels_.end() && remaining_qty > 0; ) {
        Price price = it->first;
        OrderBookPriceLevel& level = it->second;
        
        while (!level.empty() && remaining_qty > 0) {
            Order maker_order;
            bool fully_consumed = level.consume_order(remaining_qty, maker_order);
            
            if (fully_consumed) {
                // Order fully consumed
                remaining_qty -= maker_order.quantity;
                trades.emplace_back(maker_order.id, taker_id, price, maker_order.quantity, timestamp);
                
                // Remove from order lookup
                order_lookup_.erase(maker_order.id);
                order_count_--;
                
                // Update statistics
                last_trade_price_ = price;
                total_volume_ += maker_order.quantity;
                trade_count_++;
            } else {
                // Partial fill - remaining_qty is now 0
                trades.emplace_back(maker_order.id, taker_id, price, remaining_qty, timestamp);
                remaining_qty = 0;
                
                // Update statistics
                last_trade_price_ = price;
                total_volume_ += remaining_qty;
                trade_count_++;
            }
        }
        
        // Remove empty price level
        if (level.empty()) {
            it = buy_levels_.erase(it);
        } else {
            ++it;
        }
    }
    
    return trades;
}

bool OrderBook::cancel_order(OrderId order_id) {
    auto it = order_lookup_.find(order_id);
    if (it == order_lookup_.end()) {
        return false; // Order not found
    }
    
    Price price = it->second.first;
    Side side = it->second.second;
    
    remove_order_from_side(order_id, price, side);
    return true;
}

void OrderBook::remove_order_from_side(OrderId order_id, Price price, Side side) {
    Order removed_order;
    
    if (side == Side::BUY) {
        auto level_it = buy_levels_.find(price);
        if (level_it != buy_levels_.end()) {
            if (level_it->second.remove_order(order_id, removed_order)) {
                order_lookup_.erase(order_id);
                order_count_--;
                
                // Remove empty price level
                if (level_it->second.empty()) {
                    buy_levels_.erase(level_it);
                }
            }
        }
    } else {
        auto level_it = sell_levels_.find(price);
        if (level_it != sell_levels_.end()) {
            if (level_it->second.remove_order(order_id, removed_order)) {
                order_lookup_.erase(order_id);
                order_count_--;
                
                // Remove empty price level
                if (level_it->second.empty()) {
                    sell_levels_.erase(level_it);
                }
            }
        }
    }
}

std::optional<Price> OrderBook::best_bid_price() const {
    if (buy_levels_.empty()) {
        return std::nullopt;
    }
    return buy_levels_.begin()->first;
}

std::optional<Qty> OrderBook::best_bid_quantity() const {
    if (buy_levels_.empty()) {
        return std::nullopt;
    }
    return buy_levels_.begin()->second.total_quantity();
}

std::optional<Price> OrderBook::best_ask_price() const {
    if (sell_levels_.empty()) {
        return std::nullopt;
    }
    return sell_levels_.begin()->first;
}

std::optional<Qty> OrderBook::best_ask_quantity() const {
    if (sell_levels_.empty()) {
        return std::nullopt;
    }
    return sell_levels_.begin()->second.total_quantity();
}

MarketSnapshot OrderBook::top_of_book(Timestamp timestamp) const {
    auto best_bid = best_bid_price();
    auto best_ask = best_ask_price();
    auto best_bid_qty = best_bid_quantity();
    auto best_ask_qty = best_ask_quantity();
    
    return MarketSnapshot(
        best_bid.value_or(0),
        best_ask.value_or(0),
        best_bid_qty.value_or(0),
        best_ask_qty.value_or(0),
        last_trade_price_,
        timestamp
    );
}

std::vector<PriceLevel> OrderBook::get_depth(int levels) const {
    std::vector<PriceLevel> depth;
    depth.reserve(levels * 2); // Reserve for both sides
    
    // Add bid levels (descending price)
    auto buy_it = buy_levels_.begin();
    for (int i = 0; i < levels && buy_it != buy_levels_.end(); ++i, ++buy_it) {
        Price price = buy_it->first;
        Qty qty = buy_it->second.total_quantity();
        depth.emplace_back(price, qty, 0);
    }
    
    // Add ask levels (ascending price)
    auto sell_it = sell_levels_.begin();
    for (int i = 0; i < levels && sell_it != sell_levels_.end(); ++i, ++sell_it) {
        Price price = sell_it->first;
        Qty qty = sell_it->second.total_quantity();
        depth.emplace_back(price, 0, qty);
    }
    
    return depth;
}

std::optional<Order> OrderBook::get_order(OrderId order_id) const {
    auto it = order_lookup_.find(order_id);
    if (it == order_lookup_.end()) {
        return std::nullopt;
    }
    
    Price price = it->second.first;
    Side side = it->second.second;
    
    if (side == Side::BUY) {
        auto level_it = buy_levels_.find(price);
        if (level_it != buy_levels_.end()) {
            for (const auto& order : level_it->second.get_orders()) {
                if (order.id == order_id) {
                    return order;
                }
            }
        }
    } else {
        auto level_it = sell_levels_.find(price);
        if (level_it != sell_levels_.end()) {
            for (const auto& order : level_it->second.get_orders()) {
                if (order.id == order_id) {
                    return order;
                }
            }
        }
    }
    
    return std::nullopt;
}

void OrderBook::clear() {
    buy_levels_.clear();
    sell_levels_.clear();
    order_lookup_.clear();
    order_count_ = 0;
    last_trade_price_ = 0;
    total_volume_ = 0;
    trade_count_ = 0;
}

} // namespace mms
