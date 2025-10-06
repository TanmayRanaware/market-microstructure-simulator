"""
Example trading strategies for the market microstructure simulator.
"""

import numpy as np
from typing import List, Dict, Any, Optional
from .core import Order, Trade, MarketSnapshot, Side, EventType


class BaseStrategy:
    """Base class for trading strategies."""
    
    def __init__(self, strategy_id: int, name: str):
        self.id = strategy_id
        self.name = name
        self.pnl = 0.0
        self.inventory = 0
        self.trades = []
    
    def update(self, timestamp: int, market_snapshot: MarketSnapshot) -> List[Order]:
        """Update strategy and return new orders."""
        raise NotImplementedError
    
    def on_trade(self, trade: Trade):
        """Handle trade execution."""
        self.trades.append(trade)
        # Update inventory and PnL based on trade
        if trade.taker_id == self.id:
            # We were the taker
            if trade.maker_id == self.id:
                # This shouldn't happen
                return
            
            # Update PnL (simplified)
            self.pnl -= trade.price * trade.quantity
            
            # Update inventory
            # This is a simplified calculation
            if trade.taker_id == self.id:
                self.inventory += trade.quantity  # Simplified
        else:
            # We were the maker
            self.pnl += trade.price * trade.quantity
            self.inventory -= trade.quantity  # Simplified
    
    def get_stats(self) -> Dict[str, Any]:
        """Get strategy performance statistics."""
        return {
            'id': self.id,
            'name': self.name,
            'pnl': self.pnl,
            'inventory': self.inventory,
            'total_trades': len(self.trades)
        }


class SimpleStrategy(BaseStrategy):
    """Simple market making strategy."""
    
    def __init__(self, strategy_id: int, name: str = "SimpleStrategy", 
                 spread: int = 2, quantity: int = 50):
        super().__init__(strategy_id, name)
        self.spread = spread
        self.quantity = quantity
        self.last_bid = 0
        self.last_ask = 0
    
    def update(self, timestamp: int, market_snapshot: MarketSnapshot) -> List[Order]:
        orders = []
        
        if market_snapshot.best_bid > 0 and market_snapshot.best_ask > 0:
            mid_price = (market_snapshot.best_bid + market_snapshot.best_ask) / 2
            
            # Place bid order
            bid_price = mid_price - self.spread // 2
            if bid_price != self.last_bid:
                orders.append(Order(
                    id=timestamp + self.id,
                    side=Side.BUY,
                    price=bid_price,
                    quantity=self.quantity,
                    timestamp=timestamp
                ))
                self.last_bid = bid_price
            
            # Place ask order
            ask_price = mid_price + self.spread // 2
            if ask_price != self.last_ask:
                orders.append(Order(
                    id=timestamp + self.id + 1000,
                    side=Side.SELL,
                    price=ask_price,
                    quantity=self.quantity,
                    timestamp=timestamp
                ))
                self.last_ask = ask_price
        
        return orders


class MeanReversionStrategy(BaseStrategy):
    """Mean reversion strategy based on price deviations."""
    
    def __init__(self, strategy_id: int, name: str = "MeanReversionStrategy",
                 lookback_period: int = 100, threshold: float = 0.02,
                 quantity: int = 30):
        super().__init__(strategy_id, name)
        self.lookback_period = lookback_period
        self.threshold = threshold
        self.quantity = quantity
        self.price_history = []
        self.last_action = None
    
    def update(self, timestamp: int, market_snapshot: MarketSnapshot) -> List[Order]:
        orders = []
        
        if market_snapshot.best_bid > 0 and market_snapshot.best_ask > 0:
            mid_price = (market_snapshot.best_bid + market_snapshot.best_ask) / 2
            self.price_history.append(mid_price)
            
            # Keep only recent prices
            if len(self.price_history) > self.lookback_period:
                self.price_history = self.price_history[-self.lookback_period:]
            
            if len(self.price_history) >= 10:
                mean_price = np.mean(self.price_history)
                current_deviation = (mid_price - mean_price) / mean_price
                
                # Buy if price is below mean (oversold)
                if current_deviation < -self.threshold and self.last_action != 'buy':
                    orders.append(Order(
                        id=timestamp + self.id,
                        side=Side.BUY,
                        price=market_snapshot.best_ask,  # Market buy
                        quantity=self.quantity,
                        timestamp=timestamp
                    ))
                    self.last_action = 'buy'
                
                # Sell if price is above mean (overbought)
                elif current_deviation > self.threshold and self.last_action != 'sell':
                    orders.append(Order(
                        id=timestamp + self.id + 1000,
                        side=Side.SELL,
                        price=market_snapshot.best_bid,  # Market sell
                        quantity=self.quantity,
                        timestamp=timestamp
                    ))
                    self.last_action = 'sell'
        
        return orders


