# PTrix

# set up
Environment:
Ubuntu 14.04


Build PT Module:
```
cd pt
make
./reinstall_ptmod.sh
```

Build afl:
```
cd afl-2.42b
make
```

Build elfpatcher:
```
cd afl-2.42b/pt_mode/elfpatcher
./bootstrap.sh
./configure
make
```

Build pt_proxy 
```
cd afl-2.42b/pt_mode/pt_proxy
make
```

Build customized ld
```
cd afl-2.42b/pt_mode/glibc-2.19
mkdir build
cd build
../configure --prefix=/
make -j64
```

Build testcase - binutils
```
cd afl-2.42b/test_progs/binutils-2.29
mkdir build
cd build
../configure
make -j64
```

Patch testcase - objdump
```
cd afl-2.42b/pt_mode
./patch-bin.sh ../test_progs/binutils-2.29/build/binutils/objdump
```

Fuzzing objdump
```
cd afl-2.42b/
./pt-fuzz-fast -P -i ./testcases/others/elf -o ./test_progs/binutils-2.29/build/binutils/out -- ./test_progs/binutils-2.29/build/binutils/objdump -D @@
```

# pt-mode resume fuzzing work:
 1) when fuzz with -P, a rand_map will be written to target_dir, name as ".target.rmap"
 2) when resuming fuzzing with -i-, specify env AFL_PTMODE_RAND_MAP=@rand_map_location
