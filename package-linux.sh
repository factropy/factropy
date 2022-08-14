#!/bin/bash

make clean
rm -rf factropy-linux*

make -j$(grep processor /proc/cpuinfo | wc -l) linux2

mkdir -p factropy-linux/programs
cp -r factropy assets models font shader scenario LICENSE README.md factropy-linux/

zip -r factropy-linux.zip factropy-linux/
