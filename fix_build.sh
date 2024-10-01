rm -rf openapi/build
rm -rf openapi/external
mkdir -p openapi/build
cd openapi/build
cmake ..
make -j$(nproc)