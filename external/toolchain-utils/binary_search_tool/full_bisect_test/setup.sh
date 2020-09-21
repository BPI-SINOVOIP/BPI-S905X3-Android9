#!/bin/bash
#
# This script creates common.sh, which will be sourced by all the other
# scripts, to set up the necessary environment variables for the bisection
# to work properly.  It is called from main-bisect-test.sh.
#

DIR=`pwd`/"full_bisect_test"

GOOD_BUILD=${DIR}/good-objects
BAD_BUILD=${DIR}/bad-objects

mkdir -p ${DIR}/work

WORK_BUILD=${DIR}/work

rm -f ${WORK_BUILD}/*

COMMON_FILE="${DIR}/common.sh"

cat <<-EOF > ${COMMON_FILE}

BISECT_GOOD_BUILD=${GOOD_BUILD}
BISECT_BAD_BUILD=${BAD_BUILD}
BISECT_WORK_BUILD=${WORK_BUILD}

BISECT_GOOD_SET=${GOOD_BUILD}/_LIST
BISECT_BAD_BAD=${BAD_BUILD}/_LIST

BISECT_STAGE="TRIAGE"

EOF

chmod 755 ${COMMON_FILE}

exit 0
