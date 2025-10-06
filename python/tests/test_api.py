"""
Python API tests for Market Microstructure Simulator.
"""

import pytest
import numpy as np
import pandas as pd
import tempfile
import os
from pathlib import Path

# Add the python directory to the path
import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from mms import (
    Simulator, SimulationConfig, MarketMakerConfig, TakerConfig, NoiseTraderConfig,
    Side, EventType, Order, Trade, MarketSnapshot
)
from mms.utils import create_dataframes, calculate_statistics, plot_results, save_results


class TestSimulator:
    """Test the main Simulator class."""
    
    def test_simulator_initialization(self):
        """Test simulator initialization with default config."""
        sim = Simulator()
        assert sim is not None
    
    def test_simulator_with_config(self):
        """Test simulator initialization with custom config."""
        config = SimulationConfig(seed=123, time_step=2000)
        sim = Simulator(config)
        assert sim is not None
    
    def test_basic_simulation_run(self):
        """Test basic simulation execution."""
        sim = Simulator(SimulationConfig(seed=42))
        
        maker_config = MarketMakerConfig()
        taker_config = TakerConfig()
        noise_config = NoiseTraderConfig()
        
        result = sim.run(1000, maker_config, taker_config, noise_config)
        
        assert 'trades' in result
        assert 'market_snapshots' in result
        assert 'agent_pnl' in result
        assert 'total_events_processed' in result
        assert 'total_trades' in result
        assert 'simulation_duration' in result
        assert 'simulation_time_seconds' in result
        
        assert result['total_events_processed'][0] > 0
        assert result['total_trades'][0] >= 0
        assert result['simulation_duration'][0] > 0
        assert result['simulation_time_seconds'][0] > 0.0
    
    def test_deterministic_simulation(self):
        """Test that simulations with same seed produce identical results."""
        config = SimulationConfig(seed=12345)
        sim1 = Simulator(config)
        sim2 = Simulator(config)
        
        maker_config = MarketMakerConfig()
        taker_config = TakerConfig()
        noise_config = NoiseTraderConfig()
        
        result1 = sim1.run(100, maker_config, taker_config, noise_config)
        result2 = sim2.run(100, maker_config, taker_config, noise_config)
        
        assert result1['total_events_processed'][0] == result2['total_events_processed'][0]
        assert result1['total_trades'][0] == result2['total_trades'][0]
        assert result1['simulation_duration'][0] == result2['simulation_duration'][0]
    
    def test_different_seeds_produce_different_results(self):
        """Test that different seeds produce different results."""
        sim1 = Simulator(SimulationConfig(seed=11111))
        sim2 = Simulator(SimulationConfig(seed=22222))
        
        maker_config = MarketMakerConfig()
        taker_config = TakerConfig()
        noise_config = NoiseTraderConfig()
        
        result1 = sim1.run(100, maker_config, taker_config, noise_config)
        result2 = sim2.run(100, maker_config, taker_config, noise_config)
        
        assert result1['total_events_processed'][0] != result2['total_events_processed'][0]
    
    def test_custom_agent_configurations(self):
        """Test simulation with custom agent configurations."""
        sim = Simulator(SimulationConfig(seed=42))
        
        maker_config = MarketMakerConfig(spread=5, quantity=100)
        taker_config = TakerConfig(intensity=2.0, quantity_mean=80)
        noise_config = NoiseTraderConfig(limit_intensity=0.5, quantity_mean=20)
        
        result = sim.run(200, maker_config, taker_config, noise_config)
        
        assert result['total_events_processed'][0] > 0
        assert result['total_trades'][0] >= 0
    
    def test_empty_simulation(self):
        """Test simulation with 0 steps."""
        sim = Simulator(SimulationConfig(seed=42))
        
        maker_config = MarketMakerConfig()
        taker_config = TakerConfig()
        noise_config = NoiseTraderConfig()
        
        result = sim.run(0, maker_config, taker_config, noise_config)
        
        assert result['total_events_processed'][0] == 0
        assert result['total_trades'][0] == 0
        assert result['simulation_duration'][0] == 0


