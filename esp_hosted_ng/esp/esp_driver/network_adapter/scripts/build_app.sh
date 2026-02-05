#!/bin/bash

CMAKELISTS="CMakeLists.txt"

PROJECT=$(grep -oP 'project\(\K[^)]+' ${CMAKELISTS})

# cd ..

idf.py app || exit -1

mkdir -p image
cp  ./build/bootloader/bootloader.bin \
    ./build/${PROJECT}.bin \
    ./build/partition_table/partition-table.bin \
    image

ls -alh image/*
