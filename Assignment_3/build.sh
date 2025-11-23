# Unset PKG_CONFIG for ALSA host
rm -rf build
export PKG_CONFIG_SYSROOT_DIR=""
export PKG_CONFIG_PATH=""

cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=$(pwd)/aarch64-toolchain.cmake
cmake --build build