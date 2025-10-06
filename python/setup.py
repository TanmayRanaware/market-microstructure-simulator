"""
Setup script for Market Microstructure Simulator Python package.
"""

from setuptools import setup, find_packages, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext
import os
import sys

# Read README for long description
def read_readme():
    with open("../README.md", "r", encoding="utf-8") as fh:
        return fh.read()

# Define the extension module
ext_modules = [
    Pybind11Extension(
        "mms_core",
        sources=["mms_bindings.cpp"],
        include_dirs=["../cpp/include"],
        libraries=["mms_core"],
        library_dirs=["../build"],
        language="c++",
        cxx_std=20,
    )
]

setup(
    name="market-microstructure-simulator",
    version="1.0.0",
    author="Market Microstructure Team",
    author_email="team@mms.example.com",
    description="A production-grade market microstructure simulator with C++ engine and Python bindings",
    long_description=read_readme(),
    long_description_content_type="text/markdown",
    url="https://github.com/example/market-microstructure-simulator",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Financial and Insurance Industry",
        "Intended Audience :: Science/Research",
        "Topic :: Office/Business :: Financial :: Investment",
        "Topic :: Scientific/Engineering :: Mathematics",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: C++",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.8",
    install_requires=[
        "numpy>=1.21.0",
        "pandas>=1.3.0",
        "matplotlib>=3.5.0",
        "seaborn>=0.11.0",
        "pybind11>=2.8.0",
    ],
    extras_require={
        "dev": [
            "pytest>=7.0.0",
            "pytest-cov>=3.0.0",
            "black>=22.0.0",
            "flake8>=4.0.0",
            "mypy>=0.950",
            "pre-commit>=2.17.0",
        ],
        "notebooks": [
            "jupyter>=1.0.0",
            "notebook>=6.4.0",
            "ipykernel>=6.0.0",
        ],
        "analysis": [
            "scipy>=1.7.0",
            "scikit-learn>=1.0.0",
            "plotly>=5.0.0",
            "bokeh>=2.4.0",
        ],
    },
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    entry_points={
        "console_scripts": [
            "mms-sim=scripts.run_sim:main",
        ],
    },
    include_package_data=True,
    package_data={
        "mms": ["*.py", "*.pyi"],
    },
    zip_safe=False,
)
