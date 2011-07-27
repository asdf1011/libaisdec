#!/bin/bash

set -e
set -u

cd `dirname $0`

NUM_TESTS=0
for name in data/*.aivdm; do
    NUM_TESTS=$((NUM_TESTS + 1))
    echo -n .
    if ! diff -u $name.xml <( ../decode.py $name ); then
        echo
        echo "Test failed, differences found in $name!"
        exit 1
    fi
done
echo
echo All $NUM_TESTS tests passed.
