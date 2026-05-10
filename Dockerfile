# QuteFuzz - Dockerfile for quantum compiler fuzzing
FROM ubuntu:24.04

# Install needed deps
RUN apt-get update && apt-get install -y sudo curl vim python3 python3-full python3-venv && rm -rf /var/lib/apt/lists/*

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Set working directory
WORKDIR /app

# Copy project files
COPY . .

# Add Cargo and local bin to PATH so uv can be found
ENV PATH="/root/.cargo/bin:/root/.local/bin:${PATH}"

# Default command
CMD ["/bin/bash"]
