rm -rf *.so *.o *.so.*
gcc -shared -fPIC -o libdrm.so.2.4.0 xf86drm.c xf86drmMode.c
ln -s libdrm.so.2.4.0 libdrm.so.2
ln -s libdrm.so.2 libdrm.so
