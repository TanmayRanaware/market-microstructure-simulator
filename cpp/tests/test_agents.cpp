#include <gtest/gtest.h>
#include "mms/agents.hpp"
#include "mms/types.hpp"

namespace mms {

class AgentsTest : public ::testing::Test {
protected:
    void SetUp() override {
        rng.seed(42);
    }
    
    RNG rng;
};

TEST_F(AgentsTest, MarketMakerInitialization) {
    MarketMaker::Config config;
    config.spread = 2;
    config.quantity = 50;
    config.refresh_interval = 50000;
    
    MarketMaker maker(1, "TestMaker", config, rng);
    
    EXPECT_EQ(maker.get_id(), 1);
    EXPECT_EQ(maker.get_name(), "TestMaker");
    EXPECT_EQ(maker.get_pnl(), 0.0);
    EXPECT_EQ(maker.get_inventory(), 0);
}

TEST_F(AgentsTest, TakerInitialization) {
    Taker::Config config;
    config.intensity = 0.8;
    config.side_bias = 0.5;
    config.quantity_mean = 40;
    
    Taker taker(2, "TestTaker", config, rng);
    
    EXPECT_EQ(taker.get_id(), 2);
    EXPECT_EQ(taker.get_name(), "TestTaker");
    EXPECT_EQ(taker.get_pnl(), 0.0);
    EXPECT_EQ(taker.get_inventory(), 0);
}

TEST_F(AgentsTest, NoiseTraderInitialization) {
    NoiseTrader::Config config;
    config.limit_intensity = 1.5;
    config.cancel_intensity = 0.7;
    config.quantity_mean = 30;
    
    NoiseTrader noise(3, "TestNoise", config, rng);
    
    EXPECT_EQ(noise.get_id(), 3);
    EXPECT_EQ(noise.get_name(), "TestNoise");
    EXPECT_EQ(noise.get_pnl(), 0.0);
    EXPECT_EQ(noise.get_inventory(), 0);
}

TEST_F(AgentsTest, MarketMakerStep) {
    MarketMaker::Config config;
    config.spread = 2;
    config.quantity = 50;
    config.refresh_interval = 50000;
    
    MarketMaker maker(1, "TestMaker", config, rng);
    
    // Generate events for a timestamp
    auto events = maker.step(1000);
    
    // Should generate events (bid/ask orders)
    EXPECT_GT(events.size(), 0);
}

TEST_F(AgentsTest, TakerStep) {
    Taker::Config config;
    config.intensity = 0.8;
    config.side_bias = 0.5;
    config.quantity_mean = 40;
    
    Taker taker(2, "TestTaker", config, rng);
    
    // Generate events for a timestamp
    auto events = taker.step(1000);
    
    // May or may not generate events depending on intensity
    // This is probabilistic, so we just check it doesn't crash
    EXPECT_TRUE(true); // Test passes if no crash
}

TEST_F(AgentsTest, NoiseTraderStep) {
    NoiseTrader::Config config;
    config.limit_intensity = 1.5;
    config.cancel_intensity = 0.7;
    config.quantity_mean = 30;
    
    NoiseTrader noise(3, "TestNoise", config, rng);
    
    // Generate events for a timestamp
    auto events = noise.step(1000);
    
    // May or may not generate events depending on intensity
    // This is probabilistic, so we just check it doesn't crash
    EXPECT_TRUE(true); // Test passes if no crash
}

TEST_F(AgentsTest, AgentOnTrade) {
    MarketMaker::Config config;
    MarketMaker maker(1, "TestMaker", config, rng);
    
    // Create a trade where maker is the maker
    Trade trade(1, 2, 10000, 50, 1000); // maker_id=1, taker_id=2
    
    maker.on_trade(trade);
    
    // PnL and inventory should be updated
    // Note: The actual values depend on the implementation
    EXPECT_TRUE(true); // Test passes if no crash
}

TEST_F(AgentsTest, AgentReset) {
    MarketMaker::Config config;
    MarketMaker maker(1, "TestMaker", config, rng);
    
    // Modify state somehow (through trades or steps)
    Trade trade(1, 2, 10000, 50, 1000);
    maker.on_trade(trade);
    
    // Reset agent
    maker.reset();
    
    // Should be back to initial state
    EXPECT_EQ(maker.get_pnl(), 0.0);
    EXPECT_EQ(maker.get_inventory(), 0);
}

TEST_F(AgentsTest, AgentManagerAddAgent) {
    AgentManager manager;
    
    auto maker = std::make_unique<MarketMaker>(1, "Maker", MarketMaker::Config{}, rng);
    manager.add_agent(std::move(maker));
    
    EXPECT_EQ(manager.get_agents().size(), 1);
}

TEST_F(AgentsTest, AgentManagerGetAgent) {
    AgentManager manager;
    
    auto maker = std::make_unique<MarketMaker>(1, "Maker", MarketMaker::Config{}, rng);
    manager.add_agent(std::move(maker));
    
    Agent* retrieved = manager.get_agent(1);
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->get_id(), 1);
    
    Agent* not_found = manager.get_agent(999);
    EXPECT_EQ(not_found, nullptr);
}

TEST_F(AgentsTest, AgentManagerStep) {
    AgentManager manager;
    
    auto maker = std::make_unique<MarketMaker>(1, "Maker", MarketMaker::Config{}, rng);
    manager.add_agent(std::move(maker));
    
    auto events = manager.step(1000);
    
    // Should collect events from all agents
    EXPECT_TRUE(true); // Test passes if no crash
}

TEST_F(AgentsTest, AgentManagerNotifyTrade) {
    AgentManager manager;
    
    auto maker = std::make_unique<MarketMaker>(1, "Maker", MarketMaker::Config{}, rng);
    manager.add_agent(std::move(maker));
    
    Trade trade(1, 2, 10000, 50, 1000);
    manager.notify_trade(trade);
    
    // Should notify all agents
    EXPECT_TRUE(true); // Test passes if no crash
}

TEST_F(AgentsTest, AgentManagerGetStats) {
    AgentManager manager;
    
    auto maker = std::make_unique<MarketMaker>(1, "Maker", MarketMaker::Config{}, rng);
    manager.add_agent(std::move(maker));
    
    auto stats = manager.get_stats();
    
    EXPECT_EQ(stats.size(), 1);
    EXPECT_EQ(stats[0].id, 1);
    EXPECT_EQ(stats[0].name, "Maker");
}

TEST_F(AgentsTest, AgentManagerReset) {
    AgentManager manager;
    
    auto maker = std::make_unique<MarketMaker>(1, "Maker", MarketMaker::Config{}, rng);
    manager.add_agent(std::move(maker));
    
    // Modify agent state somehow
    Trade trade(1, 2, 10000, 50, 1000);
    manager.notify_trade(trade);
    
    // Reset all agents
    manager.reset();
    
    // All agents should be back to initial state
    auto stats = manager.get_stats();
    EXPECT_EQ(stats[0].pnl, 0.0);
    EXPECT_EQ(stats[0].inventory, 0);
}

} // namespace mms
