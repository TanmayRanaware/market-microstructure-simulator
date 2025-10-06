#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/functional.h>
#include <mms/types.hpp>
#include <mms/order_book.hpp>
#include <mms/matching_engine.hpp>
#include <mms/agents.hpp>
#include <mms/simulator.hpp>

namespace py = pybind11;

PYBIND11_MODULE(mms_core, m) {
    m.doc() = "Market Microstructure Simulator - C++ Core";
    
    // Enums
    py::enum_<mms::Side>(m, "Side")
        .value("BUY", mms::Side::BUY)
        .value("SELL", mms::Side::SELL);
    
    py::enum_<mms::EventType>(m, "EventType")
        .value("LIMIT", mms::EventType::LIMIT)
        .value("MARKET", mms::EventType::MARKET)
        .value("CANCEL", mms::EventType::CANCEL);
    
    // Basic types
    py::class_<mms::Order>(m, "Order")
        .def(py::init<>())
        .def(py::init<mms::OrderId, mms::Side, mms::Price, mms::Qty, mms::Timestamp>())
        .def_readwrite("id", &mms::Order::id)
        .def_readwrite("side", &mms::Order::side)
        .def_readwrite("price", &mms::Order::price)
        .def_readwrite("quantity", &mms::Order::quantity)
        .def_readwrite("timestamp", &mms::Order::timestamp)
        .def("__repr__", [](const mms::Order& o) {
            return "Order(id=" + std::to_string(o.id) + 
                   ", side=" + mms::side_to_string(o.side) +
                   ", price=" + std::to_string(o.price) +
                   ", quantity=" + std::to_string(o.quantity) +
                   ", timestamp=" + std::to_string(o.timestamp) + ")";
        });
    
    py::class_<mms::Trade>(m, "Trade")
        .def(py::init<>())
        .def(py::init<mms::OrderId, mms::OrderId, mms::Price, mms::Qty, mms::Timestamp>())
        .def_readwrite("maker_id", &mms::Trade::maker_id)
        .def_readwrite("taker_id", &mms::Trade::taker_id)
        .def_readwrite("price", &mms::Trade::price)
        .def_readwrite("quantity", &mms::Trade::quantity)
        .def_readwrite("timestamp", &mms::Trade::timestamp)
        .def("__repr__", [](const mms::Trade& t) {
            return "Trade(maker=" + std::to_string(t.maker_id) +
                   ", taker=" + std::to_string(t.taker_id) +
                   ", price=" + std::to_string(t.price) +
                   ", quantity=" + std::to_string(t.quantity) +
                   ", timestamp=" + std::to_string(t.timestamp) + ")";
        });
    
    py::class_<mms::MarketSnapshot>(m, "MarketSnapshot")
        .def(py::init<>())
        .def(py::init<mms::Price, mms::Price, mms::Qty, mms::Qty, mms::Price, mms::Timestamp>())
        .def_readwrite("best_bid", &mms::MarketSnapshot::best_bid)
        .def_readwrite("best_ask", &mms::MarketSnapshot::best_ask)
        .def_readwrite("best_bid_qty", &mms::MarketSnapshot::best_bid_qty)
        .def_readwrite("best_ask_qty", &mms::MarketSnapshot::best_ask_qty)
        .def_readwrite("last_trade_price", &mms::MarketSnapshot::last_trade_price)
        .def_readwrite("timestamp", &mms::MarketSnapshot::timestamp)
        .def("__repr__", [](const mms::MarketSnapshot& s) {
            return "MarketSnapshot(bid=" + std::to_string(s.best_bid) +
                   ", ask=" + std::to_string(s.best_ask) +
                   ", timestamp=" + std::to_string(s.timestamp) + ")";
        });
    
    // OrderBook
    py::class_<mms::OrderBook>(m, "OrderBook")
        .def(py::init<>())
        .def("add_limit_order", &mms::OrderBook::add_limit_order)
        .def("add_market_order", &mms::OrderBook::add_market_order)
        .def("cancel_order", &mms::OrderBook::cancel_order)
        .def("best_bid_price", &mms::OrderBook::best_bid_price)
        .def("best_bid_quantity", &mms::OrderBook::best_bid_quantity)
        .def("best_ask_price", &mms::OrderBook::best_ask_price)
        .def("best_ask_quantity", &mms::OrderBook::best_ask_quantity)
        .def("top_of_book", &mms::OrderBook::top_of_book)
        .def("get_depth", &mms::OrderBook::get_depth, py::arg("levels") = 10)
        .def("size", &mms::OrderBook::size)
        .def("empty", &mms::OrderBook::empty)
        .def("get_order", &mms::OrderBook::get_order)
        .def("clear", &mms::OrderBook::clear)
        .def("last_trade_price", &mms::OrderBook::last_trade_price)
        .def("total_volume", &mms::OrderBook::total_volume)
        .def("trade_count", &mms::OrderBook::trade_count);
    
    // Event
    py::class_<mms::Event>(m, "Event")
        .def(py::init<>())
        .def(py::init<mms::EventType, mms::OrderId, mms::Side, mms::Price, mms::Qty, mms::Timestamp, mms::OrderId>())
        .def_readwrite("type", &mms::Event::type)
        .def_readwrite("order_id", &mms::Event::order_id)
        .def_readwrite("side", &mms::Event::side)
        .def_readwrite("price", &mms::Event::price)
        .def_readwrite("quantity", &mms::Event::quantity)
        .def_readwrite("timestamp", &mms::Event::timestamp)
        .def_readwrite("agent_id", &mms::Event::agent_id);
    
    // MatchingEngine
    py::class_<mms::MatchingEngine>(m, "MatchingEngine")
        .def(py::init<>())
        .def("process_event", &mms::MatchingEngine::process_event)
        .def("process_events", &mms::MatchingEngine::process_events)
        .def("get_market_snapshot", &mms::MatchingEngine::get_market_snapshot)
        .def("get_depth", &mms::MatchingEngine::get_depth, py::arg("levels") = 10)
        .def("order_count", &mms::MatchingEngine::order_count)
        .def("last_trade_price", &mms::MatchingEngine::last_trade_price)
        .def("total_volume", &mms::MatchingEngine::total_volume)
        .def("trade_count", &mms::MatchingEngine::trade_count)
        .def("clear", &mms::MatchingEngine::clear);
    
    // Agent configurations
    py::class_<mms::MarketMaker::Config>(m, "MarketMakerConfig")
        .def(py::init<>())
        .def_readwrite("spread", &mms::MarketMaker::Config::spread)
        .def_readwrite("quantity", &mms::MarketMaker::Config::quantity)
        .def_readwrite("refresh_interval", &mms::MarketMaker::Config::refresh_interval)
        .def_readwrite("max_inventory", &mms::MarketMaker::Config::max_inventory)
        .def_readwrite("inventory_penalty", &mms::MarketMaker::Config::inventory_penalty);
    
    py::class_<mms::Taker::Config>(m, "TakerConfig")
        .def(py::init<>())
        .def_readwrite("intensity", &mms::Taker::Config::intensity)
        .def_readwrite("side_bias", &mms::Taker::Config::side_bias)
        .def_readwrite("quantity_mean", &mms::Taker::Config::quantity_mean)
        .def_readwrite("quantity_std", &mms::Taker::Config::quantity_std)
        .def_readwrite("use_market_orders", &mms::Taker::Config::use_market_orders);
    
    py::class_<mms::NoiseTrader::Config>(m, "NoiseTraderConfig")
        .def(py::init<>())
        .def_readwrite("limit_intensity", &mms::NoiseTrader::Config::limit_intensity)
        .def_readwrite("cancel_intensity", &mms::NoiseTrader::Config::cancel_intensity)
        .def_readwrite("quantity_mean", &mms::NoiseTrader::Config::quantity_mean)
        .def_readwrite("quantity_std", &mms::NoiseTrader::Config::quantity_std)
        .def_readwrite("price_volatility", &mms::NoiseTrader::Config::price_volatility)
        .def_readwrite("cancel_probability", &mms::NoiseTrader::Config::cancel_probability);
    
    // Simulation configuration
    py::class_<mms::SimulationConfig>(m, "SimulationConfig")
        .def(py::init<>())
        .def_readwrite("seed", &mms::SimulationConfig::seed)
        .def_readwrite("start_time", &mms::SimulationConfig::start_time)
        .def_readwrite("time_step", &mms::SimulationConfig::time_step)
        .def_readwrite("max_steps", &mms::SimulationConfig::max_steps)
        .def_readwrite("enable_logging", &mms::SimulationConfig::enable_logging)
        .def_readwrite("output_dir", &mms::SimulationConfig::output_dir);
    
    // RunResult
    py::class_<mms::Simulator::RunResult>(m, "RunResult")
        .def_readonly("trades", &mms::Simulator::RunResult::trades)
        .def_readonly("market_snapshots", &mms::Simulator::RunResult::market_snapshots)
        .def_readonly("agent_pnl", &mms::Simulator::RunResult::agent_pnl)
        .def_readonly("total_events_processed", &mms::Simulator::RunResult::total_events_processed)
        .def_readonly("total_trades", &mms::Simulator::RunResult::total_trades)
        .def_readonly("simulation_duration", &mms::Simulator::RunResult::simulation_duration)
        .def_readonly("simulation_time_seconds", &mms::Simulator::RunResult::simulation_time_seconds);
    
    // Simulator
    py::class_<mms::Simulator>(m, "Simulator")
        .def(py::init<>())
        .def(py::init<const mms::SimulationConfig&>())
        .def("run", py::overload_cast<size_t, const mms::MarketMaker::Config&, const mms::Taker::Config&, const mms::NoiseTrader::Config&>(&mms::Simulator::run))
        .def("run_with_agents", &mms::Simulator::run_with_agents)
        .def("reset", &mms::Simulator::reset)
        .def("update_config", &mms::Simulator::update_config)
        .def("set_data_collection", &mms::Simulator::set_data_collection)
        .def("set_output_dir", &mms::Simulator::set_output_dir);
    
    // RNG
    py::class_<mms::RNG>(m, "RNG")
        .def(py::init<>())
        .def(py::init<uint64_t>())
        .def("uniform_int", [](mms::RNG& self, int min, int max) {
            return self.uniform_int(min, max);
        })
        .def("uniform_real", py::overload_cast<>(&mms::RNG::uniform_real))
        .def("uniform_real", py::overload_cast<double, double>(&mms::RNG::uniform_real))
        .def("exponential", &mms::RNG::exponential)
        .def("normal", &mms::RNG::normal)
        .def("poisson", &mms::RNG::poisson)
        .def("bernoulli", &mms::RNG::bernoulli)
        .def("seed", &mms::RNG::seed);
    
    // Utility functions
    m.def("side_to_string", &mms::side_to_string);
    m.def("event_type_to_string", &mms::event_type_to_string);
    m.def("get_mid_price", &mms::get_mid_price);
    m.def("get_spread", &mms::get_spread);
    m.def("generate_time_seed", &mms::generate_time_seed);
    
    // Analysis functions
    m.def("calculate_vwap", &mms::analysis::calculate_vwap);
    m.def("calculate_twap", &mms::analysis::calculate_twap);
    m.def("calculate_realized_volatility", &mms::analysis::calculate_realized_volatility);
}
