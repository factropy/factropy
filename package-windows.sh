#!/bin/bash

set -e

make clean
rm -rf factropy-windows*

make -j$(grep processor /proc/cpuinfo | wc -l) -f Makefile.msys

mkdir -p factropy-windows/programs
cp -r factropy.exe assets models font shader scenario LICENSE README.md factropy-windows/

for lib in $(ldd factropy.exe | grep mingw64/bin | awk '{print $1}'); do
	echo $lib; cp /mingw64/bin/$lib factropy-windows/;
done

zip -r factropy-windows.zip factropy-windows/

