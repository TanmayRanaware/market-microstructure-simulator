#!/bin/bash

echo "🚀 Market Microstructure Simulator Dashboard"
echo "============================================="

# Check if dashboard directory exists
if [ ! -d "dashboard" ]; then
    echo "❌ Dashboard directory not found!"
    exit 1
fi

# Check if C++ simulator is built
if [ ! -f "build/simple_sim" ]; then
    echo "🔨 Building C++ simulator..."
    cd build
    make
    cd ..
fi

echo "✅ C++ simulator ready!"

# Install Python requirements if needed
if [ ! -d "dashboard/venv" ]; then
    echo "📦 Setting up Python environment..."
    cd dashboard
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt
    cd ..
fi

echo "🌐 Starting dashboard server..."
echo "📊 Dashboard will be available at: http://localhost:5000"
echo "⚡ Press Ctrl+C to stop the server"
echo "============================================="

# Start the dashboard
cd dashboard
source venv/bin/activate 2>/dev/null || python3 app.py
