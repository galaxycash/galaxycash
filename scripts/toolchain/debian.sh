#!/bin/bash -ev

PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g')

export BASEDIR=$PWD

mkdir -p toolchains
cd toolchains
mkdir -p debian
cd debian
mkdir -p build
mkdir -p downloads
mkdir -p sources

export BUILD=x86_64-linux-gnu
export HOST=x86_64-linux-gnu
export TARGET=x86_64-pc-linux-gnu
export BINUTILS_VERSION=2.33.1
export GCC_VERSION=9.2.0
export PREFIX=$BASEDIR/toolchains/debian
export SUFFIX=-9.2
export BINDIR=$PREFIX/bin
export LIBDIR=$PREFIX/lib
export INCLUDEDIR=$PREFIX/include
export PATH=$PATH:$BINDIR

cd downloads
wget -nc https://mirror.tochlab.net/pub/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.gz
wget -nc http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.gz
cd ..

cd sources
[ -d binutils-${BINUTILS_VERSION} ] || tar -xvf ../downloads/binutils-${BINUTILS_VERSION}.tar.gz
[ -d gcc-${GCC_VERSION} ] || tar -xvf ../downloads/gcc-${GCC_VERSION}.tar.gz

cd gcc-${GCC_VERSION}
[ -d ../../build/gcc ] || contrib/download_prerequisites --force --no-verify
cd ../../

cd build

mkdir -p binutils
cd binutils
../../sources/binutils-${BINUTILS_VERSION}/configure --target=$TARGET --prefix=$PREFIX --enable-gold
make -j6
make install -j6
cd ..


mkdir -p gcc
cd gcc
../../sources/gcc-${GCC_VERSION}/configure --target=$TARGET --prefix=$PREFIX --enable-checking=release --enable-languages=c,c++ --disable-multilib --program-suffix=$SUFFIX
make -j6
make install-strip -j6
cd ..

export LD_LIBRARY_PATH=$PREFIX/lib64:$LD_LIBRARY_PATH