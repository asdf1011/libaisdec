#!/bin/bash

set -e
set -u

cd `dirname $0`

for name in data/*.aivdm; do
    if ! diff -u $name.xml <( ../decode.py $name ); then
        echo "Tests failed, differences found in $name!"
        exit 1
    fi
done
echo All tests passed.
