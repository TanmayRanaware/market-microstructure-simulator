#!/usr/bin/env python3
"""
Market Microstructure Simulator CLI

Command-line interface for running market microstructure simulations
and generating analysis plots and CSV outputs.
"""

import argparse
import sys
import os
import time
from pathlib import Path

# Add the python directory to the path
sys.path.insert(0, str(Path(__file__).parent.parent / "python"))

import numpy as np
import pandas as pd
from mms import Simulator, SimulationConfig, MarketMakerConfig, TakerConfig, NoiseTraderConfig
from mms.utils import plot_results, save_results, benchmark_simulation, analyze_liquidity


def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Market Microstructure Simulator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run basic simulation with default parameters
  python run_sim.py --steps 100000 --seed 42

  # Run with custom agent parameters
  python run_sim.py --steps 200000 --seed 123 --maker-spread 3 --taker-intensity 1.2

  # Run benchmark test
  python run_sim.py --benchmark --steps 100000 --iterations 5

  # Generate plots only (from existing results)
  python run_sim.py --plot-only --outdir results
        """
    )
    
    # Simulation parameters
    parser.add_argument('--steps', type=int, default=100000,
                       help='Number of simulation steps (default: 100000)')
    parser.add_argument('--seed', type=int, default=42,
                       help='Random seed for reproducibility (default: 42)')
    parser.add_argument('--output-dir', '--outdir', type=str, default='output',
                       help='Output directory for results (default: output)')
    
    # Market maker parameters
    parser.add_argument('--maker-spread', type=int, default=2,
                       help='Market maker bid-ask spread (default: 2)')
    parser.add_argument('--maker-quantity', type=int, default=50,
                       help='Market maker order quantity (default: 50)')
    parser.add_argument('--maker-refresh', type=int, default=50000,
                       help='Market maker refresh interval in nanoseconds (default: 50000)')
    parser.add_argument('--maker-max-inventory', type=int, default=1000,
                       help='Market maker max inventory (default: 1000)')
    
    # Taker parameters
    parser.add_argument('--taker-intensity', type=float, default=0.8,
                       help='Taker order arrival intensity (default: 0.8)')
    parser.add_argument('--taker-side-bias', type=float, default=0.5,
                       help='Taker buy order probability (default: 0.5)')
    parser.add_argument('--taker-quantity-mean', type=int, default=40,
                       help='Taker mean order size (default: 40)')
    parser.add_argument('--taker-quantity-std', type=int, default=10,
                       help='Taker order size standard deviation (default: 10)')
    
    # Noise trader parameters
    parser.add_argument('--noise-limit-intensity', type=float, default=1.5,
                       help='Noise trader limit order intensity (default: 1.5)')
    parser.add_argument('--noise-cancel-intensity', type=float, default=0.7,
                       help='Noise trader cancel order intensity (default: 0.7)')
    parser.add_argument('--noise-quantity-mean', type=int, default=30,
                       help='Noise trader mean order size (default: 30)')
    parser.add_argument('--noise-price-volatility', type=int, default=5,
                       help='Noise trader price volatility (default: 5)')
    
    # Simulation options
    parser.add_argument('--time-step', type=int, default=1000,
                       help='Simulation time step in nanoseconds (default: 1000)')
    parser.add_argument('--no-plots', action='store_true',
                       help='Skip generating plots')
    parser.add_argument('--no-csv', action='store_true',
                       help='Skip saving CSV files')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Enable verbose output')
    
    # Special modes
    parser.add_argument('--benchmark', action='store_true',
                       help='Run performance benchmark')
    parser.add_argument('--iterations', type=int, default=5,
                       help='Number of benchmark iterations (default: 5)')
    parser.add_argument('--plot-only', action='store_true',
                       help='Generate plots from existing CSV files in output directory')
    
    return parser.parse_args()


def run_simulation(args):
    """Run the market microstructure simulation."""
    print("🚀 Starting Market Microstructure Simulation")
    print("=" * 50)
    
    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Create simulation configuration
    sim_config = SimulationConfig(
        seed=args.seed,
        start_time=0,
        time_step=args.time_step,
        max_steps=args.steps,
        enable_logging=args.verbose,
        output_dir=args.output_dir
    )
    
    # Create agent configurations
    maker_config = MarketMakerConfig(
        spread=args.maker_spread,
        quantity=args.maker_quantity,
        refresh_interval=args.maker_refresh,
        max_inventory=args.maker_max_inventory,
        inventory_penalty=0.001
    )
    
    taker_config = TakerConfig(
        intensity=args.taker_intensity,
        side_bias=args.taker_side_bias,
        quantity_mean=args.taker_quantity_mean,
        quantity_std=args.taker_quantity_std,
        use_market_orders=True
    )
    
    noise_config = NoiseTraderConfig(
        limit_intensity=args.noise_limit_intensity,
        cancel_intensity=args.noise_cancel_intensity,
        quantity_mean=args.noise_quantity_mean,
        quantity_std=8,
        price_volatility=args.noise_price_volatility,
        cancel_probability=0.3
    )
    
    # Print configuration
    if args.verbose:
        print(f"Simulation Steps: {args.steps:,}")
        print(f"Random Seed: {args.seed}")
        print(f"Output Directory: {args.output_dir}")
        print(f"Market Maker Spread: {args.maker_spread}")
        print(f"Taker Intensity: {args.taker_intensity}")
        print(f"Noise Limit Intensity: {args.noise_limit_intensity}")
        print()
    
    # Run simulation
    print("⏳ Running simulation...")
    start_time = time.time()
    
    sim = Simulator(sim_config)
    result = sim.run(args.steps, maker_config, taker_config, noise_config)
    
    end_time = time.time()
    simulation_time = end_time - start_time
    
    print(f"✅ Simulation completed in {simulation_time:.2f} seconds")
    print(f"📊 Events per second: {result['total_events_processed'][0] / simulation_time:,.0f}")
    
    # Save results
    if not args.no_csv:
        print("💾 Saving results to CSV...")
        file_paths = save_results(result, args.output_dir)
        for data_type, file_path in file_paths.items():
            print(f"   {data_type}: {file_path}")
    
    # Generate plots
    if not args.no_plots:
        print("📈 Generating plots...")
        plot_results(result, args.output_dir)
        print(f"   Plots saved to: {args.output_dir}/plots/")
    
    # Print summary statistics
    print("\n📋 Simulation Summary:")
    print("-" * 30)
    print(f"Total Events Processed: {result['total_events_processed'][0]:,}")
    print(f"Total Trades: {result['total_trades'][0]:,}")
    print(f"Simulation Duration: {result['simulation_duration'][0]:,} ns")
    print(f"Execution Time: {simulation_time:.2f} seconds")
    
    # Analyze liquidity
    liquidity_metrics = analyze_liquidity(result)
    if liquidity_metrics:
        print(f"\n💧 Liquidity Metrics:")
        print(f"Average Spread: {liquidity_metrics['avg_spread']:.2f}")
        print(f"Average Depth: {liquidity_metrics['avg_depth']:.2f}")
        print(f"Spread Volatility: {liquidity_metrics['spread_volatility']:.2f}")
    
    print(f"\n🎯 Results saved to: {args.output_dir}/")
    return result


def run_benchmark(args):
    """Run performance benchmark."""
    print("🏃 Running Performance Benchmark")
    print("=" * 40)
    
    benchmark_results = benchmark_simulation(
        n_steps=args.steps,
        seed=args.seed,
        iterations=args.iterations
    )
    
    print(f"Benchmark Results ({args.iterations} iterations):")
    print(f"  Mean Time: {benchmark_results['mean_time']:.3f} seconds")
    print(f"  Std Time: {benchmark_results['std_time']:.3f} seconds")
    print(f"  Min Time: {benchmark_results['min_time']:.3f} seconds")
    print(f"  Max Time: {benchmark_results['max_time']:.3f} seconds")
    print(f"  Events/Second: {benchmark_results['events_per_second']:,.0f}")
    
    # Save benchmark results
    benchmark_file = os.path.join(args.output_dir, 'benchmark_results.txt')
    os.makedirs(args.output_dir, exist_ok=True)
    
    with open(benchmark_file, 'w') as f:
        f.write("Market Microstructure Simulator Benchmark Results\n")
        f.write("=" * 50 + "\n\n")
        f.write(f"Steps: {args.steps:,}\n")
        f.write(f"Seed: {args.seed}\n")
        f.write(f"Iterations: {args.iterations}\n\n")
        for key, value in benchmark_results.items():
            f.write(f"{key}: {value}\n")
    
    print(f"\nBenchmark results saved to: {benchmark_file}")


def plot_existing_results(args):
    """Generate plots from existing CSV files."""
    print("📈 Generating plots from existing results...")
    
    if not os.path.exists(args.output_dir):
        print(f"❌ Output directory {args.output_dir} does not exist!")
        return
    
    # Check for required CSV files
    required_files = ['trades.csv', 'market_snapshots.csv', 'agent_pnl.csv']
    missing_files = []
    
    for file_name in required_files:
        file_path = os.path.join(args.output_dir, file_name)
        if not os.path.exists(file_path):
            missing_files.append(file_name)
    
    if missing_files:
        print(f"❌ Missing required files: {', '.join(missing_files)}")
        return
    
    # Load data and convert to numpy arrays
    try:
        trades_df = pd.read_csv(os.path.join(args.output_dir, 'trades.csv'))
        snapshots_df = pd.read_csv(os.path.join(args.output_dir, 'market_snapshots.csv'))
        pnl_df = pd.read_csv(os.path.join(args.output_dir, 'agent_pnl.csv'))
        
        # Convert to the expected format
        result = {
            'trades': trades_df.to_records(index=False),
            'market_snapshots': snapshots_df.to_records(index=False),
            'agent_pnl': pnl_df.to_records(index=False)
        }
        
        # Generate plots
        plot_results(result, args.output_dir)
        print(f"✅ Plots generated successfully in: {args.output_dir}/plots/")
        
    except Exception as e:
        print(f"❌ Error generating plots: {e}")


def main():
    """Main entry point."""
    args = parse_arguments()
    
    try:
        if args.plot_only:
            plot_existing_results(args)
        elif args.benchmark:
            run_benchmark(args)
        else:
            run_simulation(args)
            
    except KeyboardInterrupt:
        print("\n⏹️  Simulation interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Error: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
