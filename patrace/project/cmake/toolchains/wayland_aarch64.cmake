SET(CMAKE_SYSTEM_NAME Linux)

SET(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
SET(WINDOWSYSTEM wayland)
SET(ARCH aarch64)

set(CC_HOST "aarch64-linux-gnu")
set(ENV{PKG_CONFIG_PATH} "/usr/lib/aarch64-linux-gnu/pkgconfig")
