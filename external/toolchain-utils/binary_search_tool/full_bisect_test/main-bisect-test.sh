#!/bin/bash
#
#  This script is the heart of the bisection test.  It assumes the good-objects
#  and bad-objects directories have been created and populated.  It runs three
#  bisection tests:
#   Test 1.  use --file_args, and no pruning, which passes the object file list
#            in a file, and stops as soon as it finds the first bad file.
#   Test 2.  do not use --file_args, and no pruning.  The object files are passed
#            directly on the command line; stop as soon as it finds the first
#            bad file.
#   Test 3.  use --file_args and --prune.  Pass the object file list in a file
#            and run until it finds ALL the bad files (there are two of them).
#

SAVE_DIR=`pwd`

DIR=full_bisect_test

# Make sure you are running this script from the parent directory.
if [[ ! -f "${DIR}/setup.sh" ]] ; then
  echo "Cannot find ${DIR}/setup.sh.  You are running this from the wrong directory."
  echo "You need to run this from toolchain-utils/binary_search_tool ."
  exit 1
fi

# Run Test 1.
${DIR}/setup.sh

./binary_search_state.py --get_initial_items="${DIR}/get_initial_items.sh" \
  --switch_to_good="${DIR}/switch_to_good.sh" \
  --switch_to_bad="${DIR}/switch_to_bad.sh" \
  --test_setup_script="${DIR}/test_setup.sh" \
  --test_script="${DIR}/interactive_test.sh" \
  --file_args &> /tmp/full_bisect_test.log

${DIR}/cleanup.sh

grep "Search complete. First bad version: " /tmp/full_bisect_test.log &> /dev/null
test_status=$?

if [[ ${test_status} -ne 0 ]] ; then
  echo "Test 1 FAILED. See /tmp/full_bisect_test.log for details."
  exit 1
else
  echo "Test 1 passed."
fi

cd ${SAVE_DIR}

# Run Test 2.
${DIR}/setup.sh

./binary_search_state.py --get_initial_items="${DIR}/get_initial_items.sh" \
  --switch_to_good="${DIR}/switch_to_good.sh" \
  --switch_to_bad="${DIR}/switch_to_bad.sh" \
  --test_setup_script="${DIR}/test_setup.sh" \
  --test_script="${DIR}/interactive_test.sh" \
  &> /tmp/full_bisect_test.log

${DIR}/cleanup.sh

grep "Search complete. First bad version: " /tmp/full_bisect_test.log &> /dev/null
test_status=$?

if [[ ${test_status} -ne 0 ]] ; then
  echo "Test 2 FAILED. See /tmp/full_bisect_test.log for details."
  exit 1
else
  echo "Test 2 passed."
fi

cd ${SAVE_DIR}

# Run Test 3.
${DIR}/setup.sh

./binary_search_state.py --get_initial_items="${DIR}/get_initial_items.sh" \
  --switch_to_good="${DIR}/switch_to_good.sh" \
  --switch_to_bad="${DIR}/switch_to_bad.sh" \
  --test_setup_script="${DIR}/test_setup.sh" \
  --test_script="${DIR}/interactive_test.sh" \
  --file_args --prune &> /tmp/full_bisect_test.log

${DIR}/cleanup.sh

grep "Bad items are: " /tmp/full_bisect_test.log | grep inorder_norecurse.o &> /dev/null
test_status_1=$?

grep "Bad items are: " /tmp/full_bisect_test.log | grep preorder_norecurse.o &> /dev/null
test_status_2=$?

if [[ ${test_status_1} -ne 0 ]] ; then
  echo "Test 3 FAILED. See /tmp/full_bisect_test.log for details."
  exit 1
elif [[ ${test_status_2} -ne 0 ]] ; then
  echo "Test 3 FAILED. See /tmp/full_bisect_test.log for details."
  exit 1
else
  echo "Test 3 passed."
fi

# All tests passed!
exit 0

