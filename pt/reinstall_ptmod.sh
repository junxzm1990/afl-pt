#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

sudo rmmod ptmodule
mod_param=$(sudo cat /proc/kallsyms  | grep  '\bkallsyms_lookup_name\b'  | cut -d ' ' -f1)
read -p "module param is $mod_param, continue to install module?[y/n]" yn
case $yn in [Yy]* )
            sudo insmod ptmodule.ko kallsyms_lookup_name_ptr=0x$mod_param
            if [ $? -eq 0 ]; then
                echo -e "${GREEN}PT Module installed successfully${NC}"
            else
                echo -e "${RED}Failed to install PT module. Please check dmesg output.${NC}"
            fi;;
    [Nn]* ) exit;;
esac

