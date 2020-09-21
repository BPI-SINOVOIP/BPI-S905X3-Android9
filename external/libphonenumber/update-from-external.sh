#!/bin/bash
#
# Copyright 2016 The Android Open Source Project.
#
# Retrieves the specified version of libphonenumber into the
# external/libphonenumber directory
#
# Does not create a GIT commit.

if [ $# -ne 1 ]; then
    echo "usage: $0 <version>"
    echo "  where <version> is the version number, e.g. 7.7.3"
    exit 1
fi

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Missing environment variables. Did you run build/envsetup.sh and lunch?" 1>&2
    exit 1
fi
    
VERSION=$1
TAG=v$VERSION
SOURCE="https://github.com/googlei18n/libphonenumber/"
DIR=$ANDROID_BUILD_TOP/external/libphonenumber

tmp=$(mktemp -d)
trap "echo Removing temporary directory; rm -fr $tmp" EXIT

echo "Fetching source into $tmp"
(cd $tmp; git clone -q -b $TAG --depth 1 $SOURCE source)

for i in $(ls $tmp/source/java)
do
    echo "Updating $i"
    rm -fr $DIR/$i
    cp -r $tmp/source/java/$i $DIR/$i
    (cd $DIR; git add $i)
done

for i in README.version README.android
do
    echo "Updating $i"
    cp $DIR/$i $tmp
    sed "s|Version: .*$|Version: $VERSION|" < $tmp/$i > $DIR/$i
    (cd $DIR; git add $i)
done
