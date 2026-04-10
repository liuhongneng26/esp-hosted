#!/bin/bash

idf.py clean

idf.py build || exit

cp  ./build/bootloader/bootloader.bin \
    ./build/hzesp32c3.bin \
    ./build/partition_table/partition-table.bin \
    img

ls -alh img/*

./esp_flash.sh /dev/ttyACM0 all
