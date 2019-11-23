#!/bin/bash -ev

PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g')

cd depends

echo "Downloading Windows dependencies"
make download-win -j4
echo "Building Windows dependencies"
make HOST=x86_64-w64-mingw32 -j4
cd ..

python $PWD/scripts/mkpkg.py -n boost -p $PWD/depends/x86_64-w64-mingw32 -v 1.70.0 -d "Boost System dep" $PWD/depends/x86_64-w64-mingw32 -o $PWD/depends/x86_64-w64-mingw32/lib/pkgconfig/boost.pc
#python $PWD/scripts/mkpkg.py -n boost-filesystem -p $PWD/depends/x86_64-w64-mingw32 -v 1.70.0 -d "Boost FileSystem dep" $PWD/depends/x86_64-w64-mingw32 -o $PWD/depends/x86_64-w64-mingw32/lib/pkgconfig/libboost-filesystem.pc
#python $PWD/scripts/mkpkg.py -n boost-thread -p $PWD/depends/x86_64-w64-mingw32 -v 1.70.0 -d "Boost Thread dep" $PWD/depends/x86_64-w64-mingw32 -o $PWD/depends/x86_64-w64-mingw32/lib/pkgconfig/libboost-thread.pc
#python $PWD/scripts/mkpkg.py -n boost-chrono -p $PWD/depends/x86_64-w64-mingw32 -v 1.70.0 -d "Boost Chrono dep" $PWD/depends/x86_64-w64-mingw32 -o $PWD/depends/x86_64-w64-mingw32/lib/pkgconfig/libboost-chrono.pc

echo "Configuring GalaxyCash for Windows"
$PWD/autogen.sh
CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site $PWD/configure --with-boost=$PWD/depends/x86_64-w64-mingw32 --with-boost-libdir=$PWD/depends/x86_64-w64-mingw32/lib CXXFLAGS="-std=c++17" --enable-shared=no --enable-static=yes --enable-dependency-tracking -prefix=$PWD/build-host/win64