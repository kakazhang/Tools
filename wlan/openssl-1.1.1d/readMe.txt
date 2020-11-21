./Configure linux-armv4 shared no-asm --prefix=$absolute_source_code_path CROSS_COMPILE=arm-linux-gnueabihf-
make -j4
make install -j4
