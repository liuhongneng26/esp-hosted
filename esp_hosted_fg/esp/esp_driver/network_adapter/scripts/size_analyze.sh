#!/bin/bash
# Analyze IRAM/DRAM usage from ESP-IDF build

set -e
source ~/esp32/esp-idf-v5.5.2/export.sh > /dev/null 2>&1

echo "=========================================="
echo "   ESP-IDF Memory Analysis (IRAM/DRAM)"
echo "=========================================="
echo ""

# Get CSV output
DATA=$(python3 -m esp_idf_size build/demo.map --archives --format csv 2>/dev/null)

# Summary
echo ">>> Memory Summary <<<"
IRAM=$(echo "$DATA" | grep "Used stat D/IRAM" | grep -oE '[0-9]+ bytes' | head -1 | grep -oE '[0-9]+')
IRAM_KB=$((IRAM / 1024))
IRAM_TOTAL=320
IRAM_PCT=$((IRAM * 100 / (IRAM_TOTAL * 1024)))
echo "IRAM:    ${IRAM} bytes (${IRAM_KB} KB) - ${IRAM_PCT}% of ${IRAM_TOTAL}KB"

FLASH=$(echo "$DATA" | grep "Used Flash size" | grep -oE '[0-9]+ bytes' | head -1 | grep -oE '[0-9]+')
FLASH_KB=$((FLASH / 1024))
FLASH_MB=$((FLASH / 1024 / 1024))
FLASH_R=$((FLASH % (1024 * 1024)))
echo "Flash:   ${FLASH} bytes (${FLASH_KB} KB = ${FLASH_MB}.$((FLASH_R/1024)) MB)"

echo ""

# IRAM Top 15
echo ">>> IRAM (.text) Top 15 (bytes | KB) <<<"
echo "$DATA" | awk -F',' 'NR>10 && $1 ~ /lib/ {
    gsub(/ /,"",$4)
    if($4+0 > 0) {
        iram=$4; name=$1
        if (iram > 999) printf "%6d  %5d  %s\n", iram, iram/1024, name
    }
}' | sort -rn | head -15
echo ""

# DRAM Top 15
echo ">>> DRAM (data+bss) Top 15 (bytes | KB) <<<"
echo "$DATA" | awk -F',' 'NR>10 && $1 ~ /lib/ {
    gsub(/ /,"",$2); gsub(/ /,"",$3)
    dram=$2+$3
    if(dram > 0) printf "%6d  %5d  %s\n", dram, dram/1024, $1
}' | sort -rn | head -15
echo ""

# Flash Code Top 10
echo ">>> Flash Code Top 10 (bytes | KB) <<<"
echo "$DATA" | awk -F',' 'NR>10 && $1 ~ /lib/ {
    gsub(/ /,"",$6)
    if($6+0 > 10000) printf "%7d  %6d  %s\n", $6, $6/1024, $1
}' | sort -rn | head -10
echo ""

echo "=========================================="
TOTAL=$(echo "$DATA" | grep "Total image" | awk -F',' '{print $2}' | sed 's/ bytes.*//')
TOTAL_KB=$((TOTAL / 1024))
TOTAL_MB=$((TOTAL / 1024 / 1024))
TOTAL_R=$((TOTAL % (1024 * 1024)))
echo "Total: ${TOTAL} bytes (${TOTAL_KB} KB = ${TOTAL_MB}.$((TOTAL_R/1024)) MB)"
