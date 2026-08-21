mkdir -p build

VERSION="0.1.0"

gcc -shared -fPIC src/* logger.c -o build/libvkkatzi.so.$VERSION -lvulkan
gcc -shared -fPIC sdl.c -o build/libvkkatzi_sdl3.so.$VERSION -Lbuild -lvkkatzi -lSDL3