class TestDataTypes:
    """Test basic data types and structures."""
    
    def test_side_enum(self):
        """Test Side enum values."""
        assert Side.BUY == 0
        assert Side.SELL == 1
    
    def test_event_type_enum(self):
        """Test EventType enum values."""
        assert EventType.LIMIT == 0
        assert EventType.MARKET == 1
        assert EventType.CANCEL == 2
    
    def test_order_creation(self):
        """Test Order object creation."""
        order = Order(id=1, side=Side.BUY, price=10000, quantity=100, timestamp=1000)
        
        assert order.id == 1
        assert order.side == Side.BUY
        assert order.price == 10000
        assert order.quantity == 100
        assert order.timestamp == 1000
    
    def test_trade_creation(self):
        """Test Trade object creation."""
        trade = Trade(maker_id=1, taker_id=2, price=10000, quantity=50, timestamp=1001)
        
        assert trade.maker_id == 1
        assert trade.taker_id == 2
        assert trade.price == 10000
        assert trade.quantity == 50
        assert trade.timestamp == 1001
    
    def test_market_snapshot_creation(self):
        """Test MarketSnapshot object creation."""
        snapshot = MarketSnapshot(
            best_bid=9999, best_ask=10001, best_bid_qty=100, best_ask_qty=50,
            last_trade_price=10000, timestamp=1002
        )
        
        assert snapshot.best_bid == 9999
        assert snapshot.best_ask == 10001
        assert snapshot.best_bid_qty == 100
        assert snapshot.best_ask_qty == 50
        assert snapshot.last_trade_price == 10000
        assert snapshot.timestamp == 1002


class TestConfigurations:
    """Test configuration classes."""
    
    def test_simulation_config(self):
        """Test SimulationConfig creation."""
        config = SimulationConfig(
            seed=123,
            start_time=1000,
            time_step=5000,
            max_steps=50000,
            enable_logging=True,
            output_dir="test_output"
        )
        
        assert config.config.seed == 123
        assert config.config.start_time == 1000
        assert config.config.time_step == 5000
        assert config.config.max_steps == 50000
        assert config.config.enable_logging == True
        assert config.config.output_dir == "test_output"
    
    def test_market_maker_config(self):
        """Test MarketMakerConfig creation."""
        config = MarketMakerConfig(
            spread=3,
            quantity=75,
            refresh_interval=60000,
            max_inventory=1500,
            inventory_penalty=0.002
        )
        
        assert config.config.spread == 3
        assert config.config.quantity == 75
        assert config.config.refresh_interval == 60000
        assert config.config.max_inventory == 1500
        assert config.config.inventory_penalty == 0.002
    
    def test_taker_config(self):
        """Test TakerConfig creation."""
        config = TakerConfig(
            intensity=1.2,
            side_bias=0.6,
            quantity_mean=60,
            quantity_std=15,
            use_market_orders=False
        )
        
        assert config.config.intensity == 1.2
        assert config.config.side_bias == 0.6
        assert config.config.quantity_mean == 60
        assert config.config.quantity_std == 15
        assert config.config.use_market_orders == False
    
    def test_noise_trader_config(self):
        """Test NoiseTraderConfig creation."""
        config = NoiseTraderConfig(
            limit_intensity=2.0,
            cancel_intensity=1.0,
            quantity_mean=40,
            quantity_std=12,
            price_volatility=8,
            cancel_probability=0.4
        )
        
        assert config.config.limit_intensity == 2.0
        assert config.config.cancel_intensity == 1.0
        assert config.config.quantity_mean == 40
        assert config.config.quantity_std == 12
        assert config.config.price_volatility == 8
        assert config.config.cancel_probability == 0.4


