#!/bin/bash

pushd pt
make
# require sudo privilege to insert PT module
./reinstall_ptmod.sh
popd

# Build the customized AFL
pushd afl-2.42b
make
popd

## Build elfpatcher
pushd afl-2.42b/pt_mode/elfpatcher
./bootstrap.sh
./configure
make
popd

## Build pt_proxy
pushd afl-2.42b/pt_mode/pt_proxy
make
popd

## Build customized ld
pushd afl-2.42b/pt_mode/glibc-2.19
mkdir build
cd build
../configure
make -j8
cd ../
popd

## Build testcase cxxfilt
pushd afl-2.42b/test_progs/binutils-2.29
mkdir build
cd build
../configure --enable-shared=no --enable-static=yes
make -j8
cd ../
popd

## Patch testcase cxxfilt
pushd afl-2.42b/pt_mode
./patch-bin.sh ../test_progs/binutils-2.29/build/binutils/cxxfilt
popd

## Fuzzing testcase cxxfilt 
cd afl-2.42b/
./pt-fuzz-fast -P -i ./testcases/others/elf -o ./test_progs/binutils-2.29/build/binutils/cxxfilt_out -- ./test_progs/binutils-2.29/build/binutils/cxxfilt
