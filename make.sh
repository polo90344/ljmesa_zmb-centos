#!/bin/bash
rm -rf build
meson build -Dprefix=$MESA_INSTALL --buildtype debug -Dglx=disabled -Dgbm=enabled -Dplatforms= -Degl-native-platform=drm  -Dvulkan-drivers=  -Dgallium-drivers=ljmgpu -Dllvm=disabled -Dtools=glsl,drm-shim
ninja -C build
ninja -C build install
