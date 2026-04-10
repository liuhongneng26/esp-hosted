#!/bin/bash

CMAKELISTS="CMakeLists.txt"

which idf.py >> /dev/null

if [ $? != 0 ]; then
    echo "not find idf! try get_idf!"
    get_idf

    which idf.py
    if [ $? != 0 ]; then
        echo ??
    fi
fi


export PROJECT=$(grep -oP 'project\(\K[^)]+' ${CMAKELISTS})