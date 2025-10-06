"""
Core market microstructure simulator interface.
"""

import numpy as np
import pandas as pd
from typing import Dict, List, Optional, Tuple, Any
import mms_core as core


class SimulationConfig:
    """Configuration for market microstructure simulation."""
    
    def __init__(self, 
                 seed: int = 42,
                 start_time: int = 0,
                 time_step: int = 1000,
                 max_steps: int = 1000000,
                 enable_logging: bool = False,
                 output_dir: str = "output"):
        self.config = core.SimulationConfig()
        self.config.seed = seed
        self.config.start_time = start_time
        self.config.time_step = time_step
        self.config.max_steps = max_steps
        self.config.enable_logging = enable_logging
        self.config.output_dir = output_dir


class MarketMakerConfig:
    """Configuration for market maker agent."""
    
    def __init__(self,
                 spread: int = 2,
                 quantity: int = 50,
                 refresh_interval: int = 50000,
                 max_inventory: int = 1000,
                 inventory_penalty: float = 0.001):
        self.config = core.MarketMakerConfig()
        self.config.spread = spread
        self.config.quantity = quantity
        self.config.refresh_interval = refresh_interval
        self.config.max_inventory = max_inventory
        self.config.inventory_penalty = inventory_penalty


class TakerConfig:
    """Configuration for liquidity taker agent."""
    
    def __init__(self,
                 intensity: float = 0.8,
                 side_bias: float = 0.5,
                 quantity_mean: int = 40,
                 quantity_std: int = 10,
                 use_market_orders: bool = True):
        self.config = core.TakerConfig()
        self.config.intensity = intensity
        self.config.side_bias = side_bias
        self.config.quantity_mean = quantity_mean
        self.config.quantity_std = quantity_std
        self.config.use_market_orders = use_market_orders


class NoiseTraderConfig:
    """Configuration for noise trader agent."""
    
    def __init__(self,
                 limit_intensity: float = 1.5,
                 cancel_intensity: float = 0.7,
                 quantity_mean: int = 30,
                 quantity_std: int = 8,
                 price_volatility: int = 5,
                 cancel_probability: float = 0.3):
        self.config = core.NoiseTraderConfig()
        self.config.limit_intensity = limit_intensity
        self.config.cancel_intensity = cancel_intensity
        self.config.quantity_mean = quantity_mean
        self.config.quantity_std = quantity_std
        self.config.price_volatility = price_volatility
        self.config.cancel_probability = cancel_probability


