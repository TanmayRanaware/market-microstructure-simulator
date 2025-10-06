#include <iostream>
#include <chrono>
#include "mms/simulator.hpp"

int main() {
    std::cout << "Market Microstructure Simulator - Simple Example" << std::endl;
    std::cout << "================================================" << std::endl;
    
    // Create simulation configuration
    mms::SimulationConfig config;
    config.seed = 42;
    config.start_time = 0;
    config.time_step = 1000;
    config.max_steps = 100000;
    config.enable_logging = false;
    
    // Create simulator
    mms::Simulator simulator(config);
    
    // Configure agents
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
    
    std::cout << "Running simulation..." << std::endl;
    
    // Run simulation
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto result = simulator.run(
        100000,  // n_steps
        maker_config,
        taker_config,
        noise_config
    );
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Print results
    std::cout << "\nSimulation Results:" << std::endl;
    std::cout << "------------------" << std::endl;
    std::cout << "Total Events Processed: " << result.total_events_processed << std::endl;
    std::cout << "Total Trades: " << result.total_trades << std::endl;
    std::cout << "Simulation Duration: " << result.simulation_duration << " ns" << std::endl;
    std::cout << "Execution Time: " << duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "Events per Second: " << result.total_events_processed / (duration.count() / 1e6) << std::endl;
    
    // Print some trade statistics
    if (!result.trades.empty()) {
        std::cout << "\nTrade Statistics:" << std::endl;
        std::cout << "-----------------" << std::endl;
        
        // Calculate basic statistics
        int64_t total_volume = 0;
        int64_t min_price = result.trades[0].price;
        int64_t max_price = result.trades[0].price;
        double total_value = 0.0;
        
        for (const auto& trade : result.trades) {
            total_volume += trade.quantity;
            total_value += trade.price * trade.quantity;
            min_price = std::min(min_price, trade.price);
            max_price = std::max(max_price, trade.price);
        }
        
        double avg_price = total_value / total_volume;
        
        std::cout << "Average Trade Price: " << avg_price << std::endl;
        std::cout << "Price Range: " << min_price << " - " << max_price << std::endl;
        std::cout << "Total Volume: " << total_volume << std::endl;
    }
    
    // Print agent statistics
    if (!result.agent_pnl.empty()) {
        std::cout << "\nAgent Performance:" << std::endl;
        std::cout << "------------------" << std::endl;
        
        std::map<uint64_t, double> final_pnl;
        std::map<uint64_t, int64_t> final_inventory;
        
        for (const auto& [agent_id, timestamp, pnl, inventory] : result.agent_pnl) {
            final_pnl[agent_id] = pnl;
            final_inventory[agent_id] = inventory;
        }
        
        for (const auto& [agent_id, pnl] : final_pnl) {
            std::cout << "Agent " << agent_id << ": PnL=" << pnl 
                      << ", Inventory=" << final_inventory[agent_id] << std::endl;
        }
    }
    
    std::cout << "\nSimulation completed successfully!" << std::endl;
    
    return 0;
}
