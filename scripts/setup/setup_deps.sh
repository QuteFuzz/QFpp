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
    uv init
fi


if [ ! -f "uv.lock" ]; then
    # Add local directory in editable mode
    uv add --dev --editable .
fi

echo ">>> 6. Building 'qir-runner' from source..."
# qir-runner is special; it requires a manual build step before python installation
if [ ! -d "libs/qir-runner" ]; then
    mkdir -p libs
    git clone https://github.com/CQCL/qir-runner.git libs/qir-runner
    
    pushd libs/qir-runner
    
    # Build the Rust binary
    cargo build --release
    
    popd
fi

echo ">>> 7. Adding Python Dependencies..."
# We use 'uv add' instead of 'pip install' so they are saved to pyproject.toml

# A. Standard PyPI packages
uv add \
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
# Note: Syntax is "package @ git+url"
uv add "hugr-qir @ git+https://github.com/CQCL/hugr-qir.git"

# C. The local qir-runner binding we just built
# We point to the 'pip' subdirectory where the python package lives
uv add "./libs/qir-runner/pip"

# If running in GitHub Actions, save these variables for future steps
if [ -n "$GITHUB_ENV" ]; then
    echo "LLVM_SYS_140_PREFIX=$LLVM_SYS_140_PREFIX" >> "$GITHUB_ENV"
    echo "$HOME/.cargo/bin" >> $GITHUB_PATH
    echo "$HOME/.local/bin" >> $GITHUB_PATH
fi

echo ">>> Setup Complete!"
echo "Run your code using: uv run scripts/ci_pipeline.py"