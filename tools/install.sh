#!/bin/bash

# double check the version of running kernel
if [[ $(uname -r) != 4.4.0* ]]; then
  echo "Running Kernel is not suitable for Ptrix, please double check your environment"
fi

pushd pt
make -j$(nproc)
# require sudo privilege to insert PT module
./reinstall_ptmod.sh
popd

# Build the customized AFL
pushd afl-2.42b
make -j$(nproc)
popd

## Build elfpatcher
pushd afl-2.42b/pt_mode/elfpatcher
./bootstrap.sh
./configure
make -j$(nproc)
popd

## Build pt_proxy
pushd afl-2.42b/pt_mode/pt_proxy
make -j$(nproc)
popd

## Build customized ld
pushd afl-2.42b/pt_mode/glibc-2.19
mkdir build
cd build
../configure --prefix=/
make -j$(nproc)
cd ../
popd

## Build testcase cxxfilt
sudo apt install texinfo bison flex
pushd afl-2.42b/test_progs/binutils-2.29
mkdir build
cd build
../configure --enable-shared=no --enable-static=yes
make -j$(nproc)
cd ../
popd

## Patch testcase cxxfilt
pushd afl-2.42b/pt_mode
./patch-bin.sh ../test_progs/binutils-2.29/build/binutils/cxxfilt
popd

echo "To fuzz testcase cxxfilt, run: "
echo "cd afl-2.42b/"
echo "./pt-fuzz-fast -P -i ./testcases/others/elf -o ./test_progs/binutils-2.29/build/binutils/cxxfilt_out -- ./test_progs/binutils-2.29/build/binutils/cxxfilt"
