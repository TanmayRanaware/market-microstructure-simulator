#include <gtest/gtest.h>
#include "mms/order_book.hpp"
#include "mms/types.hpp"

namespace mms {

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        book.clear();
    }
    
    OrderBook book;
};

TEST_F(OrderBookTest, EmptyBookInitialization) {
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.size(), 0);
    EXPECT_FALSE(book.best_bid_price().has_value());
    EXPECT_FALSE(book.best_ask_price().has_value());
}

TEST_F(OrderBookTest, AddSingleBuyOrder) {
    Order order(1, Side::BUY, 10000, 100, 1000);
    
    EXPECT_TRUE(book.add_limit_order(order));
    EXPECT_FALSE(book.empty());
    EXPECT_EQ(book.size(), 1);
    
    auto best_bid = book.best_bid_price();
    EXPECT_TRUE(best_bid.has_value());
    EXPECT_EQ(best_bid.value(), 10000);
    
    auto best_bid_qty = book.best_bid_quantity();
    EXPECT_TRUE(best_bid_qty.has_value());
    EXPECT_EQ(best_bid_qty.value(), 100);
}

TEST_F(OrderBookTest, AddSingleSellOrder) {
    Order order(2, Side::SELL, 10002, 50, 1001);
    
    EXPECT_TRUE(book.add_limit_order(order));
    EXPECT_FALSE(book.empty());
    EXPECT_EQ(book.size(), 1);
    
    auto best_ask = book.best_ask_price();
    EXPECT_TRUE(best_ask.has_value());
    EXPECT_EQ(best_ask.value(), 10002);
    
    auto best_ask_qty = book.best_ask_quantity();
    EXPECT_TRUE(best_ask_qty.has_value());
    EXPECT_EQ(best_ask_qty.value(), 50);
}

TEST_F(OrderBookTest, AddMultipleOrdersSamePrice) {
    Order order1(1, Side::BUY, 10000, 100, 1000);
    Order order2(2, Side::BUY, 10000, 200, 1001);
    
    EXPECT_TRUE(book.add_limit_order(order1));
    EXPECT_TRUE(book.add_limit_order(order2));
    EXPECT_EQ(book.size(), 2);
    
    auto best_bid_qty = book.best_bid_quantity();
    EXPECT_TRUE(best_bid_qty.has_value());
    EXPECT_EQ(best_bid_qty.value(), 300); // 100 + 200
}

TEST_F(OrderBookTest, PriceTimePriority) {
    Order order1(1, Side::BUY, 10000, 100, 1000);
    Order order2(2, Side::BUY, 10001, 200, 1001); // Higher price
    Order order3(3, Side::BUY, 10000, 50, 1002);  // Same price, later time
    
    EXPECT_TRUE(book.add_limit_order(order1));
    EXPECT_TRUE(book.add_limit_order(order2));
    EXPECT_TRUE(book.add_limit_order(order3));
    
    // Best bid should be highest price
    auto best_bid = book.best_bid_price();
    EXPECT_TRUE(best_bid.has_value());
    EXPECT_EQ(best_bid.value(), 10001);
    
    // Quantity at 10001 should be 200
    auto best_bid_qty = book.best_bid_quantity();
    EXPECT_TRUE(best_bid_qty.has_value());
    EXPECT_EQ(best_bid_qty.value(), 200);
}

TEST_F(OrderBookTest, MarketOrderExecution) {
    // Add some limit orders
    Order buy_order(1, Side::BUY, 10000, 100, 1000);
    Order sell_order(2, Side::SELL, 10002, 50, 1001);
    
    EXPECT_TRUE(book.add_limit_order(buy_order));
    EXPECT_TRUE(book.add_limit_order(sell_order));
    
    // Execute market buy order
    auto trades = book.add_market_order(Side::BUY, 30, 3, 1002);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 10002);
    EXPECT_EQ(trades[0].quantity, 30);
    EXPECT_EQ(trades[0].maker_id, 2);
    EXPECT_EQ(trades[0].taker_id, 3);
    
    // Check remaining quantity in sell order
    auto remaining_ask_qty = book.best_ask_quantity();
    EXPECT_TRUE(remaining_ask_qty.has_value());
    EXPECT_EQ(remaining_ask_qty.value(), 20); // 50 - 30
}

TEST_F(OrderBookTest, PartialFill) {
    Order sell_order(1, Side::SELL, 10002, 100, 1000);
    EXPECT_TRUE(book.add_limit_order(sell_order));
    
    // Market buy for more than available
    auto trades = book.add_market_order(Side::BUY, 150, 2, 1001);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 100); // Only 100 available
    EXPECT_EQ(trades[0].price, 10002);
    
    // Book should be empty now
    EXPECT_TRUE(book.empty());
}

