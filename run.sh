#!/bin/bash

docker build -t picosdk .
docker run --rm -it --name picosdk -v ${PWD}:/home/ubuntu picosdk make emu tester