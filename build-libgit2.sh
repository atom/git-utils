#!/bin/sh

cd deps/libgit2
touch .npmignore
mkdir build
cd build
cmake .. -DCMAKE_C_FLAGS="$1" -DCMAKE_BUILD_TYPE=Release -DTHREADSAFE=1 \
         -DBUILD_CLAR=0 -DBUILD_SHARED_LIBS=0 -DZLIB_LIBRARY=1 \
         -DSHA1_TYPE=builtin
cmake --build .
