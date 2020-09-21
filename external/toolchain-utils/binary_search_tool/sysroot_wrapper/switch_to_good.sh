#!/bin/bash -u

source common/common.sh

# Remove file, signaling to emerge that it needs to be rebuilt. The compiler
# wrapper will insert the correct object file based on $BISECT_GOOD_SET
cat $1 | sudo xargs rm -f

exit 0
