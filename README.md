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
../configure
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
 
# Technical details
PTrix uses a series of optimization techniques to exploit intel PT for maximum fuzzing efficiencies.
Including: 
On the PT module side
1) Direct feedback translation
2) Parallel packet decoding

On the AFL side,
1) Switch feedback scheme from counted edge to slice to improve path sensitivity
2) Use bit-level granularity to record each slice instead of bytes to improve cache locality

Additionally, PTrix supports fork-server mode and mutiplexing PT buffer for different fuzzing instances. 
For more infomation, please check out the paper PTrix: Efficient Hardware-Assisted Fuzzing for COTS Binary (AsiaCCS'19)
Link: https://arxiv.org/abs/1905.10499 
