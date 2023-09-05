#!/bin/bash

set -e
set -x

LEAVE=":leave"
STORETOEXTFLASH=0

if [ $# -eq 0 ]; then
  echo "Usage: [--store] [--nostart] FILEPREFIX"
  echo "FILEPREFIX: Directory with filenameprefix where a file has the -flash.bin or -ram.tar ending is present"
  echo "--store: Store the file into the filesystem on the external flash"
  echo "--nostart: Do not start the transferred program"
  exit 2
fi

if [ $# -eq 1 ]; then
  PREFIX=$1
fi

if [ $# -eq 2 ]; then
echo "$1"
  if [ "$1" == "--store" ]; then
    STORETOEXTFLASH=1
  elif [ "$1" == "--nostart" ]; then
    LEAVE=""
  else
    echo "Error, parameter $1 is unknown"
    exit 2
  fi
  PREFIX=$2
fi

if [ $# -eq 3 ]; then
  if [ "$1" == "--store" ] && [ "$2" == "--nostart" ]; then
    STORETOEXTFLASH=1
    LEAVE=""
  elif [ "$1" == "--nostart" ] && [ "$2" == "--store" ]; then
    STORETOEXTFLASH=1
    LEAVE=""
  else
    echo "Error, parameter $1 or $2 is unknown"
    exit 2
  fi
  PREFIX=$3
fi

if [ $# -gt 3 ]; then
  echo "Error, too many parameters"
  exit 2
fi

DEVICES=$(dfu-util -l | tee /dev/tty)

if [[ "$DEVICES" == *"name=\"@Internal RAM"* ]]; then
#Use the DFU download firmware provided by the 03-loader project

#remove :leave to not start the program after downloading
#replace -a 0 by -a 1 to store the program in the external flash. Can be combined with :leave.

dfu-util -S "Loader" -d 0x1209:7702 -a $STORETOEXTFLASH -s 0x01000$LEAVE -D "$PREFIX-ram.tar"

elif [[ "$DEVICES" == *"name=\"@Internal Flash"* ]]; then
#Use the DFU download program stored in the ROM by ST

dfu-util -a 0 -s 0x08000000$LEAVE -D "$PREFIX-flash.bin"

else

echo "Error, no compatible DFU device detected"

exit 1

fi

