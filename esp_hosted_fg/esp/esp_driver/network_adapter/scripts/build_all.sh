#!/bin/bash

CMAKELISTS="CMakeLists.txt"

PROJECT=$(grep -oP 'project\(\K[^)]+' ${CMAKELISTS})

# cd ..

# idf.py fullclean
rm -rf build

idf.py build size || exit

mkdir -p image
./scripts/size.sh

cp  ./build/bootloader/bootloader.bin \
    ./build/${PROJECT}.bin \
    ./build/partition_table/partition-table.bin \
    image

ls -alh image/*