class MomentumStrategy(BaseStrategy):
    """Momentum strategy based on price trends."""
    
    def __init__(self, strategy_id: int, name: str = "MomentumStrategy",
                 lookback_period: int = 50, momentum_threshold: float = 0.01,
                 quantity: int = 40):
        super().__init__(strategy_id, name)
        self.lookback_period = lookback_period
        self.momentum_threshold = momentum_threshold
        self.quantity = quantity
        self.price_history = []
        self.last_action = None
    
    def update(self, timestamp: int, market_snapshot: MarketSnapshot) -> List[Order]:
        orders = []
        
        if market_snapshot.best_bid > 0 and market_snapshot.best_ask > 0:
            mid_price = (market_snapshot.best_bid + market_snapshot.best_ask) / 2
            self.price_history.append(mid_price)
            
            # Keep only recent prices
            if len(self.price_history) > self.lookback_period:
                self.price_history = self.price_history[-self.lookback_period:]
            
            if len(self.price_history) >= 20:
                # Calculate momentum
                recent_prices = self.price_history[-10:]
                older_prices = self.price_history[-20:-10]
                
                if len(recent_prices) > 0 and len(older_prices) > 0:
                    recent_avg = np.mean(recent_prices)
                    older_avg = np.mean(older_prices)
                    momentum = (recent_avg - older_avg) / older_avg
                    
                    # Buy on positive momentum
                    if momentum > self.momentum_threshold and self.last_action != 'buy':
                        orders.append(Order(
                            id=timestamp + self.id,
                            side=Side.BUY,
                            price=market_snapshot.best_ask,  # Market buy
                            quantity=self.quantity,
                            timestamp=timestamp
                        ))
                        self.last_action = 'buy'
                    
                    # Sell on negative momentum
                    elif momentum < -self.momentum_threshold and self.last_action != 'sell':
                        orders.append(Order(
                            id=timestamp + self.id + 1000,
                            side=Side.SELL,
                            price=market_snapshot.best_bid,  # Market sell
                            quantity=self.quantity,
                            timestamp=timestamp
                        ))
                        self.last_action = 'sell'
        
        return orders


class ArbitrageStrategy(BaseStrategy):
    """Simple arbitrage strategy looking for price discrepancies."""
    
    def __init__(self, strategy_id: int, name: str = "ArbitrageStrategy",
                 min_spread: int = 1, quantity: int = 25):
        super().__init__(strategy_id, name)
        self.min_spread = min_spread
        self.quantity = quantity
        self.last_bid = 0
        self.last_ask = 0
    
    def update(self, timestamp: int, market_snapshot: MarketSnapshot) -> List[Order]:
        orders = []
        
        if market_snapshot.best_bid > 0 and market_snapshot.best_ask > 0:
            spread = market_snapshot.best_ask - market_snapshot.best_bid
            
            # Only trade if spread is wide enough
            if spread >= self.min_spread:
                mid_price = (market_snapshot.best_bid + market_snapshot.best_ask) / 2
                
                # Place orders inside the spread
                bid_price = mid_price - 1
                ask_price = mid_price + 1
                
                # Only place if prices have changed
                if bid_price != self.last_bid:
                    orders.append(Order(
                        id=timestamp + self.id,
                        side=Side.BUY,
                        price=bid_price,
                        quantity=self.quantity,
                        timestamp=timestamp
                    ))
                    self.last_bid = bid_price
                
                if ask_price != self.last_ask:
                    orders.append(Order(
                        id=timestamp + self.id + 1000,
                        side=Side.SELL,
                        price=ask_price,
                        quantity=self.quantity,
                        timestamp=timestamp
                    ))
                    self.last_ask = ask_price
        
        return orders


def create_sample_strategies() -> List[BaseStrategy]:
    """Create a sample set of trading strategies."""
    strategies = [
        SimpleStrategy(1001, "SimpleMarketMaker", spread=2, quantity=50),
        MeanReversionStrategy(1002, "MeanReversion", lookback_period=100, threshold=0.02),
        MomentumStrategy(1003, "Momentum", lookback_period=50, momentum_threshold=0.01),
        ArbitrageStrategy(1004, "Arbitrage", min_spread=1, quantity=25)
    ]
    return strategies


def backtest_strategy(strategy: BaseStrategy, 
                     market_data: List[MarketSnapshot],
                     start_capital: float = 10000.0) -> Dict[str, Any]:
    """Simple backtest for a strategy against market data."""
    results = {
        'strategy_name': strategy.name,
        'total_trades': 0,
        'pnl': 0.0,
        'final_inventory': 0,
        'max_drawdown': 0.0,
        'sharpe_ratio': 0.0,
        'win_rate': 0.0
    }
    
    capital = start_capital
    max_capital = start_capital
    returns = []
    
    for snapshot in market_data:
        # Update strategy
        orders = strategy.update(snapshot.timestamp, snapshot)
        
        # Simulate order execution (simplified)
        for order in orders:
            if order.side == Side.BUY and order.price >= snapshot.best_ask:
                # Buy order executed
                trade_price = snapshot.best_ask
                capital -= trade_price * order.quantity
                strategy.inventory += order.quantity
                results['total_trades'] += 1
            elif order.side == Side.SELL and order.price <= snapshot.best_bid:
                # Sell order executed
                trade_price = snapshot.best_bid
                capital += trade_price * order.quantity
                strategy.inventory -= order.quantity
                results['total_trades'] += 1
        
        # Track performance
        total_value = capital + strategy.inventory * (snapshot.best_bid + snapshot.best_ask) / 2
        returns.append((total_value - max_capital) / max_capital)
        
        if total_value > max_capital:
            max_capital = total_value
        
        drawdown = (max_capital - total_value) / max_capital
        if drawdown > results['max_drawdown']:
            results['max_drawdown'] = drawdown
    
    # Calculate final statistics
    results['pnl'] = capital - start_capital
    results['final_inventory'] = strategy.inventory
    
    if len(returns) > 1:
        mean_return = np.mean(returns)
        std_return = np.std(returns)
        if std_return > 0:
            results['sharpe_ratio'] = mean_return / std_return
    
    # Calculate win rate (simplified)
    positive_returns = sum(1 for r in returns if r > 0)
    results['win_rate'] = positive_returns / len(returns) if returns else 0.0
    
    return results
