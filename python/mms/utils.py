"""
Utility functions for market microstructure analysis and visualization.
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from typing import Dict, List, Any, Optional, Tuple
import os
from .core import create_dataframes, calculate_statistics


def plot_results(result_dict: Dict[str, np.ndarray], 
                output_dir: str = "plots",
                figsize: Tuple[int, int] = (15, 10)) -> None:
    """
    Create comprehensive plots from simulation results.
    
    Args:
        result_dict: Results from simulator.run()
        output_dir: Directory to save plots
        figsize: Figure size tuple
    """
    os.makedirs(output_dir, exist_ok=True)
    
    # Convert to DataFrames
    dfs = create_dataframes(result_dict)
    
    # Set up the plotting style
    plt.style.use('seaborn-v0_8')
    sns.set_palette("husl")
    
    # Create subplots
    fig, axes = plt.subplots(2, 3, figsize=figsize)
    fig.suptitle('Market Microstructure Simulation Results', fontsize=16, fontweight='bold')
    
    # 1. Price evolution
    if not dfs['market_snapshots'].empty:
        ax1 = axes[0, 0]
        snapshots = dfs['market_snapshots']
        snapshots['mid_price'] = (snapshots['best_bid'] + snapshots['best_ask']) / 2
        
        ax1.plot(snapshots['timestamp'], snapshots['mid_price'], 'b-', linewidth=1, alpha=0.7)
        ax1.fill_between(snapshots['timestamp'], 
                        snapshots['best_bid'], 
                        snapshots['best_ask'], 
                        alpha=0.3, color='gray', label='Bid-Ask Spread')
        ax1.set_title('Price Evolution')
        ax1.set_xlabel('Timestamp')
        ax1.set_ylabel('Price')
        ax1.legend()
        ax1.grid(True, alpha=0.3)
    
    # 2. Bid-Ask Spread
    if not dfs['market_snapshots'].empty:
        ax2 = axes[0, 1]
        snapshots['spread'] = snapshots['best_ask'] - snapshots['best_bid']
        ax2.plot(snapshots['timestamp'], snapshots['spread'], 'r-', linewidth=1)
        ax2.set_title('Bid-Ask Spread')
        ax2.set_xlabel('Timestamp')
        ax2.set_ylabel('Spread')
        ax2.grid(True, alpha=0.3)
    
    # 3. Trading Volume
    if not dfs['trades'].empty:
        ax3 = axes[0, 2]
        trades = dfs['trades']
        # Aggregate volume by time buckets
        trades['time_bucket'] = trades['timestamp'] // 1000000  # 1ms buckets
        volume_by_time = trades.groupby('time_bucket')['quantity'].sum()
        
        ax3.bar(volume_by_time.index, volume_by_time.values, alpha=0.7, color='green')
        ax3.set_title('Trading Volume Over Time')
        ax3.set_xlabel('Time Bucket')
        ax3.set_ylabel('Volume')
        ax3.grid(True, alpha=0.3)
    
    # 4. Agent PnL
    if not dfs['agent_pnl'].empty:
        ax4 = axes[1, 0]
        pnl_data = dfs['agent_pnl']
        
        for agent_id in pnl_data['agent_id'].unique():
            agent_data = pnl_data[pnl_data['agent_id'] == agent_id]
            ax4.plot(agent_data['timestamp'], agent_data['pnl'], 
                    label=f'Agent {agent_id}', linewidth=2)
        
        ax4.set_title('Agent PnL Over Time')
        ax4.set_xlabel('Timestamp')
        ax4.set_ylabel('PnL')
        ax4.legend()
        ax4.grid(True, alpha=0.3)
    
    # 5. Agent Inventory
    if not dfs['agent_pnl'].empty:
        ax5 = axes[1, 1]
        
        for agent_id in pnl_data['agent_id'].unique():
            agent_data = pnl_data[pnl_data['agent_id'] == agent_id]
            ax5.plot(agent_data['timestamp'], agent_data['inventory'], 
                    label=f'Agent {agent_id}', linewidth=2)
        
        ax5.set_title('Agent Inventory Over Time')
        ax5.set_xlabel('Timestamp')
        ax5.set_ylabel('Inventory')
        ax5.legend()
        ax5.grid(True, alpha=0.3)
    
    # 6. Trade Size Distribution
    if not dfs['trades'].empty:
        ax6 = axes[1, 2]
        trades = dfs['trades']
        
        ax6.hist(trades['quantity'], bins=50, alpha=0.7, color='purple', edgecolor='black')
        ax6.set_title('Trade Size Distribution')
        ax6.set_xlabel('Trade Size')
        ax6.set_ylabel('Frequency')
        ax6.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'simulation_results.png'), dpi=300, bbox_inches='tight')
    plt.close()
    
    # Create additional detailed plots
    _create_detailed_plots(dfs, output_dir)


def _create_detailed_plots(dfs: Dict[str, pd.DataFrame], output_dir: str) -> None:
    """Create additional detailed analysis plots."""
    
    # Price impact analysis
    if not dfs['trades'].empty and not dfs['market_snapshots'].empty:
        fig, axes = plt.subplots(1, 2, figsize=(15, 6))
        
        trades = dfs['trades']
        snapshots = dfs['market_snapshots']
        
        # Trade price vs time
        axes[0].scatter(trades['timestamp'], trades['price'], alpha=0.6, s=10)
        axes[0].set_title('Trade Prices Over Time')
        axes[0].set_xlabel('Timestamp')
        axes[0].set_ylabel('Trade Price')
        axes[0].grid(True, alpha=0.3)
        
        # Volume-weighted average price (VWAP)
        vwap = (trades['price'] * trades['quantity']).sum() / trades['quantity'].sum()
        axes[1].axhline(y=vwap, color='r', linestyle='--', label=f'VWAP: {vwap:.2f}')
        
        # Plot VWAP over time
        trades_sorted = trades.sort_values('timestamp')
        cumulative_volume = trades_sorted['quantity'].cumsum()
        cumulative_value = (trades_sorted['price'] * trades_sorted['quantity']).cumsum()
        rolling_vwap = cumulative_value / cumulative_volume
        
        axes[1].plot(trades_sorted['timestamp'], rolling_vwap, 'b-', alpha=0.7, label='Rolling VWAP')
        axes[1].set_title('Volume-Weighted Average Price')
        axes[1].set_xlabel('Timestamp')
        axes[1].set_ylabel('Price')
        axes[1].legend()
        axes[1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'price_analysis.png'), dpi=300, bbox_inches='tight')
        plt.close()
    
    # Agent performance comparison
    if not dfs['agent_pnl'].empty:
        fig, axes = plt.subplots(2, 2, figsize=(15, 10))
        
        pnl_data = dfs['agent_pnl']
        
        # Final PnL comparison
        final_pnl = pnl_data.groupby('agent_id')['pnl'].last()
        axes[0, 0].bar(final_pnl.index, final_pnl.values)
        axes[0, 0].set_title('Final PnL by Agent')
        axes[0, 0].set_xlabel('Agent ID')
        axes[0, 0].set_ylabel('Final PnL')
        axes[0, 0].grid(True, alpha=0.3)
        
        # Final inventory comparison
        final_inventory = pnl_data.groupby('agent_id')['inventory'].last()
        axes[0, 1].bar(final_inventory.index, final_inventory.values, color='orange')
        axes[0, 1].set_title('Final Inventory by Agent')
        axes[0, 1].set_xlabel('Agent ID')
        axes[0, 1].set_ylabel('Final Inventory')
        axes[0, 1].grid(True, alpha=0.3)
        
        # PnL distribution
        all_pnl = pnl_data['pnl'].values
        axes[1, 0].hist(all_pnl, bins=50, alpha=0.7, color='green', edgecolor='black')
        axes[1, 0].set_title('PnL Distribution')
        axes[1, 0].set_xlabel('PnL')
        axes[1, 0].set_ylabel('Frequency')
        axes[1, 0].grid(True, alpha=0.3)
        
        # Inventory distribution
        all_inventory = pnl_data['inventory'].values
        axes[1, 1].hist(all_inventory, bins=50, alpha=0.7, color='red', edgecolor='black')
        axes[1, 1].set_title('Inventory Distribution')
        axes[1, 1].set_xlabel('Inventory')
        axes[1, 1].set_ylabel('Frequency')
        axes[1, 1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'agent_performance.png'), dpi=300, bbox_inches='tight')
        plt.close()


def save_results(result_dict: Dict[str, np.ndarray], 
                output_dir: str = "results") -> Dict[str, str]:
    """
    Save simulation results to CSV files.
    
    Args:
        result_dict: Results from simulator.run()
        output_dir: Directory to save results
        
    Returns:
        Dictionary mapping data types to file paths
    """
    os.makedirs(output_dir, exist_ok=True)
    
    # Convert to DataFrames
    dfs = create_dataframes(result_dict)
    
    file_paths = {}
    
    # Save trades
    if not dfs['trades'].empty:
        trades_file = os.path.join(output_dir, 'trades.csv')
        dfs['trades'].to_csv(trades_file, index=False)
        file_paths['trades'] = trades_file
    
    # Save market snapshots
    if not dfs['market_snapshots'].empty:
        snapshots_file = os.path.join(output_dir, 'market_snapshots.csv')
        dfs['market_snapshots'].to_csv(snapshots_file, index=False)
        file_paths['market_snapshots'] = snapshots_file
    
    # Save agent PnL
    if not dfs['agent_pnl'].empty:
        pnl_file = os.path.join(output_dir, 'agent_pnl.csv')
        dfs['agent_pnl'].to_csv(pnl_file, index=False)
        file_paths['agent_pnl'] = pnl_file
    
    # Save summary statistics
    stats = calculate_statistics(dfs)
    stats_file = os.path.join(output_dir, 'summary_statistics.txt')
    with open(stats_file, 'w') as f:
        f.write("Market Microstructure Simulation Summary\n")
        f.write("=" * 50 + "\n\n")
        
        for key, value in stats.items():
            if isinstance(value, dict):
                f.write(f"{key}:\n")
                for sub_key, sub_value in value.items():
                    f.write(f"  {sub_key}: {sub_value}\n")
                f.write("\n")
            else:
                f.write(f"{key}: {value}\n")
    
    file_paths['statistics'] = stats_file
    
    return file_paths


def create_sample_agents() -> Dict[str, Any]:
    """Create sample agent configurations for testing."""
    return {
        'market_maker': {
            'spread': 2,
            'quantity': 50,
            'refresh_interval': 50000,
            'max_inventory': 1000,
            'inventory_penalty': 0.001
        },
        'taker': {
            'intensity': 0.8,
            'side_bias': 0.5,
            'quantity_mean': 40,
            'quantity_std': 10,
            'use_market_orders': True
        },
        'noise_trader': {
            'limit_intensity': 1.5,
            'cancel_intensity': 0.7,
            'quantity_mean': 30,
            'quantity_std': 8,
            'price_volatility': 5,
            'cancel_probability': 0.3
        }
    }


def benchmark_simulation(n_steps: int = 100000, 
                        seed: int = 42,
                        iterations: int = 5) -> Dict[str, float]:
    """
    Benchmark simulation performance.
    
    Args:
        n_steps: Number of simulation steps
        seed: Random seed
        iterations: Number of benchmark iterations
        
    Returns:
        Performance statistics
    """
    from .core import Simulator, SimulationConfig, MarketMakerConfig, TakerConfig, NoiseTraderConfig
    
    times = []
    
    for i in range(iterations):
        config = SimulationConfig(seed=seed + i)
        sim = Simulator(config)
        
        maker_config = MarketMakerConfig()
        taker_config = TakerConfig()
        noise_config = NoiseTraderConfig()
        
        import time
        start_time = time.time()
        
        result = sim.run(n_steps, maker_config, taker_config, noise_config)
        
        end_time = time.time()
        times.append(end_time - start_time)
    
    return {
        'mean_time': np.mean(times),
        'std_time': np.std(times),
        'min_time': np.min(times),
        'max_time': np.max(times),
        'events_per_second': n_steps / np.mean(times),
        'iterations': iterations
    }


def analyze_liquidity(result_dict: Dict[str, np.ndarray]) -> Dict[str, Any]:
    """
    Analyze market liquidity metrics.
    
    Args:
        result_dict: Simulation results
        
    Returns:
        Liquidity analysis metrics
    """
    dfs = create_dataframes(result_dict)
    
    if dfs['market_snapshots'].empty:
        return {}
    
    snapshots = dfs['market_snapshots']
    
    # Calculate mid prices and spreads
    snapshots['mid_price'] = (snapshots['best_bid'] + snapshots['best_ask']) / 2
    snapshots['spread'] = snapshots['best_ask'] - snapshots['best_bid']
    snapshots['relative_spread'] = snapshots['spread'] / snapshots['mid_price']
    
    # Calculate depth (total quantity at best bid/ask)
    snapshots['depth'] = snapshots['best_bid_qty'] + snapshots['best_ask_qty']
    
    # Liquidity metrics
    liquidity_metrics = {
        'avg_spread': snapshots['spread'].mean(),
        'avg_relative_spread': snapshots['relative_spread'].mean(),
        'spread_volatility': snapshots['spread'].std(),
        'avg_depth': snapshots['depth'].mean(),
        'depth_volatility': snapshots['depth'].std(),
        'min_spread': snapshots['spread'].min(),
        'max_spread': snapshots['spread'].max(),
        'spread_percentiles': {
            '25th': snapshots['spread'].quantile(0.25),
            '50th': snapshots['spread'].quantile(0.50),
            '75th': snapshots['spread'].quantile(0.75),
            '95th': snapshots['spread'].quantile(0.95)
        }
    }
    
    return liquidity_metrics
