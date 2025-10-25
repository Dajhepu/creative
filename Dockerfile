# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    make \
    git \
    libssl-dev \
    libboost-system-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    libboost-log-dev \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone, build, and install tgbot-cpp
RUN git clone https://github.com/reo7sp/tgbot-cpp.git && \
    cd tgbot-cpp && \
    cmake . -DCMAKE_INSTALL_PREFIX=/usr && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    cd / && \
    rm -rf /tgbot-cpp

WORKDIR /app

# Clone sqlite_orm into the vendor directory
RUN git clone https://github.com/fnc12/sqlite_orm.git vendor/sqlite_orm

# Copy the rest of the bot source code
COPY . .

# Build the bot executable
RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

# Stage 2: Runtime environment
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.74.0 \
    libcurl4 \
    zlib1g \
    libboost-log1.74.0 \
    libsqlite3-0 \
    python3-pip \
    python-is-python3 \
    ffmpeg \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy necessary files from the builder stage
COPY --from=builder /app/build/TgYoutubeBot .
COPY start.sh .

# Make the start script executable
RUN chmod +x ./start.sh

RUN mkdir -p downloads && chmod 777 downloads

# Set BOT_TOKEN and ADMIN_USER_ID in the Railway.app service configuration.
CMD ["./start.sh"]
