./configure --host=arm-linux-gnueabihf --prefix=$absolute_source_code_path/out
make -j4
make install

copy out/lib/* to destination
