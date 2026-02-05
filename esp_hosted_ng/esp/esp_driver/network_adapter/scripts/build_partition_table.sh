#!/bin/bash

$IDF_PATH/components/partition_table/gen_esp32part.py -q --offset 0x8000 --primary-bootloader-offset 0x0 --flash-size 16MB -- \
    partitions_4m.csv \
    ../image/partition-table.bin