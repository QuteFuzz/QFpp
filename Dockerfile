# base image
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. Install System Dependencies
RUN apt update && apt install -y \
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
    libffi-dev \
    && apt clean && rm -rf /var/lib/apt/lists/*

# 2. Install Rust and Cargo
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV PATH="/root/.cargo/bin:$PATH"

# 3. Install uv
# We use the specific installer to get the binary directly
COPY --from=ghcr.io/astral-sh/uv:latest /uv /bin/uv

# 4. Configure Python Environment (The "uv way")
# Ubuntu 24.04 blocks system-wide pip installs (PEP 668).
# We create a virtual environment in /opt/venv and add it to PATH.
ENV VIRTUAL_ENV=/opt/venv
RUN uv venv $VIRTUAL_ENV
ENV PATH="$VIRTUAL_ENV/bin:$PATH"

# 5. Set LLVM environment variable for Rust packages
ENV LLVM_SYS_140_PREFIX="/usr/lib/llvm-14"

# 6. Install PyPI Packages
# We use 'uv pip install' (since we are in a venv).
# Note: Added --no-cache to keep image size down, though uv is very efficient.
RUN uv pip install --no-cache \
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

# 7. Install git dependencies
RUN uv pip install --no-cache git+https://github.com/CQCL/hugr-qir.git

# 8. Clone and build qir-runner
# We use uv to install the python bindings directly from the local folder
RUN git clone https://github.com/CQCL/qir-runner.git /tmp/qir-runner
WORKDIR /tmp/qir-runner
RUN cargo build --release
# Install the python bindings located in the 'pip' subdirectory
RUN uv pip install --no-cache ./pip && \
    cd /tmp && rm -rf qir-runner

# 9. Setup Working Directory
ARG WORKDIR_PATH=/qutefuzz
WORKDIR ${WORKDIR_PATH}

# Copy project files last to maximize caching
COPY . .

ENV PYTHONPATH=${WORKDIR_PATH}

CMD ["/bin/bash"]