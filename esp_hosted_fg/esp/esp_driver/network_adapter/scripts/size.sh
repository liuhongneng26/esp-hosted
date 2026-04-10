#!/bin/bash

if [ -f image/size ]; then
    cp image/size image/size.old
fi

idf.py size size-components > image/size