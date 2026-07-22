FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

# Install system dependencies and build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    g++ \
    unzip \
    libgmp-dev\
    libntl-dev\
    libssl-dev\
    libsodium-dev\
    pkg-config\
    python3 \
    python3-pip \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# csv is included in Python's standard library.
RUN python3 -m pip install --no-cache-dir pandas matplotlib

WORKDIR /opt

COPY ./emp-tool /opt/emp-tool/
COPY ./emp-ot /opt/emp-ot/
COPY ./emp-zk /opt/emp-zk/

RUN cd emp-tool\
    && cmake -B build\
    && cmake --build build -j$(nproc)\
    && cmake --install build\
    && cd .. \
    && cd emp-ot\
    && cmake -B build\
    && cmake --build build -j$(nproc)\
    && cmake --install build\
    && cd ..\
    && cd emp-zk\
    && cmake -B build\
    && cmake --build build -j$(nproc)\
    && cmake --install build\
    && cd ..\
    && rm -rf emp-tool emp-ot emp-zk

# Create and set the working directory inside the container
WORKDIR /zkcec

# Copy all files from the host directory to /zkcec inside the container
COPY ./ZKCEC /zkcec/

RUN cd /zkcec\
    && cmake -G Ninja -B build\
    && cmake --build build -j$(nproc)\
    && chmod +x /zkcec/run_experiment.sh

CMD ["/zkcec/run_experiment.sh"]
