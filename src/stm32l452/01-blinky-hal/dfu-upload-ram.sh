#!/bin/bash

set -e

dfu-util -l

dfu-util -S "Loader" -a 0 -s 0x20000000:leave -D build/blinky-stm32l452-ram.bin
