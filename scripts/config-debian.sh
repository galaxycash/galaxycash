#!/bin/bash -ev

PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g')

cd depends
echo "Downloading Linux dependencies"
make download-linux -j6
echo "Building linux-x86_64 dependencies"
make HOST=x86_64-pc-linux-gnu BUILD=x86_64-pc-linux-gnu all -j6
cd ..

echo "Configuring GalaxyCash for Debian"
$PWD/autogen.sh
CONFIG_SITE=$PWD/depends/x86_64-pc-linux-gnu/share/config.site ./configure CXXFLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768 -O2" --enable-shared=no --enable-static=yes --enable-glibc-back-compat=yes --with-gui=qt5 -prefix=`pwd`/depends/x86_64-pc-linux-gnu

