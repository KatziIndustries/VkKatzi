mkdir -p build

gcc -shared -fPIC src/* logger.c -o build/libvkkatzi.so -lvulkan
gcc -shared -fPIC sdl.c -o build/libvkkatzi_sdl3.so -Lbuild -lvkkatzi -lSDL3
