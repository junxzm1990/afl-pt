# Installation

```
git clone https://github.com/junxzm1990/afl-pt
```

## Build PT Module
```
cd pt
make
./reinstall_ptmod.sh
```

## Build the customized AFL
```
cd afl-2.42b
make
```

## Build elfpatcher
```
cd afl-2.42b/pt_mode/elfpatcher
./bootstrap.sh
./configure
make
```

## Build pt_proxy 
```
cd afl-2.42b/pt_mode/pt_proxy
make
```

## Build customized ld
```
cd afl-2.42b/pt_mode/glibc-2.19
mkdir build
cd build
../configure
make -j64
```

## Build testcase cxxfilt
```
cd afl-2.42b/test_progs/binutils-2.29
mkdir build
cd build
../configure --enable-shared=no --enable-static=yes
make -j8
```

## Patch testcase cxxfilt
```
cd afl-2.42b/pt_mode
./patch-bin.sh ../test_progs/binutils-2.29/build/binutils/cxxfilt
```

## Fuzzing testcase cxxfilt 
```
cd afl-2.42b/
./pt-fuzz-fast -P -i ./testcases/others/elf -o ./test_progs/binutils-2.29/build/binutils/cxxfilt_out -- ./test_progs/binutils-2.29/build/binutils/cxxfilt
```

