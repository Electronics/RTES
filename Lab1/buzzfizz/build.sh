#!/bin/bash

DEVICE_PATH=`ls /run/media/james | grep KL46` 
echo $DEVICE_PATH
make clean
make
if [[ -z "$DEVICE_PATH" ]]
then
    exit 1
fi
read -p "Would you like to flash? (Please replug device) [Y/n]" -n 1 -r
echo    # (optional) move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    exit 1
fi
FILENAME=`ls build | grep srec` 
echo "\n\n\n"
echo "Copying $FILENAME to $DEVICE_PATH"
sudo cp --no-preserve=mode,ownership build/$FILENAME /run/media/james/$DEVICE_PATH
echo "Copying to board"
