#!/bin/bash


pushd tester
mkdir -p build && pushd build
cmake ..
make
popd
popd

pushd src
mkdir -p build && pushd build
cmake ..
make
popd
popd