#!/bin/bash -ev

PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g')

export GCH_PATH=$PWD
mkdir -p $GCH_PATH/build-host/debian

echo "Configuring GalaxyCash for Debian"
$GCH_PATH/autogen.sh
$GCH_PATH/configure CXXFLAGS="-std=c++17 --param ggc-min-expand=1 --param ggc-min-heapsize=32768 -O2" --enable-shared=no --enable-static=yes --enable-glibc-back-compat=yes --with-gui=qt5 -prefix=$GCH_PATH/build-host/debian

