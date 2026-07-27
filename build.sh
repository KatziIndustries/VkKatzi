#/usr/bin/env bash

shader/recompile_shaders.sh
gcc *.c -lvulkan -lglfw -o main