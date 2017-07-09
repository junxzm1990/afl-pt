#!/bin/bash


if [ -z $1 ]; then
    echo "usage: "$(basename $0) "[target]"
    exit
fi

BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$BASE/elfpatcher/src/patchelf --set-interpreter $BASE/glibc-2.19/build/elf/ld.so $1
