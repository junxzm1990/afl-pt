#!/bin/bash
sudo apt-get install gawk
mkdir build
cd build
../configure --prefix=/
make -j16
