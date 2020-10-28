# FROM balenalib/jetson-xavier-ubuntu:bionic
FROM arm64v8/ubuntu:bionic

RUN apt-get update \
  && apt-get install -y \
     ca-certificates \
     wget \
     gnupg \
     software-properties-common \
     build-essential \
     cmake \
     clang \
     libc++-dev \
     libc++abi-dev \
     libsdl2-dev \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /home/app/assets

RUN wget https://tokbox.com/downloads/libopentok_linux_llvm_arm64-2.18.0 \
    && tar -xvf libopentok_linux_llvm_arm64-2.18.0 \
    && rm -r libopentok_linux_llvm_arm64-2.18.0 \
    && export LD_LIBRARY_PATH=${PWD}/libopentok_linux_llvm_arm64/lib:$LD_LIBRARY_PATH

WORKDIR /home/app

RUN apt-get update \
  && apt-get install -y \
     alsa-utils \
     pulseaudio \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

ENV CXXFLAGS -pthread
COPY ./common common
COPY ./Custom-Video-Capturer Custom-Video-Capturer


RUN cd Custom-Video-Capturer && rm -rf build && mkdir build && cd build &&  CC=clang CXX=clang++ cmake .. && make

WORKDIR /home/app/Custom-Video-Capturer/build
CMD ["./custom_video_capturer"]
