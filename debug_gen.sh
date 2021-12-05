#!/bin/bash

./build.sh debug -DDEBUG_GENERATOR -DFUNC_TEST || exit
echo -n '-----------------------------------------------------------------------------------------'
./comp
