# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Set non-interactive frontend for package installations
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
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

# Set working directory for the bot build
WORKDIR /app

# Copy the bot source code
COPY . .

# Build the bot executable
RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

# Stage 2: Runtime environment
FROM ubuntu:22.04

# Set non-interactive frontend for package installations
ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.74.0 \
    libcurl4 \
    zlib1g \
    libboost-log1.74.0 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install yt-dlp
RUN pip3 install yt-dlp

# Set working directory
WORKDIR /app

# Copy the compiled bot executable from the builder stage
COPY --from=builder /app/build/TgYoutubeBot .

# Create a directory for downloads and set permissions
RUN mkdir -p downloads && chmod 777 downloads

# The BOT_TOKEN environment variable must be set in the Railway.app service configuration.
# ENV BOT_TOKEN=""

# Command to run the bot
CMD ["./TgYoutubeBot"]
