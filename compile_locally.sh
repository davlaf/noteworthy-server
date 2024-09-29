mkdir -p openapi/build
cd openapi/build
cmake ..
make -j$(nproc)