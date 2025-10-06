#include <gtest/gtest.h>
#include "mms/types.hpp"

namespace mms {

class TypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TypesTest, OrderCreation) {
    Order order(1, Side::BUY, 10000, 100, 1000);
    
    EXPECT_EQ(order.id, 1);
    EXPECT_EQ(order.side, Side::BUY);
    EXPECT_EQ(order.price, 10000);
    EXPECT_EQ(order.quantity, 100);
    EXPECT_EQ(order.timestamp, 1000);
}

TEST_F(TypesTest, TradeCreation) {
    Trade trade(1, 2, 10000, 50, 1001);
    
    EXPECT_EQ(trade.maker_id, 1);
    EXPECT_EQ(trade.taker_id, 2);
    EXPECT_EQ(trade.price, 10000);
    EXPECT_EQ(trade.quantity, 50);
    EXPECT_EQ(trade.timestamp, 1001);
}

TEST_F(TypesTest, MarketSnapshotCreation) {
    MarketSnapshot snapshot(9999, 10001, 100, 50, 10000, 1002);
    
    EXPECT_EQ(snapshot.best_bid, 9999);
    EXPECT_EQ(snapshot.best_ask, 10001);
    EXPECT_EQ(snapshot.best_bid_qty, 100);
    EXPECT_EQ(snapshot.best_ask_qty, 50);
    EXPECT_EQ(snapshot.last_trade_price, 10000);
    EXPECT_EQ(snapshot.timestamp, 1002);
}

TEST_F(TypesTest, HelperFunctions) {
    EXPECT_STREQ(side_to_string(Side::BUY), "BUY");
    EXPECT_STREQ(side_to_string(Side::SELL), "SELL");
    
    EXPECT_STREQ(event_type_to_string(EventType::LIMIT), "LIMIT");
    EXPECT_STREQ(event_type_to_string(EventType::MARKET), "MARKET");
    EXPECT_STREQ(event_type_to_string(EventType::CANCEL), "CANCEL");
    
    EXPECT_TRUE(is_valid_price(10000));
    EXPECT_FALSE(is_valid_price(0));
    EXPECT_FALSE(is_valid_price(-100));
    
    EXPECT_TRUE(is_valid_quantity(100));
    EXPECT_FALSE(is_valid_quantity(0));
    EXPECT_FALSE(is_valid_quantity(-50));
    
    EXPECT_EQ(get_mid_price(9999, 10001), 10000);
    EXPECT_EQ(get_mid_price(0, 10001), 0);
    EXPECT_EQ(get_mid_price(9999, 0), 0);
    
    EXPECT_EQ(get_spread(9999, 10001), 2);
    EXPECT_EQ(get_spread(0, 10001), 0);
    EXPECT_EQ(get_spread(9999, 0), 0);
}

} // namespace mms
