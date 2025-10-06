#pragma once

#include "types.hpp"
#include "rng.hpp"
#include "matching_engine.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

namespace mms {

// Base agent interface
class Agent {
public:
    virtual ~Agent() = default;
    
    // Generate events for the given timestamp
    virtual std::vector<Event> step(Timestamp timestamp) = 0;
    
    // Get agent ID
    OrderId get_id() const { return id_; }
    
    // Get agent name
    const std::string& get_name() const { return name_; }
    
    // Update PnL based on trade
    virtual void on_trade(const Trade& trade) = 0;
    
    // Get current PnL
    virtual double get_pnl() const = 0;
    
    // Get current inventory (positive = long, negative = short)
    virtual Qty get_inventory() const = 0;
    
    // Reset agent state
    virtual void reset() = 0;

protected:
    Agent(OrderId id, const std::string& name) : id_(id), name_(name) {}
    
    OrderId id_;
    std::string name_;
};

// Market maker agent that provides liquidity
class MarketMaker : public Agent {
public:
    struct Config {
        Price spread = 2;           // Bid-ask spread in ticks
        Qty quantity = 50;          // Order size
        Timestamp refresh_interval = 50000; // Refresh interval in nanoseconds
        Qty max_inventory = 1000;   // Maximum inventory position
        double inventory_penalty = 0.001; // Penalty for inventory imbalance
    };
    
    MarketMaker(OrderId id, const std::string& name, const Config& config, RNG& rng);
    
    std::vector<Event> step(Timestamp timestamp) override;
    void on_trade(const Trade& trade) override;
    double get_pnl() const override { return pnl_; }
    Qty get_inventory() const override { return inventory_; }
    void reset() override;
    
    // Get current quotes
    Price get_bid_price() const;
    Price get_ask_price() const;
    
    // Update configuration
    void update_config(const Config& config) { config_ = config; }

private:
    Config config_;
    RNG& rng_;
    
    // State
    Qty inventory_ = 0;
    double pnl_ = 0.0;
    Timestamp last_refresh_ = 0;
    Price current_bid_ = 0;
    Price current_ask_ = 0;
    OrderId bid_order_id_ = 0;
    OrderId ask_order_id_ = 0;
    
    // Helper methods
    void update_quotes(Price mid_price);
    void cancel_old_orders(Timestamp timestamp, std::vector<Event>& events);
    void place_new_orders(Price mid_price, Timestamp timestamp, std::vector<Event>& events);
    Price calculate_mid_price() const;
};

// Liquidity taker agent that consumes liquidity
class Taker : public Agent {
public:
    struct Config {
        double intensity = 0.8;     // Order arrival rate (orders per microsecond)
        double side_bias = 0.5;     // Probability of buy orders (0.5 = neutral)
        Qty quantity_mean = 40;     // Mean order size
        Qty quantity_std = 10;      // Order size standard deviation
        bool use_market_orders = true; // Use market orders vs aggressive limits
    };
    
    Taker(OrderId id, const std::string& name, const Config& config, RNG& rng);
    
    std::vector<Event> step(Timestamp timestamp) override;
    void on_trade(const Trade& trade) override;
    double get_pnl() const override { return pnl_; }
    Qty get_inventory() const override { return inventory_; }
    void reset() override;
    
    // Update configuration
    void update_config(const Config& config) { config_ = config; }

private:
    Config config_;
    RNG& rng_;
    
    // State
    Qty inventory_ = 0;
    double pnl_ = 0.0;
    Timestamp next_order_time_ = 0;
    
    // Helper methods
    Timestamp calculate_next_order_time(Timestamp current_time);
    Qty generate_order_quantity();
    Side generate_order_side();
    Price generate_aggressive_price(Side side, Price current_bid, Price current_ask);
};

// Noise trader agent for realistic market dynamics
class NoiseTrader : public Agent {
public:
    struct Config {
        double limit_intensity = 1.5;    // Limit order arrival rate
        double cancel_intensity = 0.7;   // Cancel order rate
        Qty quantity_mean = 30;          // Mean order size
        Qty quantity_std = 8;            // Order size standard deviation
        Price price_volatility = 5;      // Price volatility for limit orders
        double cancel_probability = 0.3; // Probability of canceling existing orders
    };
    
    NoiseTrader(OrderId id, const std::string& name, const Config& config, RNG& rng);
    
    std::vector<Event> step(Timestamp timestamp) override;
    void on_trade(const Trade& trade) override;
    double get_pnl() const override { return pnl_; }
    Qty get_inventory() const override { return inventory_; }
    void reset() override;
    
    // Update configuration
    void update_config(const Config& config) { config_ = config; }

private:
    Config config_;
    RNG& rng_;
    
    // State
    Qty inventory_ = 0;
    double pnl_ = 0.0;
    Timestamp next_limit_order_time_ = 0;
    Timestamp next_cancel_time_ = 0;
    
    // Track our orders for cancellation
    std::unordered_map<OrderId, Order> active_orders_;
    Price reference_price_ = 10000; // Reference price for limit orders
    
    // Helper methods
    Timestamp calculate_next_limit_order_time(Timestamp current_time);
    Timestamp calculate_next_cancel_time(Timestamp current_time);
    Qty generate_order_quantity();
    Side generate_order_side();
    Price generate_limit_price(Side side, Price reference_price);
    void maybe_cancel_random_order(Timestamp timestamp, std::vector<Event>& events);
};

// Agent manager for coordinating multiple agents
class AgentManager {
public:
    AgentManager() = default;
    
    // Add an agent
    void add_agent(std::unique_ptr<Agent> agent);
    
    // Get agent by ID
    Agent* get_agent(OrderId id) const;
    
    // Get all agents
    const std::vector<std::unique_ptr<Agent>>& get_agents() const { return agents_; }
    
    // Generate events from all agents
    std::vector<Event> step(Timestamp timestamp);
    
    // Notify all agents of a trade
    void notify_trade(const Trade& trade);
    
    // Get agent statistics
    struct AgentStats {
        OrderId id;
        std::string name;
        double pnl;
        Qty inventory;
        size_t trade_count;
    };
    
    std::vector<AgentStats> get_stats() const;
    
    // Reset all agents
    void reset();

private:
    std::vector<std::unique_ptr<Agent>> agents_;
    std::unordered_map<OrderId, Agent*> agent_lookup_;
};

} // namespace mms
