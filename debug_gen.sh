#!/bin/bash

./build.sh debug -DDEBUG_GENERATOR -DFUNC_TEST || exit
echo
echo -n '================================================'
echo
./comp
