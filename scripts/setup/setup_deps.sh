#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

echo ">>> 1. Installing System Dependencies (requires sudo)..."
sudo apt-get update
sudo apt-get install -y \
    clang \
    cmake \
    graphviz \
    git \
    curl \
    build-essential \
    python3 \
    python3-venv \
    gdb \
    llvm-14 \
    llvm-14-dev \
    libllvm14 \
    libpolly-14-dev \
    zlib1g-dev \
    libxml2-dev \
    libncurses-dev \
    libffi-dev

echo ">>> 2. Setting up Environment Variables..."
# Crucial for Rust packages linking against LLVM 14 (like qir-runner)
export LLVM_SYS_140_PREFIX="/usr/lib/llvm-14"
export PATH="$HOME/.cargo/bin:$HOME/.local/bin:$PATH"

echo ">>> 3. Installing Rust (if missing)..."
if ! command -v cargo &> /dev/null; then
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
    source "$HOME/.cargo/env"
else
    echo "Rust is already installed."
fi

echo ">>> 4. Installing uv (if missing)..."
if ! command -v uv &> /dev/null; then
    curl -LsSf https://astral.sh/uv/install.sh | sh
else
    echo "uv is already installed."
fi

echo ">>> 5. Initializing uv Project..."
# Initialize only if pyproject.toml doesn't exist
if [ ! -f "pyproject.toml" ]; then
    uv init --no-workspace
fi

# Create virtual environment if it doesn't exist
if [ ! -d ".venv" ]; then
    uv venv
fi

echo ">>> 6. Building 'qir-runner' from source..."
# qir-runner is special; it requires a manual build step before python installation
# Check for Cargo.toml to ensure repo is actually cloned
if [ ! -f "libs/qir-runner" ]; then
    echo "Cloning qir-runner repository..."
    rm -rf libs/qir-runner  # Remove any empty/incomplete directory
    mkdir -p libs
    git clone https://github.com/CQCL/qir-runner.git libs/qir-runner
else
    echo "qir-runner repository already cloned."
fi

pushd libs/qir-runner

# Build the Rust binary (warning about lifetime is harmless)
echo "Building qir-runner Rust components..."
cargo build --release

# Install the Python package directly into the uv environment
echo "Installing qir-runner Python package..."
cd pip
uv pip install -e .

popd

echo ">>> 7. Installing Python Dependencies..."
# Use uv pip install to avoid workspace conflicts
# This installs directly into the virtual environment

# A. Standard PyPI packages
uv pip install \
    pytket \
    qiskit \
    pytket-qiskit \
    matplotlib \
    sympy \
    z3-solver \
    cirq \
    tket2 \
    pytket-qir \
    qnexus \
    tket \
    selene-sim \
    guppylang

# B. Git dependencies
uv pip install git+https://github.com/CQCL/hugr-qir.git

# C. Install local qutefuzz package in editable mode
uv pip install -e .

# If running in GitHub Actions, save these variables for future steps
if [ -n "$GITHUB_ENV" ]; then
    echo "LLVM_SYS_140_PREFIX=$LLVM_SYS_140_PREFIX" >> "$GITHUB_ENV"
    echo "$HOME/.cargo/bin" >> $GITHUB_PATH
    echo "$HOME/.local/bin" >> $GITHUB_PATH
fi

echo ">>> Setup Complete!"
echo "Run your code using: uv run scripts/ci_pipeline.py"