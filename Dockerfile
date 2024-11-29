# Stage 2: Build the Application
FROM ubuntu:latest

RUN apt-get -y update && \
    apt-get install -y cmake git g++ libcurl4-openssl-dev pkg-config \
    build-essential \
    libwebsockets-dev \
    nlohmann-json3-dev \
    meson \
    && rm -rf /var/lib/apt/lists/*

# Copy the source code
# Add your source files here
WORKDIR /usr/src
COPY ./src ./src
COPY ./pdfium ./pdfium
COPY CMakeLists.txt CMakeLists.txt

# Use DockerCMakeLists.txt for building
RUN mkdir -p build && \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel $(nproc)

# Expose port 8080
EXPOSE 8080

# Run the application
CMD ["/usr/src/build/api-server"]
