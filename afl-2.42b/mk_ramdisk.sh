#!/bin/bash

mkdir /tmp/ramdisk
chmod 777 /tmp/ramdisk
sudo mount -t tmpfs -o size=2048M tmpfs /tmp/ramdisk
