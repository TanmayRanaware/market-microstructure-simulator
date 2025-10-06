"""
Market Microstructure Simulator - Python Package

A production-grade market microstructure simulator with C++ engine and Python bindings.
"""

from .core import Simulator, SimulationConfig
from .core import MarketMakerConfig, TakerConfig, NoiseTraderConfig
from .core import Side, EventType, Order, Trade, MarketSnapshot
from .strategies import SimpleStrategy, MeanReversionStrategy, MomentumStrategy
from .utils import create_sample_agents, plot_results, save_results

__version__ = "1.0.0"
__author__ = "Market Microstructure Team"

__all__ = [
    "Simulator",
    "SimulationConfig", 
    "MarketMakerConfig",
    "TakerConfig", 
    "NoiseTraderConfig",
    "Side",
    "EventType",
    "Order",
    "Trade", 
    "MarketSnapshot",
    "SimpleStrategy",
    "MeanReversionStrategy",
    "MomentumStrategy",
    "create_sample_agents",
    "plot_results",
    "save_results"
]
