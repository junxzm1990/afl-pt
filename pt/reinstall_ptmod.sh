#!/bin/sh
sudo rmmod ptmodule
mod_param=$(sudo cat /proc/kallsyms  | grep  '\bkallsyms_lookup_name\b'  | cut -d ' ' -f1) 
read -p "module param is $mod_param, continue to install module?[y/n]" yn
case $yn in
    [Yy]* ) sudo insmod ptmodule.ko kallsyms_lookup_name_ptr=0x$mod_param;;
    [Nn]* ) exit;;
esac


