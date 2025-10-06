# Multi-stage Docker build for Market Microstructure Simulator

# Stage 1: Build dependencies
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    python3-dev \
    libpython3-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install Python dependencies
COPY requirements.txt /tmp/requirements.txt
RUN pip3 install --no-cache-dir -r /tmp/requirements.txt

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the project
RUN mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_TESTS=ON \
          -DBUILD_PYTHON=ON \
          -DBUILD_EXAMPLES=ON \
          .. && \
    make -j$(nproc)

# Run tests
RUN cd build && ctest --output-on-failure

# Build Python wheel
RUN python3 -m build --wheel

# Stage 2: Runtime image
FROM ubuntu:22.04 AS runtime

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -u 1000 mms && \
    mkdir -p /app/output && \
    chown -R mms:mms /app

# Set working directory
WORKDIR /app

# Copy Python wheel from builder stage
COPY --from=builder /app/dist/*.whl /tmp/

# Install the wheel
RUN pip3 install --no-cache-dir /tmp/*.whl

# Copy built binaries from builder stage
COPY --from=builder /app/build/simple_sim /usr/local/bin/
COPY --from=builder /app/build/benchmark /usr/local/bin/

# Copy scripts
COPY scripts/ /usr/local/bin/
RUN chmod +x /usr/local/bin/run_sim.py

# Switch to non-root user
USER mms

# Set environment variables
ENV PYTHONPATH=/app
ENV PATH="/usr/local/bin:${PATH}"

# Create output directory
RUN mkdir -p /app/output

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD python3 -c "import mms; print('Health check passed')" || exit 1

# Default command
CMD ["python3", "/usr/local/bin/run_sim.py", "--help"]

# Labels for metadata
LABEL maintainer="Market Microstructure Team"
LABEL version="1.0.0"
LABEL description="Market Microstructure Simulator with C++ engine and Python bindings"
