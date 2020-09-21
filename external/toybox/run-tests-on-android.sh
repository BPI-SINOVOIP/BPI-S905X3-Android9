#!/bin/bash

# Copy the toybox tests across.
adb shell mkdir /data/local/tmp/toybox-tests/
adb push tests/ /data/local/tmp/toybox-tests/
adb push scripts/runtest.sh /data/local/tmp/toybox-tests/

# Make a temporary directory on the device.
tmp_dir=`adb shell TMPDIR=/data/local/tmp mktemp --directory`

test_toy() {
  toy=$1

  echo

  location=$(adb shell "which $toy")
  if [ $? -ne 0 ]; then
    echo "-- $toy not present"
    return
  fi

  echo "-- $toy"

  implementation=$(adb shell "realpath $location")
  if [ "$implementation" != "/system/bin/toybox" ]; then
    echo "-- note: $toy is non-toybox implementation"
  fi

  adb shell -t "export FILES=/data/local/tmp/toybox-tests/tests/files/ ; \
                export VERBOSE=1 ; \
                export CMDNAME=$toy; \
                export C=$toy; \
                export LANG=en_US.UTF-8; \
                cd $tmp_dir ; \
                source /data/local/tmp/toybox-tests/runtest.sh ; \
                source /data/local/tmp/toybox-tests/tests/$toy.test"
}

if [ "$#" -eq 0 ]; then
  # Run all the tests.
  for t in tests/*.test; do
    toy=`echo $t | sed 's|tests/||' | sed 's|\.test||'`
    test_toy $toy
  done
else
  # Just run the tests for the given toys.
  for toy in "$@"; do
    test_toy $toy
  done
fi