TEST_F(OrderBookTest, OrderCancellation) {
    Order order(1, Side::BUY, 10000, 100, 1000);
    EXPECT_TRUE(book.add_limit_order(order));
    EXPECT_EQ(book.size(), 1);
    
    // Cancel the order
    EXPECT_TRUE(book.cancel_order(1));
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.size(), 0);
    
    // Try to cancel non-existent order
    EXPECT_FALSE(book.cancel_order(999));
}

TEST_F(OrderBookTest, InvalidOrders) {
    // Invalid price
    Order invalid_price(1, Side::BUY, 0, 100, 1000);
    EXPECT_FALSE(book.add_limit_order(invalid_price));
    
    // Invalid quantity
    Order invalid_qty(2, Side::BUY, 10000, 0, 1001);
    EXPECT_FALSE(book.add_limit_order(invalid_qty));
    
    EXPECT_TRUE(book.empty());
}

TEST_F(OrderBookTest, TopOfBook) {
    Order buy_order(1, Side::BUY, 10000, 100, 1000);
    Order sell_order(2, Side::SELL, 10002, 50, 1001);
    
    EXPECT_TRUE(book.add_limit_order(buy_order));
    EXPECT_TRUE(book.add_limit_order(sell_order));
    
    auto snapshot = book.top_of_book(1002);
    EXPECT_EQ(snapshot.best_bid, 10000);
    EXPECT_EQ(snapshot.best_ask, 10002);
    EXPECT_EQ(snapshot.best_bid_qty, 100);
    EXPECT_EQ(snapshot.best_ask_qty, 50);
    EXPECT_EQ(snapshot.timestamp, 1002);
}

TEST_F(OrderBookTest, DepthSnapshot) {
    // Add multiple price levels
    Order buy1(1, Side::BUY, 10000, 100, 1000);
    Order buy2(2, Side::BUY, 9999, 200, 1001);
    Order sell1(3, Side::SELL, 10002, 50, 1002);
    Order sell2(4, Side::SELL, 10003, 75, 1003);
    
    EXPECT_TRUE(book.add_limit_order(buy1));
    EXPECT_TRUE(book.add_limit_order(buy2));
    EXPECT_TRUE(book.add_limit_order(sell1));
    EXPECT_TRUE(book.add_limit_order(sell2));
    
    auto depth = book.get_depth(2);
    
    // Should have 2 levels for each side
    EXPECT_EQ(depth.size(), 4);
    
    // Check that prices are in correct order
    // Buy levels should be in descending order
    EXPECT_EQ(depth[0].price, 10000); // Best bid
    EXPECT_EQ(depth[1].price, 9999);  // Second best bid
    
    // Sell levels should be in ascending order
    EXPECT_EQ(depth[2].price, 10002); // Best ask
    EXPECT_EQ(depth[3].price, 10003); // Second best ask
}

TEST_F(OrderBookTest, TradeStatistics) {
    Order sell_order(1, Side::SELL, 10002, 100, 1000);
    EXPECT_TRUE(book.add_limit_order(sell_order));
    
    // Execute trade
    auto trades = book.add_market_order(Side::BUY, 50, 2, 1001);
    
    EXPECT_EQ(book.trade_count(), 1);
    EXPECT_EQ(book.total_volume(), 50);
    EXPECT_EQ(book.last_trade_price(), 10002);
}

TEST_F(OrderBookTest, OrderRetrieval) {
    Order order(123, Side::BUY, 10000, 100, 1000);
    EXPECT_TRUE(book.add_limit_order(order));
    
    auto retrieved = book.get_order(123);
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, 123);
    EXPECT_EQ(retrieved->price, 10000);
    EXPECT_EQ(retrieved->quantity, 100);
    
    // Non-existent order
    auto not_found = book.get_order(999);
    EXPECT_FALSE(not_found.has_value());
}

TEST_F(OrderBookTest, ClearBook) {
    Order order(1, Side::BUY, 10000, 100, 1000);
    EXPECT_TRUE(book.add_limit_order(order));
    EXPECT_FALSE(book.empty());
    
    book.clear();
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.size(), 0);
    EXPECT_EQ(book.trade_count(), 0);
    EXPECT_EQ(book.total_volume(), 0);
}

TEST_F(OrderBookTest, CrossingOrders) {
    // Add buy order
    Order buy_order(1, Side::BUY, 10000, 100, 1000);
    EXPECT_TRUE(book.add_limit_order(buy_order));
    
    // Add crossing sell order (should execute immediately)
    Order crossing_sell(2, Side::SELL, 9999, 50, 1001);
    EXPECT_TRUE(book.add_limit_order(crossing_sell));
    
    // The crossing order should have been executed
    // Check that we have a trade recorded
    EXPECT_GT(book.trade_count(), 0);
    EXPECT_GT(book.total_volume(), 0);
}

} // namespace mms
