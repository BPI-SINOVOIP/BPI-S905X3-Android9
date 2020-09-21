#!/bin/bash
#
# This script is one of the two main driver scripts for testing the bisector.
# It should be used to test the bisection tool, if you do NOT want to test
# the compiler wrapper (e.g. don't bother with POPULATE_GOOD & POPULATE_BAD
# stages).
#
# It makes sure the good & bad object directories exist (soft links); checks
# to see if it needs to compile the good & bad sources & populate the
# directories; does so if needed.
#
# Then it calls main-bisect-test, which runs the actual bisection tests.  This
# script assumes it is being run from the parent directory.
#
# NOTE: Your PYTHONPATH environment variable needs to include both the
# toolchain-utils directory and the
# toolchain-utils/binary_search_tool directory for these testers to work.
#

SAVE_DIR=`pwd`

DIR=full_bisect_test

if [[ ! -d "${DIR}" ]] ; then
  echo "Cannot find ${DIR}; you are running this script from the wrong place."
  echo "You need to run this from toolchain-utils/binary_search_tool ."
  exit 1
fi

# Set up object file soft links
cd ${DIR}

rm -f good-objects
rm -f bad-objects

ln -s good-objects-permanent good-objects
ln -s bad-objects-permanent bad-objects

if [[ ! -d work ]] ; then
  mkdir work
fi

# Check to see if the object files need to be built.
if [[ ! -f good-objects-permanent/build.o ]] ; then
  # 'make clean'
  rm -f work/*.o
  # skip populate stages in bisect wrapper
  unset BISECT_STAGE
  # Set up the 'good' source files.
  cd ..
  ${DIR}/make_sources_good.sh
  cd ${DIR}
  # Build the 'good' .o files & copy to appropriate directory.
  ./build.sh
  mv work/*.o good-objects-permanent/.
  # Set up the 'bad' source files.
  cd ..
  ${DIR}/make_sources_bad.sh
  cd ${DIR}
  # Build the 'bad' .o files & copy to appropriate directory.
  ./build.sh
  mv work/*.o bad-objects-permanent/.
fi

# Now we're ready for the main test.

cd ${SAVE_DIR}
${DIR}/main-bisect-test.sh
