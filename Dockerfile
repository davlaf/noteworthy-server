# Stage 1: Dependency Build Stage
FROM ubuntu:latest AS dependencies

RUN apt-get -y update && \
    apt-get install -y cmake pkg-config git g++ libcurl4-openssl-dev meson && \
    rm -rf /var/lib/apt/lists/*

# Set up external dependencies
WORKDIR /tmp

# Pistache
RUN git clone https://github.com/pistacheio/pistache.git && \
    cd pistache && \
    meson setup build --prefix=/usr/local --libdir=lib && \
    meson install -C build

# nlohmann/json (header-only, so we just need to copy headers)
RUN git clone https://github.com/nlohmann/json.git && \
    cd json && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DJSON_BuildTests=OFF . && \
    make install

# libwebsockets
RUN git clone -b v4.3-stable https://github.com/warmcat/libwebsockets.git && \
    cd libwebsockets && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DLWS_WITH_SHARED=OFF -DLWS_WITH_STATIC=ON -DLWS_WITHOUT_TESTAPPS=ON -DLWS_WITH_SSL=OFF . && \
    make install

# Stage 2: Build the Application
FROM ubuntu:latest AS builder

RUN apt-get -y update && \
    apt-get install -y cmake g++ libcurl4-openssl-dev && \
    rm -rf /var/lib/apt/lists/*
    

# Copy dependencies from the first stage
COPY --from=dependencies /usr/local /usr/local

# Copy the source code
WORKDIR /usr/src
COPY ./src .
COPY DockerCMakeLists.txt CMakeLists.txt

# Use DockerCMakeLists.txt for building
RUN mkdir -p build && \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel $(nproc)


# Stage 3: Runtime Image
FROM ubuntu:latest

RUN apt-get -y update && \
    apt-get install -y libcurl4-openssl-dev && \
    rm -rf /var/lib/apt/lists/*

# Copy the built application and necessary libraries
COPY --from=builder /usr/src/build/api-server /usr/local/bin/api-server
COPY --from=dependencies /usr/local/lib/libpistache.so.0.4 /usr/local/lib/libpistache.so.0.4
COPY --from=dependencies /usr/local/include/pistache /usr/local/include/pistache
COPY --from=dependencies /usr/local/lib/libwebsockets.a /usr/local/lib/libwebsockets.a
COPY --from=dependencies /usr/local/include/nlohmann /usr/local/include/nlohmann

# Ensure the library can be found
RUN ldconfig

# Expose port 8080
EXPOSE 8080

# Run the application
CMD ["api-server"]
