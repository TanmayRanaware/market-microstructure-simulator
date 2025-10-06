#include "mms/simulator.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace mms {

// MarketDataCollector implementation
void MarketDataCollector::record_trade(const Trade& trade) {
    trades_.push_back(trade);
}

void MarketDataCollector::record_snapshot(const MarketSnapshot& snapshot) {
    snapshots_.push_back(snapshot);
}

void MarketDataCollector::record_agent_pnl(OrderId agent_id, Timestamp timestamp, double pnl, Qty inventory) {
    agent_pnl_.emplace_back(agent_id, timestamp, pnl, inventory);
}

void MarketDataCollector::clear() {
    trades_.clear();
    snapshots_.clear();
    agent_pnl_.clear();
}

void MarketDataCollector::save_to_csv(const std::string& output_dir) const {
    // Save trades
    std::ofstream trades_file(output_dir + "/trades.csv");
    trades_file << "timestamp,maker_id,taker_id,price,quantity\n";
    for (const auto& trade : trades_) {
        trades_file << trade.timestamp << "," << trade.maker_id << "," 
                   << trade.taker_id << "," << trade.price << "," << trade.quantity << "\n";
    }
    trades_file.close();
    
    // Save market snapshots
    std::ofstream snapshots_file(output_dir + "/market_snapshots.csv");
    snapshots_file << "timestamp,best_bid,best_ask,best_bid_qty,best_ask_qty,last_trade_price\n";
    for (const auto& snapshot : snapshots_) {
        snapshots_file << snapshot.timestamp << "," << snapshot.best_bid << "," 
                      << snapshot.best_ask << "," << snapshot.best_bid_qty << "," 
                      << snapshot.best_ask_qty << "," << snapshot.last_trade_price << "\n";
    }
    snapshots_file.close();
    
    // Save agent PnL
    std::ofstream pnl_file(output_dir + "/agent_pnl.csv");
    pnl_file << "timestamp,agent_id,pnl,inventory\n";
    for (const auto& pnl_record : agent_pnl_) {
        pnl_file << std::get<1>(pnl_record) << "," << std::get<0>(pnl_record) << "," 
                << std::get<2>(pnl_record) << "," << std::get<3>(pnl_record) << "\n";
    }
    pnl_file.close();
}

// Simulator implementation
Simulator::Simulator(const SimulationConfig& config)
    : config_(config), rng_(config.seed), current_time_(config.start_time), 
      current_step_(0), events_processed_(0) {
}

