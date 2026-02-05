#!/bin/bash

CMAKELISTS="CMakeLists.txt"

PROJECT=$(grep -oP 'project\(\K[^)]+' ${CMAKELISTS})

# cd ..

idf.py fullclean

idf.py build|| exit

cp  ./build/bootloader/bootloader.bin \
    ./build/${PROJECT}.bin \
    ./build/partition_table/partition-table.bin \
    image

ls -alh image/*
