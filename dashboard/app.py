#!/usr/bin/env python3
"""
Market Microstructure Simulator - Web Dashboard
"""

from flask import Flask, render_template, request, jsonify, send_from_directory
import json
import subprocess
import os
import sys
import pandas as pd
import plotly
import plotly.graph_objs as go
from plotly.utils import PlotlyJSONEncoder
import numpy as np
from datetime import datetime
import threading
import time

# Add the python directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'python'))

app = Flask(__name__)

# Global variables for simulation state
simulation_results = {}
simulation_running = False
simulation_logs = []

class SimulationRunner:
    def __init__(self):
        self.running = False
        self.results = {}
        self.logs = []
    
    def run_simulation(self, config):
        """Run simulation in background thread"""
        self.running = True
        self.logs = []
        
        try:
            self.logs.append(f"Starting simulation at {datetime.now()}")
            
            # Run the C++ simulation
            cmd = [
                './simple_sim'
            ]
            
            # Change to build directory
            build_dir = os.path.join(os.path.dirname(__file__), '..', 'build')
            if not os.path.exists(build_dir):
                self.logs.append("ERROR: Build directory not found. Please build the project first.")
                return
            
            result = subprocess.run(
                cmd,
                cwd=build_dir,
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode == 0:
                self.logs.append("Simulation completed successfully!")
                self.results = self.parse_simulation_output(result.stdout)
            else:
                self.logs.append(f"Simulation failed: {result.stderr}")
                
        except subprocess.TimeoutExpired:
            self.logs.append("Simulation timed out")
        except Exception as e:
            self.logs.append(f"Error running simulation: {e}")
        finally:
            self.running = False
    
    def parse_simulation_output(self, output):
        """Parse simulation output into structured data"""
        lines = output.strip().split('\n')
        results = {
            'events_processed': 0,
            'trades': 0,
            'execution_time_ms': 0,
            'events_per_second': 0,
            'agent_performance': {}
        }
        
        for line in lines:
            if 'Total Events Processed:' in line:
                results['events_processed'] = int(line.split(':')[1].strip().replace(',', ''))
            elif 'Total Trades:' in line:
                results['trades'] = int(line.split(':')[1].strip().replace(',', ''))
            elif 'Execution Time:' in line:
                results['execution_time_ms'] = float(line.split(':')[1].strip().replace('ms', ''))
            elif 'Events per Second:' in line:
                results['events_per_second'] = float(line.split(':')[1].strip())
            elif 'Agent' in line and 'PnL=' in line:
                # Parse agent performance
                parts = line.split(':')
                if len(parts) >= 2:
                    agent_info = parts[1].strip()
                    agent_id = agent_info.split()[1]
                    pnl_start = agent_info.find('PnL=') + 4
                    pnl_end = agent_info.find(',', pnl_start)
                    pnl = float(agent_info[pnl_start:pnl_end])
                    
                    inventory_start = agent_info.find('Inventory=') + 10
                    inventory = int(agent_info[inventory_start:])
                    
                    results['agent_performance'][agent_id] = {
                        'pnl': pnl,
                        'inventory': inventory
                    }
        
        return results

# Global simulation runner
sim_runner = SimulationRunner()

@app.route('/')
def index():
    """Main dashboard page"""
    return render_template('index.html')

@app.route('/api/config')
def get_config():
    """Get default simulation configuration"""
    config = {
        'steps': 10000,
        'seed': 42,
        'maker_spread': 2,
        'maker_quantity': 50,
        'taker_intensity': 0.8,
        'taker_quantity_mean': 40,
        'noise_intensity': 1.5,
        'noise_quantity_mean': 30
    }
    return jsonify(config)

@app.route('/api/simulate', methods=['POST'])
def run_simulation():
    """Run simulation with given configuration"""
    global simulation_running
    
    if simulation_running:
        return jsonify({'error': 'Simulation already running'}), 400
    
    config = request.json
    
    # Start simulation in background thread
    simulation_running = True
    thread = threading.Thread(target=sim_runner.run_simulation, args=(config,))
    thread.daemon = True
    thread.start()
    
    return jsonify({'message': 'Simulation started'})

@app.route('/api/status')
def get_status():
    """Get simulation status"""
    return jsonify({
        'running': sim_runner.running,
        'results': sim_runner.results,
        'logs': sim_runner.logs[-10:]  # Last 10 log entries
    })

@app.route('/api/stop', methods=['POST'])
def stop_simulation():
    """Stop running simulation"""
    global simulation_running
    simulation_running = False
    sim_runner.running = False
    return jsonify({'message': 'Simulation stopped'})

@app.route('/api/charts')
def get_charts():
    """Generate sample charts for visualization"""
    
    # Generate sample market data
    timestamps = np.arange(0, 1000, 1)
    mid_prices = 10000 + np.cumsum(np.random.randn(1000) * 0.5)
    spreads = np.random.uniform(1, 5, 1000)
    volumes = np.random.poisson(50, 1000)
    
    # Price chart
    price_chart = go.Scatter(
        x=timestamps,
        y=mid_prices,
        mode='lines',
        name='Mid Price',
        line=dict(color='blue')
    )
    
    # Spread chart
    spread_chart = go.Scatter(
        x=timestamps,
        y=spreads,
        mode='lines',
        name='Bid-Ask Spread',
        line=dict(color='red'),
        yaxis='y2'
    )
    
    # Volume chart
    volume_chart = go.Bar(
        x=timestamps,
        y=volumes,
        name='Volume',
        marker=dict(color='green', opacity=0.7)
    )
    
    layout = go.Layout(
        title='Market Microstructure Dashboard',
        xaxis=dict(title='Time'),
        yaxis=dict(title='Price', side='left'),
        yaxis2=dict(title='Spread', side='right', overlaying='y'),
        hovermode='x unified'
    )
    
    charts = {
        'price_spread': {
            'data': [price_chart, spread_chart],
            'layout': layout
        },
        'volume': {
            'data': [volume_chart],
            'layout': go.Layout(title='Trading Volume', xaxis=dict(title='Time'), yaxis=dict(title='Volume'))
        }
    }
    
    return jsonify(charts)

@app.route('/api/performance')
def get_performance():
    """Get performance metrics"""
    metrics = {
        'cpu_usage': np.random.uniform(10, 80),
        'memory_usage': np.random.uniform(20, 60),
        'events_per_second': np.random.uniform(1000000, 5000000),
        'latency_ms': np.random.uniform(0.1, 2.0)
    }
    return jsonify(metrics)

@app.route('/api/orderbook')
def get_orderbook():
    """Get sample order book data"""
    orderbook = {
        'bids': [
            {'price': 10000, 'quantity': 150},
            {'price': 9999, 'quantity': 200},
            {'price': 9998, 'quantity': 100}
        ],
        'asks': [
            {'price': 10002, 'quantity': 120},
            {'price': 10003, 'quantity': 180},
            {'price': 10004, 'quantity': 90}
        ],
        'last_trade': {
            'price': 10001,
            'quantity': 50,
            'timestamp': datetime.now().isoformat()
        }
    }
    return jsonify(orderbook)

if __name__ == '__main__':
    print("🚀 Starting Market Microstructure Simulator Dashboard")
    print("📊 Dashboard will be available at: http://localhost:5000")
    print("⚡ Make sure the C++ simulator is built in the build/ directory")
    
    app.run(debug=True, host='0.0.0.0', port=5000)
