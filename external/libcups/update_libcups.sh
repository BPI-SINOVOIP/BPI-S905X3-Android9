#!/bin/bash

if [ -z "$1" ]; then
   echo "Please provide the source repo"
   exit -1
else
   SRC_REPO=$1
fi

TARGET_DIR=$(realpath $(dirname "$0"))
echo "== Updating $TARGET_DIR from $SRC_REPO =="

echo
echo "== get current rev =="
cd $TARGET_DIR

CURRENT_REV=$(cat libcups_version)
echo "Current rev is $CURRENT_REV"

echo
echo "== create tmp dir =="
TMP_DIR=$(mktemp -d)
echo "Created temporary dir $TMP_DIR"

echo
echo "== clone repo =="
cd $TMP_DIR

git clone $SRC_REPO .

echo
echo "== find new stable rev =="
NEW_REV=$(git tag -l | grep -v "release" | grep -v "b" | grep -v "rc" | sort | tail -n1)
echo "Stable rev is $NEW_REV"

if [ "$CURRENT_REV" == "$NEW_REV" ] ; then
    echo
    echo ">>> Rev $CURRENT_REV is already the newest stable rev"
else
    echo
    echo "== create diff in between rev $CURRENT_REV and rev $NEW_REV =="
    TMP_DIFF=$(mktemp)
    git diff $CURRENT_REV $NEW_REV -- cups/ filter/ LICENSE.txt > $TMP_DIFF
    echo "Diff in $TMP_DIFF"

    echo
    echo "== Apply diff =="
    cd $TARGET_DIR

    patch -p1 < $TMP_DIFF
    if [ $? -ne 0 ] ; then
       exit 1
    fi

    # update version numbers in config.h
    sed -i -e "s/^\(#.*CUPS_SVERSION\).*/\1 \"CUPS $NEW_REV\"/g" config.h
    sed -i -e "s:^\(#.*CUPS_MINIMAL\).*:\1 \"CUPS/${NEW_REV#v}\":g" config.h

    echo
    echo ">>> Updated license"

    cp LICENSE.txt NOTICE

    echo $NEW_REV > libcups_version
    git add -A
    git commit -m "Update libcups to $NEW_REV"

    echo
    echo ">>> Updated libcups from $CURRENT_REV to $NEW_REV"
fi

echo
echo "== Cleaning up =="
rm -f $TMP_DIFF
rm -rf $TMP_DIR
