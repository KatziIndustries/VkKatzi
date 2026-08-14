#!/usr/bin/env bash

set -e

mkdir -p build/windows

gcc -shared -fPIC \
    vulkan.c logger.c \
    -o build/windows/libvkkatzi.dll \
    -Wl,--out-implib,build/libvkkatzi.dll.a \
    -lvulkan

# echo "Building GLFW..."
# gcc -shared -fPIC \
#     glfw.c \
#     -o build/windows/vkkatzi_glfw.dll \
#     -Lbuild \
#     -lvkkatzi \
#     -lglfw3

gcc -shared -fPIC \
    sdl.c \
    -o build/windows/libvkkatzi_sdl3.dll \
    -Lbuild \
    -lvkkatzi \
    -lSDL3

rm build/libvkkatzi.dll.a
