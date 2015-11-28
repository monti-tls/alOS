#!/bin/bash -f

make -C ..
arm-unknown-eabi-gdb -x gdb.cfg ../bin/alOS.elf -tui
# kill $(ps | grep openocd$ | awk '{print $1}')

