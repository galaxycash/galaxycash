#!/bin/bash -ev

gcc --version
g++ --version

./contrib/install_db4.sh `pwd`
#flags arent being picked up, so need to link
sudo ln -sf `pwd`/db4/include /usr/local/include/bdb4.8
sudo ln -sf `pwd`/db4/lib/*.a /usr/local/lib
./autogen.sh
./configure --enable-shared=no --enable-static=yes --enable-glibc-back-compat=yes BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" --with-gui=qt5
make -j$(nproc)
make check
