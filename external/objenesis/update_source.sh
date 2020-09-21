#!/bin/bash
#
# Copyright 2013 The Android Open Source Project.
#
# Retrieves the current Objenesis source code into the current directory.
# Does not create a GIT commit.

# Force stop on first error.
set -e

if [ $# -ne 1 ]; then
    echo "$0 <version>" >&2
    exit 1;
fi

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Missing environment variables. Did you run build/envsetup.sh and lunch?" >&2
    exit 1
fi

VERSION=${1}

SOURCE="https://github.com/easymock/objenesis"
INCLUDE="
    LICENSE.txt
    main
    tck
    tck-android
    "

EXCLUDE="
    main/.settings/
    tck-android/.settings
    tck-android/lint.xml
    tck-android/project.properties
    "

working_dir="$(mktemp -d)"
#trap "echo \"Removing temporary directory\"; rm -rf $working_dir" EXIT

echo "Fetching Objenesis source into $working_dir"
git clone $SOURCE $working_dir/source
(cd $working_dir/source; git checkout $VERSION)

for include in ${INCLUDE}; do
  echo "Updating $include"
  rm -rf $include
  cp -R $working_dir/source/$include .
done;

for exclude in ${EXCLUDE}; do
  echo "Excluding $exclude"
  rm -r $exclude
done;

# Move the tck-android AndroidManifest.xml into the correct position.
mv tck-android/src/main/AndroidManifest.xml tck-android/AndroidManifest.xml

# Update the version and binary JAR URL.
perl -pi -e "s|^Version: .*$|Version: ${VERSION}|" "README.version"

# Remove any documentation about local modifications.
mv README.version README.tmp
grep -B 100 "Local Modifications" README.tmp > README.version
echo "        None" >> README.version
rm README.tmp

echo "Done"
