FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04

USER root

RUN apt-get update \
    && apt-get install -y \
        software-properties-common \
        ca-certificates \
        wget \
        gnupg \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && add-apt-repository -y ppa:deadsnakes/ppa \
    && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main" > /etc/apt/sources.list.d/kitware.list \
    && apt-get update \
    && apt-get install -y \
        build-essential \
        libbsd-dev \
        liburing-dev \
        libpq-dev \
    zlib1g-dev \
        ninja-build \
        cmake \
        mold \
        ccache \
        libboost-all-dev \
        python3.14 \
        python3.14-venv \
        libffi-dev \
        python3.14-dev \
        gcc-15 \
        g++-15 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 150 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-15 150 \
    && rm -rf /var/lib/apt/lists/*

RUN id -u vscode >/dev/null 2>&1 || useradd -m -s /bin/bash vscode

ENV CC=gcc-15 CXX=g++-15

USER vscode
