#!/bin/bash

set -e

dfu-util -l

dfu-util -a 0 -s 0x08000000:leave -D build/blinky-stm32l452.bin