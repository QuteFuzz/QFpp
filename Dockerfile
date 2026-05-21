FROM docker.io/library/ubuntu:24.04

RUN wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb

RUN sudo dpkg -i cuda-keyring_1.1-1_all.deb

RUN apt-get update && apt-get install -y \
    sudo curl vim python3 python3-full python3-venv \
    cuquantum cuquantum-dev cuda-toolkit \ 
    default-jre clang cmake graphviz git build-essential \
    gdb lld ninja-build libopenblas-dev libedit-dev \
    libcurl4-openssl-dev zlib1g-dev libzstd-dev libxml2-dev \
    libncurses-dev libffi-dev gcovr pkg-config \
    && rm -rf /var/lib/apt/lists/*

RUN curl -LsSf https://astral.sh/uv/install.sh | sh

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /app

COPY . .

ENV PATH="/root/.cargo/bin:/root/.local/bin:${PATH}"

CMD ["/bin/bash"]
