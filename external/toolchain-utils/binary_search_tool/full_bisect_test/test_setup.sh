#!/bin/bash
#
# This is one of the scripts that gets passed to binary_search_state.py.
# It's supposed to generate the binary to be tested, from the mix of
# good & bad object files.
#
source full_bisect_test/common.sh

WD=`pwd`

cd full_bisect_test

echo "BUILDING IMAGE"

gcc -o bin-trees work/*.o

