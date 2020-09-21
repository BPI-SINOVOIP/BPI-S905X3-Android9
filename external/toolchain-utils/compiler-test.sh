#!/bin/bash

#  This script is supposed to verify which compiler (GCC or clang) was
#  used to build the packages in a ChromeOS image.  To use the script,
#  just pass the path to the tree containing the *.debug files for a
#  ChromeOS image.  It then reads the debug info in each .debug file
#  it can find, checking the AT_producer field to determine whether
#  the package was built with clang or GCC.  It counts the total
#  number of .debug files found as well as how many are built with
#  each compiler.  It writes out these statistics when it is done.
#
#  For a locally-built ChromeOS image, the debug directory is usually:
#  ${chromeos_root}/chroot/build/${board}/usr/lib/debug (from outside
#   chroot)
#  or
#  /build/${board}/usr/lib/debug (from inside chroot)
#
#  For a buildbot-built image you can usually download the debug tree
#  (using 'gsutil cp') from
#  gs://chromeos-archive-image/<path-to-your-artifact-tree>/debug.tgz
#  After downloading it somewhere, you will need to uncompress and
#  untar it before running this script.
#

DEBUG_TREE=$1

clang_count=0
gcc_count=0
file_count=0

# Verify the argument.

if [[ $# != 1 ]]; then
    echo "Usage error:"
    echo "compiler-test.sh <debug_directory>"
    exit 1
fi


if [[ ! -d ${DEBUG_TREE} ]] ; then
    echo "Cannot find ${DEBUG_TREE}."
    exit 1
fi

cd ${DEBUG_TREE}
for f in `find . -name "*.debug" -type f` ; do
    at_producer=`readelf --debug-dump=info $f | head -25 | grep AT_producer `;
    if echo ${at_producer} | grep -q 'GNU C' ; then
        ((gcc_count++))
    elif echo ${at_producer} | grep -q 'clang'; then
        ((clang_count++));
    fi;
    ((file_count++))
done

echo "GCC count: ${gcc_count}"
echo "Clang count: ${clang_count}"
echo "Total file count: ${file_count}"


exit 0
