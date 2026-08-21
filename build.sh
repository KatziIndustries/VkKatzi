#/usr/bin/env bash

shader/recompile_shaders.sh
make install -s
gcc main.c -lvkkatzi -lvkkatzi_SDL3 -lSDL3 -o main