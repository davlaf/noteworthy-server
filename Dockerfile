# Stage 1: Build the application
FROM ubuntu:latest AS builder

# Cache the apt-get update and install steps by combining them into one RUN instruction
RUN apt-get -y update && \
    apt-get install -y cmake pkg-config git g++ libcurl4-openssl-dev meson && \
    rm -rf /var/lib/apt/lists/*

# Copy the source code
COPY . /usr/src/app

# Set the working directory
WORKDIR /usr/src/app

# Build the application
RUN mkdir build && \
    cmake -Bbuild -H. && \
    cmake --build build --parallel $(nproc)

# Stage 2: Create the runtime image
FROM ubuntu:latest

# Install only runtime dependencies
RUN apt-get -y update && \
    apt-get install -y libcurl4-openssl-dev && \
    rm -rf /var/lib/apt/lists/*

# Copy the compiled binary from the builder stage
COPY --from=builder /usr/src/app/build/api-server /usr/local/bin/api-server

# Copy the Pistache library and header files from the builder stage
COPY --from=builder /usr/src/app/external/lib/libpistache.so.0.4 /usr/local/lib/libpistache.so.0.4
COPY --from=builder /usr/src/app/external/include/pistache /usr/local/include/pistache

# Ensure the library can be found
RUN ldconfig

# Expose port 8080
EXPOSE 8080

# Run the application
CMD ["api-server"]
