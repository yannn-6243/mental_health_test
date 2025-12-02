# Build stage
FROM ubuntu:22.04 AS builder

# Avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    make \
    libsqlite3-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libasio-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Download Crow (header-only library)
WORKDIR /app
RUN wget --no-check-certificate https://github.com/CrowCpp/Crow/releases/download/v1.2.0/crow_all.h -O crow_all.h || \
    wget --no-check-certificate https://crowcpp.org/master/crow_all.h -O crow_all.h

# Copy source code from backend folder
COPY backend/main.cpp .

# Compile
RUN g++ -std=c++17 -O2 -o server main.cpp \
    -lsqlite3 -lpthread \
    -I/usr/include

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binary from builder
COPY --from=builder /app/server .

# Create data directory for SQLite
RUN mkdir -p /app/data

# Expose port (Railway will set PORT env var)
EXPOSE 8080

# Run the server
CMD ["./server"]
