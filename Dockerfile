# QuteFuzz - Dockerfile for quantum compiler fuzzing
FROM ubuntu:24.04

# Install needed deps
RUN apt-get update && apt-get install -y sudo curl vim && rm -rf /var/lib/apt/lists/*

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Set working directory
WORKDIR /app

# Copy project files
COPY . .

# Run the existing setup script (installs all dependencies and builds)
RUN chmod +x scripts/setup/setup_env.sh && \
    ./scripts/setup/setup_env.sh

# Add Cargo and local bin to PATH
ENV PATH="/root/.cargo/bin:/root/.local/bin:${PATH}"
ENV LLVM_SYS_140_PREFIX="/usr/lib/llvm-14"

# Default command
CMD ["/bin/bash"]
