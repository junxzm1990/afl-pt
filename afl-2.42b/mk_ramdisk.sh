#!/bin/bash

mkdir /tmp/afl-ramdisk
chmod 777 /tmp/afl-ramdisk
sudo mount -t tmpfs -o size=2048M tmpfs /tmp/afl-ramdisk
