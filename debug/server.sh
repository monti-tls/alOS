#!/bin/bash

rm -f swo.bin
touch swo.bin
(tail -F swo.bin | ./sworead) &
openocd 2>/dev/null
