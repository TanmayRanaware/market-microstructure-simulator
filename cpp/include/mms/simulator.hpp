#pragma once

#include "types.hpp"
#include "matching_engine.hpp"
#include "agents.hpp"
#include "rng.hpp"
#include <vector>
#include <memory>
#include <string>
#include <fstream>

namespace mms {

// Simulation configuration
struct SimulationConfig {
    uint64_t seed = 42;                    // Random seed
    Timestamp start_time = 0;              // Simulation start time
    Timestamp time_step = 1000;            // Time step in nanoseconds
    size_t max_steps = 1000000;            // Maximum simulation steps
    bool enable_logging = false;           // Enable detailed logging
    std::string output_dir = "output";     // Output directory for results
};

// Market data collector for simulation results
class MarketDataCollector {
public:
    MarketDataCollector() = default;
    
    // Record a trade
    void record_trade(const Trade& trade);
    
    // Record market snapshot
    void record_snapshot(const MarketSnapshot& snapshot);
    
    // Record agent PnL
    void record_agent_pnl(OrderId agent_id, Timestamp timestamp, double pnl, Qty inventory);
    
    // Get collected data
    const std::vector<Trade>& get_trades() const { return trades_; }
    const std::vector<MarketSnapshot>& get_snapshots() const { return snapshots_; }
    const std::vector<std::tuple<OrderId, Timestamp, double, Qty>>& get_agent_pnl() const { return agent_pnl_; }
    
    // Clear all data
    void clear();
    
    // Save data to CSV files
    void save_to_csv(const std::string& output_dir) const;

private:
    std::vector<Trade> trades_;
    std::vector<MarketSnapshot> snapshots_;
    std::vector<std::tuple<OrderId, Timestamp, double, Qty>> agent_pnl_; // agent_id, timestamp, pnl, inventory
};

// Main simulation orchestrator
class Simulator {
public:
    explicit Simulator(const SimulationConfig& config = {});
    
    // Run simulation with agent configurations
    struct RunResult {
        std::vector<Trade> trades;
        std::vector<MarketSnapshot> market_snapshots;
        std::vector<std::tuple<OrderId, Timestamp, double, Qty>> agent_pnl;
        size_t total_events_processed;
        size_t total_trades;
        Timestamp simulation_duration;
        double simulation_time_seconds;
    };
    
    RunResult run(
        size_t n_steps,
        const MarketMaker::Config& maker_config = {},
        const Taker::Config& taker_config = {},
        const NoiseTrader::Config& noise_config = {}
    );
    
    // Run simulation with custom agents
    RunResult run_with_agents(size_t n_steps, std::vector<std::unique_ptr<Agent>> agents);
    
    // Get current simulation state
    const MatchingEngine& get_matching_engine() const { return matching_engine_; }
    const AgentManager& get_agent_manager() const { return agent_manager_; }
    const MarketDataCollector& get_data_collector() const { return data_collector_; }
    
    // Get simulation statistics
    struct SimulationStats {
        size_t total_events_processed;
        size_t total_trades;
        size_t total_orders;
        Qty total_volume;
        Price last_trade_price;
        double average_spread;
        double price_volatility;
        Timestamp simulation_duration;
        double events_per_second;
    };
    
    SimulationStats get_stats() const;
    
    // Reset simulation state
    void reset();
    
    // Update configuration
    void update_config(const SimulationConfig& config) { config_ = config; }
    
    // Enable/disable data collection
    void set_data_collection(bool enabled) { data_collection_enabled_ = enabled; }
    
    // Set output directory
    void set_output_dir(const std::string& dir) { config_.output_dir = dir; }

private:
    SimulationConfig config_;
    RNG rng_;
    MatchingEngine matching_engine_;
    AgentManager agent_manager_;
    MarketDataCollector data_collector_;
    
    bool data_collection_enabled_ = true;
    
    // Simulation state
    Timestamp current_time_;
    size_t current_step_;
    size_t events_processed_;
    
    // Helper methods
    void initialize_agents(
        const MarketMaker::Config& maker_config,
        const Taker::Config& taker_config,
        const NoiseTrader::Config& noise_config
    );
    
    void process_simulation_step();
    void collect_market_data();
    void collect_agent_data();
    
    // Statistics calculation
    double calculate_average_spread() const;
    double calculate_price_volatility() const;
};

// Utility functions for simulation analysis
namespace analysis {
    
    // Calculate volume-weighted average price (VWAP)
    double calculate_vwap(const std::vector<Trade>& trades);
    
    // Calculate time-weighted average price (TWAP)
    double calculate_twap(const std::vector<MarketSnapshot>& snapshots);
    
    // Calculate realized volatility
    double calculate_realized_volatility(const std::vector<MarketSnapshot>& snapshots);
    
    // Calculate bid-ask spread statistics
    struct SpreadStats {
        double mean_spread;
        double median_spread;
        double min_spread;
        double max_spread;
        double spread_volatility;
    };
    
    SpreadStats calculate_spread_stats(const std::vector<MarketSnapshot>& snapshots);
    
    // Calculate agent performance metrics
    struct AgentPerformance {
        OrderId agent_id;
        double total_pnl;
        double sharpe_ratio;
        double max_drawdown;
        Qty final_inventory;
        double win_rate;
        size_t total_trades;
    };
    
    std::vector<AgentPerformance> calculate_agent_performance(
        const std::vector<std::tuple<OrderId, Timestamp, double, Qty>>& agent_pnl,
        const std::vector<Trade>& trades
    );
}

} // namespace mms
