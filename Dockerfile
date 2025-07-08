FROM ubuntu:24.04 AS sdk_setup

ENV http_proxy=http://proxy.s1n:3130
ENV https_proxy=http://proxy.s1n:3130
ENV HTTP_PROXY=http://proxy.s1n:3130
ENV HTTPS_PROXY=http://proxy.s1n:3130


RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install --no-install-recommends -y \
                       git \
                       ca-certificates \
                       python3 \
                       tar \
                       build-essential \
                       gcc-arm-none-eabi \
                       libnewlib-arm-none-eabi \
                       libstdc++-arm-none-eabi-newlib \
                       cmake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Raspberry Pi Pico SDK
ARG SDK_PATH=/usr/local/picosdk
RUN git clone --depth 1 --branch 2.1.1 https://github.com/raspberrypi/pico-sdk $SDK_PATH && \
    cd $SDK_PATH && \
    git submodule update --init

ENV PICO_SDK_PATH=$SDK_PATH

# Picotool installation
RUN git clone --depth 1 --branch 2.1.1 https://github.com/raspberrypi/picotool.git /home/picotool && \
    cd /home/picotool && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j$(nproc) && \
    cmake --install . && \
    rm -rf /home/picotool

USER ubuntu
WORKDIR /home/ubuntu