class Simulator:
    """Market microstructure simulator with Python-friendly interface."""
    
    def __init__(self, config: Optional[SimulationConfig] = None):
        if config is None:
            config = SimulationConfig()
        self._simulator = core.Simulator(config.config)
    
    def run(self, 
            n_steps: int,
            maker_config: Optional[MarketMakerConfig] = None,
            taker_config: Optional[TakerConfig] = None,
            noise_config: Optional[NoiseTraderConfig] = None) -> Dict[str, np.ndarray]:
        """
        Run simulation and return results as pandas-compatible numpy arrays.
        
        Args:
            n_steps: Number of simulation steps
            maker_config: Market maker configuration
            taker_config: Taker configuration  
            noise_config: Noise trader configuration
            
        Returns:
            Dictionary with numpy arrays for trades, market snapshots, and agent PnL
        """
        if maker_config is None:
            maker_config = MarketMakerConfig()
        if taker_config is None:
            taker_config = TakerConfig()
        if noise_config is None:
            noise_config = NoiseTraderConfig()
        
        result = self._simulator.run(
            n_steps,
            maker_config.config,
            taker_config.config,
            noise_config.config
        )
        
        return self._convert_result_to_arrays(result)
    
    def _convert_result_to_arrays(self, result: core.RunResult) -> Dict[str, np.ndarray]:
        """Convert C++ result to numpy arrays."""
        
        # Convert trades
        trades_data = []
        for trade in result.trades:
            trades_data.append([
                trade.timestamp,
                trade.maker_id,
                trade.taker_id,
                trade.price,
                trade.quantity
            ])
        
        trades_array = np.array(trades_data, dtype=[
            ('timestamp', 'i8'),
            ('maker_id', 'u8'),
            ('taker_id', 'u8'),
            ('price', 'i8'),
            ('quantity', 'i8')
        ])
        
        # Convert market snapshots
        snapshots_data = []
        for snapshot in result.market_snapshots:
            snapshots_data.append([
                snapshot.timestamp,
                snapshot.best_bid,
                snapshot.best_ask,
                snapshot.best_bid_qty,
                snapshot.best_ask_qty,
                snapshot.last_trade_price
            ])
        
        snapshots_array = np.array(snapshots_data, dtype=[
            ('timestamp', 'i8'),
            ('best_bid', 'i8'),
            ('best_ask', 'i8'),
            ('best_bid_qty', 'i8'),
            ('best_ask_qty', 'i8'),
            ('last_trade_price', 'i8')
        ])
        
        # Convert agent PnL
        pnl_data = []
        for agent_id, timestamp, pnl, inventory in result.agent_pnl:
            pnl_data.append([
                timestamp,
                agent_id,
                pnl,
                inventory
            ])
        
        pnl_array = np.array(pnl_data, dtype=[
            ('timestamp', 'i8'),
            ('agent_id', 'u8'),
            ('pnl', 'f8'),
            ('inventory', 'i8')
        ])
        
        return {
            'trades': trades_array,
            'market_snapshots': snapshots_array,
            'agent_pnl': pnl_array,
            'total_events_processed': np.array([result.total_events_processed]),
            'total_trades': np.array([result.total_trades]),
            'simulation_duration': np.array([result.simulation_duration]),
            'simulation_time_seconds': np.array([result.simulation_time_seconds])
        }


# Re-export core types for convenience
Side = core.Side
EventType = core.EventType
Order = core.Order
Trade = core.Trade
MarketSnapshot = core.MarketSnapshot


def create_dataframes(result_dict: Dict[str, np.ndarray]) -> Dict[str, pd.DataFrame]:
    """Convert numpy arrays to pandas DataFrames."""
    return {
        'trades': pd.DataFrame(result_dict['trades']),
        'market_snapshots': pd.DataFrame(result_dict['market_snapshots']),
        'agent_pnl': pd.DataFrame(result_dict['agent_pnl'])
    }


def calculate_statistics(dfs: Dict[str, pd.DataFrame]) -> Dict[str, Any]:
    """Calculate basic statistics from simulation results."""
    stats = {}
    
    if 'trades' in dfs and not dfs['trades'].empty:
        trades = dfs['trades']
        stats['total_trades'] = len(trades)
        stats['total_volume'] = trades['quantity'].sum()
        stats['vwap'] = (trades['price'] * trades['quantity']).sum() / trades['quantity'].sum()
        stats['price_range'] = (trades['price'].max(), trades['price'].min())
    
    if 'market_snapshots' in dfs and not dfs['market_snapshots'].empty:
        snapshots = dfs['market_snapshots']
        # Calculate mid prices
        snapshots['mid_price'] = (snapshots['best_bid'] + snapshots['best_ask']) / 2
        snapshots['spread'] = snapshots['best_ask'] - snapshots['best_bid']
        
        stats['avg_spread'] = snapshots['spread'].mean()
        stats['avg_mid_price'] = snapshots['mid_price'].mean()
        stats['price_volatility'] = snapshots['mid_price'].std()
    
    if 'agent_pnl' in dfs and not dfs['agent_pnl'].empty:
        pnl = dfs['agent_pnl']
        stats['agent_performance'] = {}
        for agent_id in pnl['agent_id'].unique():
            agent_data = pnl[pnl['agent_id'] == agent_id]
            stats['agent_performance'][agent_id] = {
                'final_pnl': agent_data['pnl'].iloc[-1] if len(agent_data) > 0 else 0,
                'final_inventory': agent_data['inventory'].iloc[-1] if len(agent_data) > 0 else 0,
                'max_pnl': agent_data['pnl'].max(),
                'min_pnl': agent_data['pnl'].min()
            }
    
    return stats
