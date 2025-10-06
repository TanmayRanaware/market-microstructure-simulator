#include "mms/agents.hpp"
#include <algorithm>
#include <cmath>

namespace mms {

// MarketMaker implementation
MarketMaker::MarketMaker(OrderId id, const std::string& name, const Config& config, RNG& rng)
    : Agent(id, name), config_(config), rng_(rng) {
    reset();
}

std::vector<Event> MarketMaker::step(Timestamp timestamp) {
    std::vector<Event> events;
    
    // Update quotes based on current market conditions
    Price mid_price = calculate_mid_price();
    if (mid_price > 0) {
        update_quotes(mid_price);
    }
    
    // Cancel old orders if needed
    if (timestamp - last_refresh_ >= config_.refresh_interval) {
        cancel_old_orders(timestamp, events);
        place_new_orders(mid_price, timestamp, events);
        last_refresh_ = timestamp;
    }
    
    return events;
}

void MarketMaker::on_trade(const Trade& trade) {
    // Update inventory and PnL
    if (trade.maker_id == bid_order_id_) {
        // We were the maker on a buy order
        inventory_ -= trade.quantity;
        pnl_ += trade.quantity * trade.price;
        bid_order_id_ = 0; // Order was consumed
    } else if (trade.maker_id == ask_order_id_) {
        // We were the maker on a sell order
        inventory_ += trade.quantity;
        pnl_ -= trade.quantity * trade.price;
        ask_order_id_ = 0; // Order was consumed
    } else if (trade.taker_id == get_id()) {
        // We were the taker
        if (trade.maker_id == bid_order_id_) {
            inventory_ -= trade.quantity;
            pnl_ += trade.quantity * trade.price;
        } else if (trade.maker_id == ask_order_id_) {
            inventory_ += trade.quantity;
            pnl_ -= trade.quantity * trade.price;
        }
    }
    
    // Apply inventory penalty
    double inventory_penalty = std::abs(inventory_) * config_.inventory_penalty;
    pnl_ -= inventory_penalty;
}

void MarketMaker::reset() {
    inventory_ = 0;
    pnl_ = 0.0;
    last_refresh_ = 0;
    current_bid_ = 0;
    current_ask_ = 0;
    bid_order_id_ = 0;
    ask_order_id_ = 0;
}

Price MarketMaker::get_bid_price() const {
    return current_bid_;
}

Price MarketMaker::get_ask_price() const {
    return current_ask_;
}

void MarketMaker::update_quotes(Price mid_price) {
    // Calculate new bid and ask prices with spread
    Price half_spread = config_.spread / 2;
    current_bid_ = mid_price - half_spread;
    current_ask_ = mid_price + half_spread;
    
    // Adjust for inventory imbalance
    if (std::abs(inventory_) > config_.max_inventory / 2) {
        if (inventory_ > 0) {
            // Long inventory - lower ask price to encourage selling
            current_ask_ -= half_spread / 2;
        } else {
            // Short inventory - raise bid price to encourage buying
            current_bid_ += half_spread / 2;
        }
    }
}

Price MarketMaker::calculate_mid_price() const {
    // Simple mid price calculation - in practice this would come from market data
    return 10000; // Placeholder
}

void MarketMaker::cancel_old_orders(Timestamp timestamp, std::vector<Event>& events) {
    if (bid_order_id_ > 0) {
        events.emplace_back(EventType::CANCEL, bid_order_id_, Side::BUY, 0, 0, timestamp, get_id());
        bid_order_id_ = 0;
    }
    if (ask_order_id_ > 0) {
        events.emplace_back(EventType::CANCEL, ask_order_id_, Side::SELL, 0, 0, timestamp, get_id());
        ask_order_id_ = 0;
    }
}

void MarketMaker::place_new_orders(Price mid_price, Timestamp timestamp, std::vector<Event>& events) {
    if (mid_price > 0) {
        // Place bid order
        bid_order_id_ = timestamp + get_id(); // Simple ID generation
        events.emplace_back(EventType::LIMIT, bid_order_id_, Side::BUY, current_bid_, 
                           config_.quantity, timestamp, get_id());
        
        // Place ask order
        ask_order_id_ = timestamp + get_id() + 1;
        events.emplace_back(EventType::LIMIT, ask_order_id_, Side::SELL, current_ask_, 
                           config_.quantity, timestamp, get_id());
    }
}

// Taker implementation
Taker::Taker(OrderId id, const std::string& name, const Config& config, RNG& rng)
    : Agent(id, name), config_(config), rng_(rng) {
    reset();
}

std::vector<Event> Taker::step(Timestamp timestamp) {
    std::vector<Event> events;
    
    if (timestamp >= next_order_time_) {
        // Generate new order
        Qty quantity = generate_order_quantity();
        Side side = generate_order_side();
        
        OrderId order_id = timestamp + get_id();
        
        if (config_.use_market_orders) {
            events.emplace_back(EventType::MARKET, order_id, side, 0, quantity, timestamp, get_id());
        } else {
            // Use aggressive limit orders
            Price price = generate_aggressive_price(side, 10000, 10002); // Placeholder prices
            events.emplace_back(EventType::LIMIT, order_id, side, price, quantity, timestamp, get_id());
        }
        
        // Schedule next order
        next_order_time_ = calculate_next_order_time(timestamp);
    }
    
    return events;
}

void Taker::on_trade(const Trade& trade) {
    if (trade.taker_id == get_id()) {
        // We were the taker
        if (trade.maker_id == get_id()) {
            // This shouldn't happen for taker
            return;
        }
        
        // Update inventory and PnL based on trade side
        // This is a simplified PnL calculation
        pnl_ -= trade.quantity * trade.price; // Simplified - assume we're always paying
    }
}

void Taker::reset() {
    inventory_ = 0;
    pnl_ = 0.0;
    next_order_time_ = 0;
}

Timestamp Taker::calculate_next_order_time(Timestamp current_time) {
    double inter_arrival = rng_.exponential(config_.intensity);
    return current_time + static_cast<Timestamp>(inter_arrival * 1000000); // Convert to nanoseconds
}

Qty Taker::generate_order_quantity() {
    double quantity = rng_.normal(config_.quantity_mean, config_.quantity_std);
    return std::max(static_cast<Qty>(1), static_cast<Qty>(std::round(quantity)));
}

Side Taker::generate_order_side() {
    return rng_.bernoulli(config_.side_bias) ? Side::BUY : Side::SELL;
}

Price Taker::generate_aggressive_price(Side side, Price current_bid, Price current_ask) {
    if (side == Side::BUY) {
        // Aggressive buy - cross the spread
        return current_ask + 1;
    } else {
        // Aggressive sell - cross the spread
        return current_bid - 1;
    }
}

// NoiseTrader implementation
NoiseTrader::NoiseTrader(OrderId id, const std::string& name, const Config& config, RNG& rng)
    : Agent(id, name), config_(config), rng_(rng) {
    reset();
}

std::vector<Event> NoiseTrader::step(Timestamp timestamp) {
    std::vector<Event> events;
    
    // Check for limit order placement
    if (timestamp >= next_limit_order_time_) {
        Qty quantity = generate_order_quantity();
        Side side = generate_order_side();
        Price price = generate_limit_price(side, reference_price_);
        
        OrderId order_id = timestamp + get_id() + rng_.uniform_int(0, 1000);
        
        events.emplace_back(EventType::LIMIT, order_id, side, price, quantity, timestamp, get_id());
        
        // Track the order for potential cancellation
        active_orders_[order_id] = Order(order_id, side, price, quantity, timestamp);
        
        // Schedule next limit order
        next_limit_order_time_ = calculate_next_limit_order_time(timestamp);
    }
    
    // Check for order cancellation
    if (timestamp >= next_cancel_time_) {
        maybe_cancel_random_order(timestamp, events);
        next_cancel_time_ = calculate_next_cancel_time(timestamp);
    }
    
    return events;
}

void NoiseTrader::on_trade(const Trade& trade) {
    // Update inventory and PnL
    if (trade.maker_id == get_id()) {
        // We were the maker
        if (trade.maker_id == get_id()) {
            if (active_orders_.count(trade.maker_id)) {
                auto& order = active_orders_[trade.maker_id];
                if (order.side == Side::BUY) {
                    inventory_ += trade.quantity;
                } else {
                    inventory_ -= trade.quantity;
                }
                pnl_ += trade.quantity * trade.price;
            }
        }
    }
    
    // Remove filled orders from active orders
    active_orders_.erase(trade.maker_id);
}

void NoiseTrader::reset() {
    inventory_ = 0;
    pnl_ = 0.0;
    next_limit_order_time_ = 0;
    next_cancel_time_ = 0;
    active_orders_.clear();
    reference_price_ = 10000;
}

Timestamp NoiseTrader::calculate_next_limit_order_time(Timestamp current_time) {
    double inter_arrival = rng_.exponential(config_.limit_intensity);
    return current_time + static_cast<Timestamp>(inter_arrival * 1000000);
}

Timestamp NoiseTrader::calculate_next_cancel_time(Timestamp current_time) {
    double inter_arrival = rng_.exponential(config_.cancel_intensity);
    return current_time + static_cast<Timestamp>(inter_arrival * 1000000);
}

Qty NoiseTrader::generate_order_quantity() {
    double quantity = rng_.normal(config_.quantity_mean, config_.quantity_std);
    return std::max(static_cast<Qty>(1), static_cast<Qty>(std::round(quantity)));
}

Side NoiseTrader::generate_order_side() {
    return rng_.bernoulli(0.5) ? Side::BUY : Side::SELL;
}

Price NoiseTrader::generate_limit_price(Side side, Price reference_price) {
    // Add some volatility around reference price
    double price_offset = rng_.normal(0, config_.price_volatility);
    Price price = reference_price + static_cast<Price>(std::round(price_offset));
    
    // Ensure price is positive
    return std::max(static_cast<Price>(1), price);
}

void NoiseTrader::maybe_cancel_random_order(Timestamp timestamp, std::vector<Event>& events) {
    if (active_orders_.empty()) {
        return;
    }
    
    if (rng_.bernoulli(config_.cancel_probability)) {
        // Choose random order to cancel
        auto it = active_orders_.begin();
        std::advance(it, rng_.uniform_int<size_t>(0, active_orders_.size() - 1));
        
        OrderId order_id = it->first;
        events.emplace_back(EventType::CANCEL, order_id, Side::BUY, 0, 0, timestamp, get_id());
        active_orders_.erase(it);
    }
}

// AgentManager implementation
void AgentManager::add_agent(std::unique_ptr<Agent> agent) {
    if (agent) {
        agent_lookup_[agent->get_id()] = agent.get();
        agents_.push_back(std::move(agent));
    }
}

Agent* AgentManager::get_agent(OrderId id) const {
    auto it = agent_lookup_.find(id);
    return it != agent_lookup_.end() ? it->second : nullptr;
}

std::vector<Event> AgentManager::step(Timestamp timestamp) {
    std::vector<Event> all_events;
    
    for (auto& agent : agents_) {
        auto events = agent->step(timestamp);
        all_events.insert(all_events.end(), events.begin(), events.end());
    }
    
    return all_events;
}

void AgentManager::notify_trade(const Trade& trade) {
    for (auto& agent : agents_) {
        agent->on_trade(trade);
    }
}

std::vector<AgentManager::AgentStats> AgentManager::get_stats() const {
    std::vector<AgentStats> stats;
    
    for (const auto& agent : agents_) {
        stats.push_back({
            agent->get_id(),
            agent->get_name(),
            agent->get_pnl(),
            agent->get_inventory(),
            0 // trade_count would need to be tracked separately
        });
    }
    
    return stats;
}

void AgentManager::reset() {
    for (auto& agent : agents_) {
        agent->reset();
    }
}

} // namespace mms
