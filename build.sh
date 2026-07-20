#/usr/bin/env bash

shader/recompile_shaders.sh
gcc main.c vulkan.c -lvulkan -lglfw -o main