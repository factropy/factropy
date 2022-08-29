#!/bin/bash

set -e

make clean
rm -rf factropy-linux-x11*

make -j$(grep processor /proc/cpuinfo | wc -l) linux2-x11

mkdir -p factropy-linux-x11/programs
cp -r factropy assets models font shader scenario LICENSE README.md factropy-linux-x11/

zip -r factropy-linux-x11.zip factropy-linux-x11/

