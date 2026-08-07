#/usr/bin/env bash

shader/recompile_shaders.sh
gcc vulkan.c main.c logger.c sdl.c -lvulkan -lm -lSDL3 -o main