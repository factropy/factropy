#!/bin/bash

make clean
rm -rf factropy-demo-windows*

make -j$(grep processor /proc/cpuinfo | wc -l) -f Makefile.msys

mkdir -p factropy-demo-windows/programs
cp -r factropy.exe assets models font shader scenario LICENSE README.md factropy-demo-windows/

for lib in $(ldd factropy.exe | grep mingw64/bin | awk '{print $1}'); do
	echo $lib; cp /mingw64/bin/$lib factropy-demo-windows/;
done

zip -r factropy-demo-windows.zip factropy-demo-windows/

