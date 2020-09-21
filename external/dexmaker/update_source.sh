#!/bin/bash
#
# Copyright 2017 The Android Open Source Project.
#
# Retrieves the current Dexmaker to source code into the current directory, excluding portions related
# to mockito's internal build system and javadoc.

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

SOURCE="https://github.com/linkedin/dexmaker"
INCLUDE="
    LICENSE
    dexmaker
    dexmaker-mockito
    dexmaker-mockito-tests
    dexmaker-mockito-inline
    dexmaker-mockito-inline-dispatcher
    dexmaker-mockito-inline-tests/src
    dexmaker-tests/src
    "

EXCLUDE="
    "

working_dir="$(mktemp -d)"
trap "echo \"Removing temporary directory\"; rm -rf $working_dir" EXIT

echo "Fetching Dexmaker source into $working_dir"
git clone $SOURCE $working_dir/source
ORG_DIR=$(pwd)
cd $working_dir/source
git checkout $VERSION
SHA=$(git rev-parse $VERSION)
cd $ORG_DIR

for include in ${INCLUDE}; do
  echo "Updating $include"
  rm -rf $include
  mkdir -p $(dirname $include)
  cp -R $working_dir/source/$include $include
done;

for exclude in ${EXCLUDE}; do
  echo "Excluding $exclude"
  rm -r $exclude
done;

# Move the dexmaker-tests AndroidManifest.xml into the correct position.
mv dexmaker-tests/src/main/AndroidManifest.xml dexmaker-tests/AndroidManifest.xml
mv dexmaker-mockito-tests/src/main/AndroidManifest.xml dexmaker-mockito-tests/AndroidManifest.xml
mv dexmaker-mockito-inline-tests/src/main/AndroidManifest.xml dexmaker-mockito-inline-tests/AndroidManifest.xml

# Remove 3rd party code
rm -r dexmaker-mockito-inline/external

echo "Updating README.version"

# Update the version.
perl -pi -e "s|^Version: .*$|Version: ${VERSION} (${SHA})|" "README.version"

# Remove any documentation about local modifications.
mv README.version README.tmp
grep -B 100 "Local Modifications" README.tmp > README.version
echo "        None" >> README.version
rm README.tmp

echo "Done"
