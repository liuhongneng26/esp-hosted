#!/bin/bash

if [ -f "nvs.csv" ]; then
    echo 
else
    cp ./scripts/nvs.csv .
fi

$IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate \
    nvs.csv ./image/nvs.bin 0x4000
