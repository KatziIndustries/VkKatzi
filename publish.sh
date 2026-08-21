mkdir -p build

VERSION="0.1.0"

gcc -shared -fPIC src/* logger.c -o build/libvkkatzi-$VERSION.so -lvulkan
gcc -shared -fPIC sdl.c -o build/libvkkatzi_sdl3-$VERSION.so -Lbuild -lvkkatzi -lSDL3
