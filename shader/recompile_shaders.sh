#/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

rm $SCRIPT_DIR/compiled/*

glslc $SCRIPT_DIR/src/vertex.vert -o $SCRIPT_DIR/compiled/vert.spv
glslc $SCRIPT_DIR/src/fragment.frag -o $SCRIPT_DIR/compiled/frag.spv

glslc $SCRIPT_DIR/src/instanced.vert -o $SCRIPT_DIR/compiled/instancedVert.spv
glslc $SCRIPT_DIR/src/instanced.frag -o $SCRIPT_DIR/compiled/instancedFrag.spv