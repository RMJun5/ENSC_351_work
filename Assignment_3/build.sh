rm -rf build
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=$(pwd)/aarch64-toolchain.cmake
cmake --build build