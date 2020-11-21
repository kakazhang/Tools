1.cd wpa_supplicant/
2.cp defconfig .config
3.change .config:
  CC = arm-linux-gnueabihf-gcc
  CFLAGS += -I$openssl_include
  LIBS += -L$openssl_lib -lssl -lcrypto
  CFLAGS += -I$libnl/include/libnl3
  LIBS += -L$libnl/lib
4.export PKG_CONFIG_PATH=$out_libnl/lib/pkgconfig:$PKG_CONFIG_PATH
5.make -j4
6.make install
