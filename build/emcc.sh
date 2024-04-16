#!/bin/bash

rm -rf bin
mkdir bin
cd bin || exit

proj_name="Voxel"
proj_root_dir=$(pwd)/../

flags=(
    -w -O3 -s WASM=1 -s USE_WEBGL2=1 -s ASYNCIFY  -s ALLOW_MEMORY_GROWTH=1 -Wno-incompatible-function-pointer-types -s EXPORTED_FUNCTIONS="['_main','_malloc','_free']" -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall','cwrap']"
)

# Include directories
inc=(
    -I ../include/           # Gunslinger includes
)

# Source files
src=(
    ../tut/main.c
)

libs=(
)

# Build
emcc "${inc[@]}" "${src[@]}" "${flags[@]}" -o "$proj_name.html"

cd ..
