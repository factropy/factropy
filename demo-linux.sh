#!/bin/bash

make clean
rm -rf factropy-demo-linux*

make -j$(grep processor /proc/cpuinfo | wc -l) linux2

mkdir -p factropy-demo-linux/programs
cp -r factropy assets models font shader scenario LICENSE README.md factropy-demo-linux/

zip -r factropy-demo-linux.zip factropy-demo-linux/
