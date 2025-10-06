#!/usr/bin/env python3
"""
Quick start script for the Market Microstructure Simulator Dashboard
"""

import os
import sys
import subprocess

def check_build():
    """Check if the C++ simulator is built"""
    build_dir = os.path.join(os.path.dirname(__file__), '..', 'build')
    simple_sim = os.path.join(build_dir, 'simple_sim')
    
    if not os.path.exists(simple_sim):
        print("❌ C++ simulator not found. Building...")
        os.chdir(build_dir)
        subprocess.run(['make'], check=True)
        print("✅ C++ simulator built successfully!")
    else:
        print("✅ C++ simulator found!")

def install_requirements():
    """Install Python requirements"""
    print("📦 Installing dashboard requirements...")
    subprocess.run([
        sys.executable, '-m', 'pip', 'install', '-r', 
        os.path.join(os.path.dirname(__file__), 'requirements.txt')
    ], check=True)
    print("✅ Requirements installed!")

def main():
    print("🚀 Market Microstructure Simulator Dashboard")
    print("=" * 50)
    
    # Check if C++ simulator is built
    check_build()
    
    # Install requirements
    install_requirements()
    
    print("\n🌐 Starting dashboard server...")
    print("📊 Dashboard will be available at: http://localhost:5000")
    print("⚡ Press Ctrl+C to stop the server")
    print("=" * 50)
    
    # Start the Flask app
    from app import app
    app.run(debug=False, host='0.0.0.0', port=5000)

if __name__ == '__main__':
    main()