class TestUtilities:
    """Test utility functions."""
    
    def test_create_dataframes(self):
        """Test conversion of numpy arrays to pandas DataFrames."""
        # Create mock result data
        result = {
            'trades': np.array([(1000, 1, 2, 10000, 50)], 
                              dtype=[('timestamp', 'i8'), ('maker_id', 'u8'), 
                                     ('taker_id', 'u8'), ('price', 'i8'), ('quantity', 'i8')]),
            'market_snapshots': np.array([(1000, 9999, 10001, 100, 50, 10000)],
                                       dtype=[('timestamp', 'i8'), ('best_bid', 'i8'),
                                              ('best_ask', 'i8'), ('best_bid_qty', 'i8'),
                                              ('best_ask_qty', 'i8'), ('last_trade_price', 'i8')]),
            'agent_pnl': np.array([(1000, 1, 100.5, 50)],
                                 dtype=[('timestamp', 'i8'), ('agent_id', 'u8'),
                                        ('pnl', 'f8'), ('inventory', 'i8')])
        }
        
        dfs = create_dataframes(result)
        
        assert 'trades' in dfs
        assert 'market_snapshots' in dfs
        assert 'agent_pnl' in dfs
        
        assert isinstance(dfs['trades'], pd.DataFrame)
        assert isinstance(dfs['market_snapshots'], pd.DataFrame)
        assert isinstance(dfs['agent_pnl'], pd.DataFrame)
        
        assert len(dfs['trades']) == 1
        assert len(dfs['market_snapshots']) == 1
        assert len(dfs['agent_pnl']) == 1
    
    def test_calculate_statistics(self):
        """Test statistics calculation."""
        # Create mock DataFrames
        trades_df = pd.DataFrame({
            'timestamp': [1000, 1001, 1002],
            'maker_id': [1, 2, 1],
            'taker_id': [2, 1, 2],
            'price': [10000, 10001, 10002],
            'quantity': [50, 30, 40]
        })
        
        snapshots_df = pd.DataFrame({
            'timestamp': [1000, 1001, 1002],
            'best_bid': [9999, 10000, 10001],
            'best_ask': [10001, 10002, 10003],
            'best_bid_qty': [100, 80, 120],
            'best_ask_qty': [50, 60, 40],
            'last_trade_price': [10000, 10001, 10002]
        })
        
        pnl_df = pd.DataFrame({
            'timestamp': [1000, 1001, 1002],
            'agent_id': [1, 1, 1],
            'pnl': [0.0, 50.0, 120.0],
            'inventory': [0, 10, -5]
        })
        
        dfs = {
            'trades': trades_df,
            'market_snapshots': snapshots_df,
            'agent_pnl': pnl_df
        }
        
        stats = calculate_statistics(dfs)
        
        assert 'total_trades' in stats
        assert 'total_volume' in stats
        assert 'vwap' in stats
        assert 'avg_spread' in stats
        assert 'avg_mid_price' in stats
        
        assert stats['total_trades'] == 3
        assert stats['total_volume'] == 120  # 50 + 30 + 40
        assert stats['vwap'] == (10000*50 + 10001*30 + 10002*40) / 120
    
    def test_save_results(self):
        """Test saving results to CSV files."""
        with tempfile.TemporaryDirectory() as temp_dir:
            # Create mock result data
            result = {
                'trades': np.array([(1000, 1, 2, 10000, 50)], 
                                  dtype=[('timestamp', 'i8'), ('maker_id', 'u8'), 
                                         ('taker_id', 'u8'), ('price', 'i8'), ('quantity', 'i8')]),
                'market_snapshots': np.array([(1000, 9999, 10001, 100, 50, 10000)],
                                           dtype=[('timestamp', 'i8'), ('best_bid', 'i8'),
                                                  ('best_ask', 'i8'), ('best_bid_qty', 'i8'),
                                                  ('best_ask_qty', 'i8'), ('last_trade_price', 'i8')]),
                'agent_pnl': np.array([(1000, 1, 100.5, 50)],
                                     dtype=[('timestamp', 'i8'), ('agent_id', 'u8'),
                                            ('pnl', 'f8'), ('inventory', 'i8')])
            }
            
            file_paths = save_results(result, temp_dir)
            
            assert 'trades' in file_paths
            assert 'market_snapshots' in file_paths
            assert 'agent_pnl' in file_paths
            assert 'statistics' in file_paths
            
            # Check that files were created
            assert os.path.exists(file_paths['trades'])
            assert os.path.exists(file_paths['market_snapshots'])
            assert os.path.exists(file_paths['agent_pnl'])
            assert os.path.exists(file_paths['statistics'])
            
            # Check file contents
            trades_df = pd.read_csv(file_paths['trades'])
            assert len(trades_df) == 1
            assert trades_df.iloc[0]['timestamp'] == 1000
            assert trades_df.iloc[0]['price'] == 10000


class TestIntegration:
    """Integration tests."""
    
    def test_full_simulation_workflow(self):
        """Test complete simulation workflow."""
        with tempfile.TemporaryDirectory() as temp_dir:
            # Configure simulation
            sim = Simulator(SimulationConfig(seed=42, output_dir=temp_dir))
            
            maker_config = MarketMakerConfig(spread=2, quantity=50)
            taker_config = TakerConfig(intensity=0.8, quantity_mean=40)
            noise_config = NoiseTraderConfig(limit_intensity=1.5, quantity_mean=30)
            
            # Run simulation
            result = sim.run(1000, maker_config, taker_config, noise_config)
            
            # Convert to DataFrames
            dfs = create_dataframes(result)
            
            # Calculate statistics
            stats = calculate_statistics(dfs)
            
            # Save results
            file_paths = save_results(result, temp_dir)
            
            # Verify results
            assert result['total_events_processed'][0] > 0
            assert 'total_trades' in stats
            assert 'trades' in file_paths
            
            # Check that CSV files were created and contain data
            trades_df = pd.read_csv(file_paths['trades'])
            if len(trades_df) > 0:
                assert 'timestamp' in trades_df.columns
                assert 'price' in trades_df.columns
                assert 'quantity' in trades_df.columns


if __name__ == "__main__":
    pytest.main([__file__])
