#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include "mms/simulator.hpp"

int main(int argc, char* argv[]) {
    std::cout << "Market Microstructure Simulator - Benchmark" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    // Parse command line arguments
    size_t n_steps = 100000;
    size_t iterations = 5;
    uint64_t seed = 42;
    
    if (argc > 1) {
        n_steps = std::stoul(argv[1]);
    }
    if (argc > 2) {
        iterations = std::stoul(argv[2]);
    }
    if (argc > 3) {
        seed = std::stoull(argv[3]);
    }
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Steps: " << n_steps << std::endl;
    std::cout << "  Iterations: " << iterations << std::endl;
    std::cout << "  Seed: " << seed << std::endl;
    std::cout << std::endl;
    
    // Create agent configurations
    mms::MarketMaker::Config maker_config;
    maker_config.spread = 2;
    maker_config.quantity = 50;
    maker_config.refresh_interval = 50000;
    maker_config.max_inventory = 1000;
    maker_config.inventory_penalty = 0.001;
    
    mms::Taker::Config taker_config;
    taker_config.intensity = 0.8;
    taker_config.side_bias = 0.5;
    taker_config.quantity_mean = 40;
    taker_config.quantity_std = 10;
    taker_config.use_market_orders = true;
    
    mms::NoiseTrader::Config noise_config;
    noise_config.limit_intensity = 1.5;
    noise_config.cancel_intensity = 0.7;
    noise_config.quantity_mean = 30;
    noise_config.quantity_std = 8;
    noise_config.price_volatility = 5;
    noise_config.cancel_probability = 0.3;
    
    std::vector<double> execution_times;
    std::vector<size_t> events_per_second;
    std::vector<size_t> total_trades;
    
    std::cout << "Running benchmark..." << std::endl;
    
    for (size_t i = 0; i < iterations; ++i) {
        std::cout << "Iteration " << (i + 1) << "/" << iterations << "..." << std::flush;
        
        // Create simulation configuration
        mms::SimulationConfig config;
        config.seed = seed + i;  // Different seed for each iteration
        config.start_time = 0;
        config.time_step = 1000;
        config.max_steps = n_steps;
        config.enable_logging = false;
        
        // Create simulator
        mms::Simulator simulator(config);
        
        // Run simulation and measure time
        auto start_time = std::chrono::high_resolution_clock::now();
        
        auto result = simulator.run(
            n_steps,
            maker_config,
            taker_config,
            noise_config
        );
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        double execution_time = duration.count() / 1e6;  // Convert to seconds
        size_t eps = result.total_events_processed / execution_time;
        
        execution_times.push_back(execution_time);
        events_per_second.push_back(eps);
        total_trades.push_back(result.total_trades);
        
        std::cout << " " << execution_time << "s (" << eps << " events/s)" << std::endl;
    }
    
    // Calculate statistics
    auto calc_mean = [](const std::vector<double>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    };
    
    auto calc_mean_size_t = [](const std::vector<size_t>& v) {
        return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    };
    
    auto calc_std = [](const std::vector<double>& v, double mean) {
        double variance = 0.0;
        for (double x : v) {
            variance += (x - mean) * (x - mean);
        }
        return std::sqrt(variance / v.size());
    };
    
    auto calc_std_size_t = [](const std::vector<size_t>& v, double mean) {
        double variance = 0.0;
        for (size_t x : v) {
            variance += (x - mean) * (x - mean);
        }
        return std::sqrt(variance / v.size());
    };
    
    double mean_time = calc_mean(execution_times);
    double std_time = calc_std(execution_times, mean_time);
    double mean_eps = calc_mean_size_t(events_per_second);
    double std_eps = calc_std_size_t(events_per_second, mean_eps);
    double mean_trades = calc_mean_size_t(total_trades);
    double std_trades = calc_std_size_t(total_trades, mean_trades);
    
    // Print results
    std::cout << "\nBenchmark Results:" << std::endl;
    std::cout << "==================" << std::endl;
    std::cout << "Execution Time (seconds):" << std::endl;
    std::cout << "  Mean: " << mean_time << std::endl;
    std::cout << "  Std:  " << std_time << std::endl;
    std::cout << "  Min:  " << *std::min_element(execution_times.begin(), execution_times.end()) << std::endl;
    std::cout << "  Max:  " << *std::max_element(execution_times.begin(), execution_times.end()) << std::endl;
    
    std::cout << "\nEvents per Second:" << std::endl;
    std::cout << "  Mean: " << mean_eps << std::endl;
    std::cout << "  Std:  " << std_eps << std::endl;
    std::cout << "  Min:  " << *std::min_element(events_per_second.begin(), events_per_second.end()) << std::endl;
    std::cout << "  Max:  " << *std::max_element(events_per_second.begin(), events_per_second.end()) << std::endl;
    
    std::cout << "\nTotal Trades:" << std::endl;
    std::cout << "  Mean: " << mean_trades << std::endl;
    std::cout << "  Std:  " << std_trades << std::endl;
    std::cout << "  Min:  " << *std::min_element(total_trades.begin(), total_trades.end()) << std::endl;
    std::cout << "  Max:  " << *std::max_element(total_trades.begin(), total_trades.end()) << std::endl;
    
    // Performance summary
    std::cout << "\nPerformance Summary:" << std::endl;
    std::cout << "===================" << std::endl;
    std::cout << "Average throughput: " << static_cast<int>(mean_eps) << " events/second" << std::endl;
    std::cout << "Average execution time: " << mean_time << " seconds" << std::endl;
    std::cout << "Average trades per run: " << static_cast<int>(mean_trades) << std::endl;
    
    if (mean_time > 0) {
        std::cout << "Scalability: " << (n_steps / mean_time) << " steps/second" << std::endl;
    }
    
    std::cout << "\nBenchmark completed!" << std::endl;
    
    return 0;
}
