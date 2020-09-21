#!/bin/bash

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Missing ANDROID_BUILD_TOP env variable. Run 'lunch' first."
    exit 1
fi

# Runs all unit tests under tools/acloud.
tests=$(find . -type f | grep test\.py)
for t in $tests; do
    PYTHONPATH=$ANDROID_BUILD_TOP/tools python $t;
done
