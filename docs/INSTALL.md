# Installation

## Environment

Ubuntu 14.04.5

## Download Source Code

```
git clone https://github.com/junxzm1990/afl-pt
cd afl-pt
```

We developed [a installation script](../tools/install-beta.sh) to automatically deploy Ptrix.

## Build PT Module

```
cd pt
make
./reinstall_ptmod.sh
# require sudo privilege to insert PT module
cd -
```

## Build the customized AFL

```
cd afl-2.42b
make
cd -
```

## Build elfpatcher

```
cd afl-2.42b/pt_mode/elfpatcher
./bootstrap.sh
./configure
make
cd -
```

## Build pt_proxy 

```
cd afl-2.42b/pt_mode/pt_proxy
make
cd -
```

## Build customized ld
```
cd afl-2.42b/pt_mode/glibc-2.19
mkdir build
cd build
../configure
make -j64
cd ../
cd ../../../
```

## Build testcase cxxfilt
```
cd afl-2.42b/test_progs/binutils-2.29
mkdir build
cd build
../configure --enable-shared=no --enable-static=yes
make -j8
cd ../
cd ../../../
```

## Patch testcase cxxfilt
```
cd afl-2.42b/pt_mode
./patch-bin.sh ../test_progs/binutils-2.29/build/binutils/cxxfilt
cd ../../
```

## Fuzzing testcase cxxfilt 
```
cd afl-2.42b/
./pt-fuzz-fast -P -i ./testcases/others/elf -o ./test_progs/binutils-2.29/build/binutils/cxxfilt_out -- ./test_progs/binutils-2.29/build/binutils/cxxfilt
```
