#!/bin/bash
CMAKELISTS="CMakeLists.txt"

PROJECT=$(grep -oP 'project\(\K[^)]+' ${CMAKELISTS})

FLASH_PART_BL="0x0      image/bootloader.bin "
FLASH_PART_TB="0x8000   image/partition-table.bin "
FLASH_PART_NVS="0x9000  image/nvs.bin "
FLASH_PART_OTA0="0x10000 image/${PROJECT}.bin "

# cd ..

if [ -n "$2" ]; then
    TARGET=$2
else
    TARGET="/dev/ttyACM0"
fi

SDKCONFIG="sdkconfig"

CONFIG_IDF_TARGET=$(grep -E '^CONFIG_IDF_TARGET=' "$SDKCONFIG" | sed -E 's/^CONFIG_IDF_TARGET="([^"]+)".*/\1/')
CONFIG_ESPTOOLPY_FLASHSIZE=$(grep -E '^CONFIG_ESPTOOLPY_FLASHSIZE=' "$SDKCONFIG" | sed -E 's/^CONFIG_ESPTOOLPY_FLASHSIZE="([^"]+)".*/\1/')
CONFIG_ESPTOOLPY_FLASHFREQ=$(grep -E '^CONFIG_ESPTOOLPY_FLASHFREQ=' "$SDKCONFIG" | sed -E 's/^CONFIG_ESPTOOLPY_FLASHFREQ="([^"]+)".*/\1/')
CONFIG_ESPTOOLPY_FLASHMODE=$(grep -E '^CONFIG_ESPTOOLPY_FLASHMODE=' "$SDKCONFIG" | sed -E 's/^CONFIG_ESPTOOLPY_FLASHMODE="([^"]+)".*/\1/')

echo "$CONFIG_IDF_TARGET"
echo "$CONFIG_ESPTOOLPY_FLASHSIZE" "$CONFIG_ESPTOOLPY_FLASHFREQ" $CONFIG_ESPTOOLPY_FLASHMODE

esptool.py --chip ${CONFIG_IDF_TARGET} -p ${TARGET} flash_id || exit

# exit

if [ "$1" = "all" ]; then
    idf.py -p ${TARGET} erase-flash
    FLASH_PART=${FLASH_PART_BL}${FLASH_PART_TB}${FLASH_PART_OTA0}
else
    FLASH_PART=${FLASH_PART_OTA0}
fi

echo $FLASH_PART $TARGET

cp  ./build/bootloader/bootloader.bin \
    ./build/${ROJEC}.bin \
    ./build/partition_table/partition-table.bin \
    image

ls -alh image/*

esptool.py --chip ${CONFIG_IDF_TARGET} \
    -p ${TARGET} -b 3000000 \
    --before=default_reset --after=hard_reset write_flash \
    --flash_size ${CONFIG_ESPTOOLPY_FLASHSIZE} \
    --flash_freq ${CONFIG_ESPTOOLPY_FLASHFREQ} \
    --flash_mode ${CONFIG_ESPTOOLPY_FLASHMODE} \
    ${FLASH_PART} || exit

picocom ${TARGET}