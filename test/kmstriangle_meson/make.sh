#!/bin/bash
rm -rf ./build
meson build --prefix=$MESA_INSTALL
ninja -C build
