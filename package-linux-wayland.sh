#!/bin/bash

set -e

make clean
rm -rf factropy-linux-wayland*

make -j$(grep processor /proc/cpuinfo | wc -l) linux2-wayland

mkdir -p factropy-linux-wayland/programs
cp -r factropy assets models font shader scenario LICENSE README.md factropy-linux-wayland/

zip -r factropy-linux-wayland.zip factropy-linux-wayland/