Simulator::RunResult Simulator::run(
    size_t n_steps,
    const MarketMaker::Config& maker_config,
    const Taker::Config& taker_config,
    const NoiseTrader::Config& noise_config
) {
    reset();
    initialize_agents(maker_config, taker_config, noise_config);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (current_step_ = 0; current_step_ < n_steps; ++current_step_) {
        process_simulation_step();
        current_time_ += config_.time_step;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double simulation_time_seconds = duration.count() / 1e6;
    
    // Collect final data
    collect_market_data();
    collect_agent_data();
    
    RunResult result;
    result.trades = data_collector_.get_trades();
    result.market_snapshots = data_collector_.get_snapshots();
    result.agent_pnl = data_collector_.get_agent_pnl();
    result.total_events_processed = events_processed_;
    result.total_trades = matching_engine_.trade_count();
    result.simulation_duration = current_time_ - config_.start_time;
    result.simulation_time_seconds = simulation_time_seconds;
    
    // Save data if output directory is specified
    if (!config_.output_dir.empty()) {
        data_collector_.save_to_csv(config_.output_dir);
    }
    
    return result;
}

Simulator::RunResult Simulator::run_with_agents(size_t n_steps, std::vector<std::unique_ptr<Agent>> agents) {
    reset();
    
    // Add custom agents
    for (auto& agent : agents) {
        agent_manager_.add_agent(std::move(agent));
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (current_step_ = 0; current_step_ < n_steps; ++current_step_) {
        process_simulation_step();
        current_time_ += config_.time_step;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double simulation_time_seconds = duration.count() / 1e6;
    
    // Collect final data
    collect_market_data();
    collect_agent_data();
    
    RunResult result;
    result.trades = data_collector_.get_trades();
    result.market_snapshots = data_collector_.get_snapshots();
    result.agent_pnl = data_collector_.get_agent_pnl();
    result.total_events_processed = events_processed_;
    result.total_trades = matching_engine_.trade_count();
    result.simulation_duration = current_time_ - config_.start_time;
    result.simulation_time_seconds = simulation_time_seconds;
    
    return result;
}

Simulator::SimulationStats Simulator::get_stats() const {
    SimulationStats stats;
    stats.total_events_processed = events_processed_;
    stats.total_trades = matching_engine_.trade_count();
    stats.total_orders = matching_engine_.order_count();
    stats.total_volume = matching_engine_.total_volume();
    stats.last_trade_price = matching_engine_.last_trade_price();
    stats.average_spread = calculate_average_spread();
    stats.price_volatility = calculate_price_volatility();
    stats.simulation_duration = current_time_ - config_.start_time;
    stats.events_per_second = events_processed_ / (stats.simulation_duration / 1e9);
    
    return stats;
}

void Simulator::reset() {
    matching_engine_.clear();
    agent_manager_.reset();
    data_collector_.clear();
    current_time_ = config_.start_time;
    current_step_ = 0;
    events_processed_ = 0;
    rng_.seed(config_.seed);
}

void Simulator::initialize_agents(
    const MarketMaker::Config& maker_config,
    const Taker::Config& taker_config,
    const NoiseTrader::Config& noise_config
) {
    // Add market maker
    auto maker = std::make_unique<MarketMaker>(1, "MarketMaker", maker_config, rng_);
    agent_manager_.add_agent(std::move(maker));
    
    // Add taker
    auto taker = std::make_unique<Taker>(2, "Taker", taker_config, rng_);
    agent_manager_.add_agent(std::move(taker));
    
    // Add noise trader
    auto noise = std::make_unique<NoiseTrader>(3, "NoiseTrader", noise_config, rng_);
    agent_manager_.add_agent(std::move(noise));
}

void Simulator::process_simulation_step() {
    // Generate events from agents
    auto events = agent_manager_.step(current_time_);
    
    // Process events through matching engine
    auto trades = matching_engine_.process_events(events);
    events_processed_ += events.size();
    
    // Notify agents of trades
    for (const auto& trade : trades) {
        agent_manager_.notify_trade(trade);
    }
    
    // Collect data periodically
    if (current_step_ % 100 == 0) { // Every 100 steps
        collect_market_data();
    }
    
    if (current_step_ % 1000 == 0) { // Every 1000 steps
        collect_agent_data();
    }
}

void Simulator::collect_market_data() {
    if (data_collection_enabled_) {
        auto snapshot = matching_engine_.get_market_snapshot(current_time_);
        data_collector_.record_snapshot(snapshot);
    }
}

void Simulator::collect_agent_data() {
    if (data_collection_enabled_) {
        auto agent_stats = agent_manager_.get_stats();
        for (const auto& stat : agent_stats) {
            data_collector_.record_agent_pnl(stat.id, current_time_, stat.pnl, stat.inventory);
        }
    }
}

double Simulator::calculate_average_spread() const {
    const auto& snapshots = data_collector_.get_snapshots();
    if (snapshots.empty()) {
        return 0.0;
    }
    
    double total_spread = 0.0;
    int valid_spreads = 0;
    
    for (const auto& snapshot : snapshots) {
        if (snapshot.best_bid > 0 && snapshot.best_ask > 0) {
            total_spread += (snapshot.best_ask - snapshot.best_bid);
            valid_spreads++;
        }
    }
    
    return valid_spreads > 0 ? total_spread / valid_spreads : 0.0;
}

double Simulator::calculate_price_volatility() const {
    const auto& snapshots = data_collector_.get_snapshots();
    if (snapshots.size() < 2) {
        return 0.0;
    }
    
    std::vector<double> returns;
    for (size_t i = 1; i < snapshots.size(); ++i) {
        if (snapshots[i].last_trade_price > 0 && snapshots[i-1].last_trade_price > 0) {
            double ret = std::log(snapshots[i].last_trade_price / snapshots[i-1].last_trade_price);
            returns.push_back(ret);
        }
    }
    
    if (returns.empty()) {
        return 0.0;
    }
    
    // Calculate standard deviation of returns
    double mean = 0.0;
    for (double ret : returns) {
        mean += ret;
    }
    mean /= returns.size();
    
    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean) * (ret - mean);
    }
    variance /= returns.size();
    
    return std::sqrt(variance);
}

// Analysis utility functions
namespace analysis {

double calculate_vwap(const std::vector<Trade>& trades) {
    if (trades.empty()) {
        return 0.0;
    }
    
    double total_value = 0.0;
    Qty total_quantity = 0;
    
    for (const auto& trade : trades) {
        total_value += trade.price * trade.quantity;
        total_quantity += trade.quantity;
    }
    
    return total_quantity > 0 ? total_value / total_quantity : 0.0;
}

double calculate_twap(const std::vector<MarketSnapshot>& snapshots) {
    if (snapshots.empty()) {
        return 0.0;
    }
    
    double total_price = 0.0;
    int valid_prices = 0;
    
    for (const auto& snapshot : snapshots) {
        Price mid_price = get_mid_price(snapshot.best_bid, snapshot.best_ask);
        if (mid_price > 0) {
            total_price += mid_price;
            valid_prices++;
        }
    }
    
    return valid_prices > 0 ? total_price / valid_prices : 0.0;
}

double calculate_realized_volatility(const std::vector<MarketSnapshot>& snapshots) {
    if (snapshots.size() < 2) {
        return 0.0;
    }
    
    std::vector<double> returns;
    for (size_t i = 1; i < snapshots.size(); ++i) {
        Price mid_prev = get_mid_price(snapshots[i-1].best_bid, snapshots[i-1].best_ask);
        Price mid_curr = get_mid_price(snapshots[i].best_bid, snapshots[i].best_ask);
        
        if (mid_prev > 0 && mid_curr > 0) {
            double ret = std::log(mid_curr / mid_prev);
            returns.push_back(ret);
        }
    }
    
    if (returns.empty()) {
        return 0.0;
    }
    
    double mean = 0.0;
    for (double ret : returns) {
        mean += ret;
    }
    mean /= returns.size();
    
    double variance = 0.0;
    for (double ret : returns) {
        variance += (ret - mean) * (ret - mean);
    }
    variance /= returns.size();
    
    return std::sqrt(variance);
}

SpreadStats calculate_spread_stats(const std::vector<MarketSnapshot>& snapshots) {
    SpreadStats stats = {};
    
    if (snapshots.empty()) {
        return stats;
    }
    
    std::vector<double> spreads;
    for (const auto& snapshot : snapshots) {
        if (snapshot.best_bid > 0 && snapshot.best_ask > 0) {
            spreads.push_back(snapshot.best_ask - snapshot.best_bid);
        }
    }
    
    if (spreads.empty()) {
        return stats;
    }
    
    std::sort(spreads.begin(), spreads.end());
    
    stats.min_spread = spreads.front();
    stats.max_spread = spreads.back();
    stats.median_spread = spreads[spreads.size() / 2];
    
    double total_spread = 0.0;
    for (double spread : spreads) {
        total_spread += spread;
    }
    stats.mean_spread = total_spread / spreads.size();
    
    double variance = 0.0;
    for (double spread : spreads) {
        variance += (spread - stats.mean_spread) * (spread - stats.mean_spread);
    }
    stats.spread_volatility = std::sqrt(variance / spreads.size());
    
    return stats;
}

std::vector<AgentPerformance> calculate_agent_performance(
    const std::vector<std::tuple<OrderId, Timestamp, double, Qty>>& agent_pnl,
    const std::vector<Trade>& trades
) {
    std::vector<AgentPerformance> performance;
    
    // Group PnL by agent
    std::unordered_map<OrderId, std::vector<double>> agent_pnl_history;
    for (const auto& pnl_record : agent_pnl) {
        agent_pnl_history[std::get<0>(pnl_record)].push_back(std::get<2>(pnl_record));
    }
    
    // Calculate performance for each agent
    for (const auto& [agent_id, pnl_history] : agent_pnl_history) {
        if (pnl_history.empty()) {
            continue;
        }
        
        AgentPerformance perf;
        perf.agent_id = agent_id;
        perf.total_pnl = pnl_history.back();
        perf.final_inventory = 0; // Would need to track this separately
        
        // Calculate Sharpe ratio (simplified)
        if (pnl_history.size() > 1) {
            double mean_return = 0.0;
            for (size_t i = 1; i < pnl_history.size(); ++i) {
                mean_return += (pnl_history[i] - pnl_history[i-1]);
            }
            mean_return /= (pnl_history.size() - 1);
            
            double variance = 0.0;
            for (size_t i = 1; i < pnl_history.size(); ++i) {
                double ret = pnl_history[i] - pnl_history[i-1];
                variance += (ret - mean_return) * (ret - mean_return);
            }
            variance /= (pnl_history.size() - 1);
            
            perf.sharpe_ratio = variance > 0 ? mean_return / std::sqrt(variance) : 0.0;
        }
        
        // Calculate max drawdown
        double peak = pnl_history[0];
        double max_dd = 0.0;
        for (double pnl : pnl_history) {
            if (pnl > peak) {
                peak = pnl;
            }
            double dd = peak - pnl;
            if (dd > max_dd) {
                max_dd = dd;
            }
        }
        perf.max_drawdown = max_dd;
        
        // Count trades for this agent
        perf.total_trades = 0;
        for (const auto& trade : trades) {
            if (trade.maker_id == agent_id || trade.taker_id == agent_id) {
                perf.total_trades++;
            }
        }
        
        perf.win_rate = 0.5; // Simplified - would need more detailed tracking
        
        performance.push_back(perf);
    }
    
    return performance;
}

} // namespace analysis

} // namespace mms
