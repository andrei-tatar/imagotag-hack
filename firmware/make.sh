#!/bin/bash

SDCC_PATH="/d/Programs/SDCC/bin/sdcc.exe"

find ./src -name "*.c" -type f | xargs -L 1 $SDCC_PATH -mmcs51 -o ./build/ -c
find ./build -name "*.rel" -type f ! -name main.rel | xargs $SDCC_PATH -o ./build/ ./build/main.rel