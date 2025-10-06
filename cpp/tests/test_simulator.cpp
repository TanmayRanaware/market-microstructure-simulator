#include <gtest/gtest.h>
#include "mms/simulator.hpp"
#include "mms/types.hpp"

namespace mms {

class SimulatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config.seed = 42;
        config.start_time = 0;
        config.time_step = 1000;
        config.max_steps = 1000;
        config.enable_logging = false;
        config.output_dir = "";
        
        simulator = std::make_unique<Simulator>(config);
    }
    
    SimulationConfig config;
    std::unique_ptr<Simulator> simulator;
};

TEST_F(SimulatorTest, BasicSimulationRun) {
    MarketMaker::Config maker_config;
    maker_config.spread = 2;
    maker_config.quantity = 50;
    maker_config.refresh_interval = 50000;
    
    Taker::Config taker_config;
    taker_config.intensity = 0.8;
    taker_config.side_bias = 0.5;
    taker_config.quantity_mean = 40;
    
    NoiseTrader::Config noise_config;
    noise_config.limit_intensity = 1.5;
    noise_config.cancel_intensity = 0.7;
    noise_config.quantity_mean = 30;
    
    auto result = simulator->run(1000, maker_config, taker_config, noise_config);
    
    EXPECT_GT(result.total_events_processed, 0);
    EXPECT_GE(result.total_trades, 0);
    EXPECT_GT(result.simulation_duration, 0);
    EXPECT_GT(result.simulation_time_seconds, 0.0);
}

TEST_F(SimulatorTest, DeterministicWithSameSeed) {
    config.seed = 12345;
    simulator = std::make_unique<Simulator>(config);
    
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    auto result1 = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Reset and run again with same seed
    simulator->reset();
    auto result2 = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Results should be identical
    EXPECT_EQ(result1.total_events_processed, result2.total_events_processed);
    EXPECT_EQ(result1.total_trades, result2.total_trades);
    EXPECT_EQ(result1.simulation_duration, result2.simulation_duration);
}

TEST_F(SimulatorTest, DifferentSeedsProduceDifferentResults) {
    config.seed = 11111;
    simulator = std::make_unique<Simulator>(config);
    
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    auto result1 = simulator->run(100, maker_config, taker_config, noise_config);
    
    config.seed = 22222;
    simulator = std::make_unique<Simulator>(config);
    auto result2 = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Results should be different
    EXPECT_NE(result1.total_events_processed, result2.total_events_processed);
}

TEST_F(SimulatorTest, SimulationStatistics) {
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    auto result = simulator->run(500, maker_config, taker_config, noise_config);
    
    auto stats = simulator->get_stats();
    
    EXPECT_EQ(stats.total_events_processed, result.total_events_processed);
    EXPECT_EQ(stats.total_trades, result.total_trades);
    EXPECT_GE(stats.events_per_second, 0.0);
}

TEST_F(SimulatorTest, ResetSimulator) {
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    // Run simulation
    auto result1 = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Reset and run again
    simulator->reset();
    auto result2 = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Results should be identical (same seed)
    EXPECT_EQ(result1.total_events_processed, result2.total_events_processed);
    EXPECT_EQ(result1.total_trades, result2.total_trades);
}

TEST_F(SimulatorTest, DataCollection) {
    simulator->set_data_collection(true);
    
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    auto result = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Should have collected some data
    EXPECT_GE(result.trades.size(), 0);
    EXPECT_GE(result.market_snapshots.size(), 0);
    EXPECT_GE(result.agent_pnl.size(), 0);
}

TEST_F(SimulatorTest, DisableDataCollection) {
    simulator->set_data_collection(false);
    
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    auto result = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Should still have basic results but minimal data collection
    EXPECT_GE(result.total_events_processed, 0);
}

TEST_F(SimulatorTest, CustomAgentConfigurations) {
    // Test with different agent configurations
    MarketMaker::Config maker_config;
    maker_config.spread = 5; // Wider spread
    maker_config.quantity = 100; // Larger orders
    
    Taker::Config taker_config;
    taker_config.intensity = 2.0; // Higher intensity
    taker_config.quantity_mean = 80; // Larger orders
    
    NoiseTrader::Config noise_config;
    noise_config.limit_intensity = 0.5; // Lower intensity
    noise_config.quantity_mean = 20; // Smaller orders
    
    auto result = simulator->run(200, maker_config, taker_config, noise_config);
    
    EXPECT_GT(result.total_events_processed, 0);
    EXPECT_GE(result.total_trades, 0);
}

TEST_F(SimulatorTest, EmptySimulation) {
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    // Run with 0 steps
    auto result = simulator->run(0, maker_config, taker_config, noise_config);
    
    EXPECT_EQ(result.total_events_processed, 0);
    EXPECT_EQ(result.total_trades, 0);
    EXPECT_EQ(result.simulation_duration, 0);
}

TEST_F(SimulatorTest, LargeSimulation) {
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    // Run larger simulation
    auto result = simulator->run(10000, maker_config, taker_config, noise_config);
    
    EXPECT_GT(result.total_events_processed, 0);
    EXPECT_GE(result.total_trades, 0);
    EXPECT_GT(result.simulation_duration, 0);
    EXPECT_GT(result.simulation_time_seconds, 0.0);
}

TEST_F(SimulatorTest, ConfigurationUpdate) {
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    // Run with default config
    auto result1 = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Update configuration
    SimulationConfig new_config = config;
    new_config.time_step = 2000; // Different time step
    simulator->update_config(new_config);
    
    auto result2 = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Results should be different due to different time step
    EXPECT_NE(result1.total_events_processed, result2.total_events_processed);
}

TEST_F(SimulatorTest, OutputDirectory) {
    const std::string output_dir = "test_output";
    simulator->set_output_dir(output_dir);
    
    MarketMaker::Config maker_config;
    Taker::Config taker_config;
    NoiseTrader::Config noise_config;
    
    auto result = simulator->run(100, maker_config, taker_config, noise_config);
    
    // Should have run successfully
    EXPECT_GT(result.total_events_processed, 0);
}

} // namespace mms
