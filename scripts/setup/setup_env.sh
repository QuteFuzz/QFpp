#!/bin/bash
set -e

echo ">>> 1. Installing System Dependencies..."
sudo apt-get update && sudo apt-get install -y \
    clang cmake graphviz git curl build-essential \
    python3 python3-venv gdb llvm-14 llvm-14-dev \
    libllvm14 libpolly-14-dev zlib1g-dev libxml2-dev \
    libncurses-dev libffi-dev

echo ">>> 2. Setting up Environment Variables..."
export LLVM_SYS_140_PREFIX="/usr/lib/llvm-14"
# Add Cargo to path for the current session
export PATH="$HOME/.cargo/bin:$HOME/.local/bin:$PATH"

echo ">>> 3. Installing Rust & uv..."
if ! command -v cargo &> /dev/null; then
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
    source "$HOME/.cargo/env"
fi

if ! command -v uv &> /dev/null; then
    curl -LsSf https://astral.sh/uv/install.sh | sh
fi

echo ">>> 4. Preparing Local Dependencies (qir-runner)..."
# We still need to fetch the repo, but uv will handle the installation logic
if [ ! -d "libs/qir-runner" ]; then
    mkdir -p libs
    git clone https://github.com/CQCL/qir-runner.git libs/qir-runner

    # Pre-build the Rust binary (Keep this if qir-runner requires manual compilation)
    # If qir-runner uses standard maturin/setuptools-rust, uv might be able to
    # build it automatically without this step, but keeping it is safer for now.
    pushd libs/qir-runner
    cargo build --release
    popd
fi

echo ">>> 5. Setting up conan..."

# Install Conan if not present
if ! command -v conan &> /dev/null; then
    echo "Installing Conan..."
    uv tool install conan
fi

# Check if default profile exists, create if it doesn't
if ! conan profile show &> /dev/null; then
    echo "Creating default Conan profile..."
    conan profile detect
else
    echo "Default Conan profile already exists, skipping..."
fi

# Check if tket-libs remote exists, add if it doesn't
if ! conan remote list | grep -q "tket-libs"; then
    echo "Adding tket-libs remote..."
    conan remote add tket-libs https://quantinuumsw.jfrog.io/artifactory/api/conan/tket1-libs --index 0
else
    echo "tket-libs remote already exists, skipping..."
fi

echo ">>> 6. Syncing Python Environment..."
uv sync

# If in CI, export the paths
if [ -n "$GITHUB_ENV" ]; then
    echo "LLVM_SYS_140_PREFIX=$LLVM_SYS_140_PREFIX" >> "$GITHUB_ENV"
    echo "$HOME/.cargo/bin" >> $GITHUB_PATH
fi


echo ">>> Setup Complete!"
echo "Run using: uv run scripts/run.py"
