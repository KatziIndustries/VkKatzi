#/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

rm $SCRIPT_DIR/compiled/*

glslc $SCRIPT_DIR/src/vertex.vert -o $SCRIPT_DIR/compiled/vertex.vert.spv
glslc $SCRIPT_DIR/src/fragment.frag -o $SCRIPT_DIR/compiled/fragment.frag.spv