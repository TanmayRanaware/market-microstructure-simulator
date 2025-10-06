#include <gtest/gtest.h>
#include "mms/matching_engine.hpp"
#include "mms/types.hpp"

namespace mms {

class MatchingEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine.clear();
    }
    
    MatchingEngine engine;
};

TEST_F(MatchingEngineTest, ProcessLimitOrder) {
    Event event(EventType::LIMIT, 1, Side::BUY, 10000, 100, 1000, 1);
    
    auto trades = engine.process_event(event);
    
    // Should have no trades initially (order just added to book)
    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(engine.order_count(), 1);
}

TEST_F(MatchingEngineTest, ProcessMarketOrder) {
    // First add a limit order
    Event limit_event(EventType::LIMIT, 1, Side::SELL, 10002, 50, 1000, 1);
    engine.process_event(limit_event);
    
    // Then process market order that should match
    Event market_event(EventType::MARKET, 2, Side::BUY, 0, 30, 1001, 2);
    auto trades = engine.process_event(market_event);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 10002);
    EXPECT_EQ(trades[0].quantity, 30);
    EXPECT_EQ(trades[0].maker_id, 1);
    EXPECT_EQ(trades[0].taker_id, 2);
}

TEST_F(MatchingEngineTest, ProcessCancelOrder) {
    // Add a limit order
    Event limit_event(EventType::LIMIT, 1, Side::BUY, 10000, 100, 1000, 1);
    engine.process_event(limit_event);
    
    EXPECT_EQ(engine.order_count(), 1);
    
    // Cancel the order
    Event cancel_event(EventType::CANCEL, 1, Side::BUY, 0, 0, 1001, 1);
    auto trades = engine.process_event(cancel_event);
    
    EXPECT_EQ(trades.size(), 0); // Cancels don't generate trades
    EXPECT_EQ(engine.order_count(), 0);
}

TEST_F(MatchingEngineTest, ProcessMultipleEvents) {
    std::vector<Event> events = {
        Event(EventType::LIMIT, 1, Side::BUY, 10000, 100, 1000, 1),
        Event(EventType::LIMIT, 2, Side::SELL, 10002, 50, 1001, 2),
        Event(EventType::MARKET, 3, Side::BUY, 0, 30, 1002, 3)
    };
    
    auto trades = engine.process_events(events);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(engine.order_count(), 2); // 1 buy + 1 sell remaining
}

TEST_F(MatchingEngineTest, CrossOrderExecution) {
    // Add a sell order
    Event sell_event(EventType::LIMIT, 1, Side::SELL, 10000, 100, 1000, 1);
    engine.process_event(sell_event);
    
    // Add a crossing buy order (should execute immediately)
    Event buy_event(EventType::LIMIT, 2, Side::BUY, 10001, 50, 1001, 2);
    auto trades = engine.process_event(buy_event);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 10000); // Executes at the existing order's price
    EXPECT_EQ(trades[0].quantity, 50);
}

TEST_F(MatchingEngineTest, PartialFill) {
    // Add a sell order
    Event sell_event(EventType::LIMIT, 1, Side::SELL, 10002, 100, 1000, 1);
    engine.process_event(sell_event);
    
    // Market buy for more than available
    Event market_event(EventType::MARKET, 2, Side::BUY, 0, 150, 1001, 2);
    auto trades = engine.process_event(market_event);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 100); // Only 100 available
    EXPECT_EQ(engine.order_count(), 0); // Sell order fully consumed
}

TEST_F(MatchingEngineTest, GetMarketSnapshot) {
    // Add some orders
    Event buy_event(EventType::LIMIT, 1, Side::BUY, 10000, 100, 1000, 1);
    Event sell_event(EventType::LIMIT, 2, Side::SELL, 10002, 50, 1001, 2);
    
    engine.process_event(buy_event);
    engine.process_event(sell_event);
    
    auto snapshot = engine.get_market_snapshot(1002);
    
    EXPECT_EQ(snapshot.best_bid, 10000);
    EXPECT_EQ(snapshot.best_ask, 10002);
    EXPECT_EQ(snapshot.best_bid_qty, 100);
    EXPECT_EQ(snapshot.best_ask_qty, 50);
    EXPECT_EQ(snapshot.timestamp, 1002);
}

TEST_F(MatchingEngineTest, GetDepth) {
    // Add multiple price levels
    Event buy1(EventType::LIMIT, 1, Side::BUY, 10000, 100, 1000, 1);
    Event buy2(EventType::LIMIT, 2, Side::BUY, 9999, 200, 1001, 1);
    Event sell1(EventType::LIMIT, 3, Side::SELL, 10002, 50, 1002, 2);
    Event sell2(EventType::LIMIT, 4, Side::SELL, 10003, 75, 1003, 2);
    
    engine.process_event(buy1);
    engine.process_event(buy2);
    engine.process_event(sell1);
    engine.process_event(sell2);
    
    auto depth = engine.get_depth(2);
    
    EXPECT_EQ(depth.size(), 4); // 2 buy levels + 2 sell levels
}

TEST_F(MatchingEngineTest, Statistics) {
    EXPECT_EQ(engine.order_count(), 0);
    EXPECT_EQ(engine.last_trade_price(), 0);
    EXPECT_EQ(engine.total_volume(), 0);
    EXPECT_EQ(engine.trade_count(), 0);
    
    // Add and execute a trade
    Event sell_event(EventType::LIMIT, 1, Side::SELL, 10002, 50, 1000, 1);
    Event market_event(EventType::MARKET, 2, Side::BUY, 0, 30, 1001, 2);
    
    engine.process_event(sell_event);
    auto trades = engine.process_event(market_event);
    
    if (!trades.empty()) {
        EXPECT_GT(engine.trade_count(), 0);
        EXPECT_GT(engine.total_volume(), 0);
        EXPECT_GT(engine.last_trade_price(), 0);
    }
}

TEST_F(MatchingEngineTest, ClearEngine) {
    // Add some orders and execute trades
    Event sell_event(EventType::LIMIT, 1, Side::SELL, 10002, 50, 1000, 1);
    Event market_event(EventType::MARKET, 2, Side::BUY, 0, 30, 1001, 2);
    
    engine.process_event(sell_event);
    engine.process_event(market_event);
    
    // Clear and verify reset
    engine.clear();
    
    EXPECT_EQ(engine.order_count(), 0);
    EXPECT_EQ(engine.last_trade_price(), 0);
    EXPECT_EQ(engine.total_volume(), 0);
    EXPECT_EQ(engine.trade_count(), 0);
}

} // namespace mms
