#!/bin/bash

set -xe
docker build -t picosdk .
docker run --rm -it --name picosdk -v ${PWD}:/home/ubuntu picosdk make emu tester
# Flash using: picotool load -f src/build/flash_emulator.uf2
