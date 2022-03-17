#!/bin/bash

set -e

DEVICES=$(dfu-util -l | tee /dev/tty)

if [[ "$DEVICES" == *"name=\"@Internal RAM"* ]]; then
#Use the DFU download firmware provided by the 03-loader project

#remove :leave to not start the program after downloading
#replace -a 0 by -a 1 to store the program in the external flash. Can be combined with :leave.

dfu-util -S "Loader" -a 0 -s 0x01000:leave -D build/*-ram.tar

elif [[ "$DEVICES" == *"name=\"@Internal Flash"* ]]; then
#Use the DFU download program stored in the ROM by ST

dfu-util -a 0 -s 0x08000000:leave -D build/*-flash.bin

else

echo "Error, no compatible DFU device detected"

exit 1

fi

