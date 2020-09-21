#!/bin/bash -u
#
#  There are two versions (good & bad) of inorder_norecurse.c and
#  preorder_norecurse.c.  This script makes sure the bad versions
#  are copied into the .c files that will be built and copied into
#  the bad-objects directory, for the bisection test. It is called
#  from run-test-nowrapper.sh.
#

pushd full_bisect_test

cp inorder_norecurse.c.bad inorder_norecurse.c
cp preorder_norecurse.c.bad preorder_norecurse.c

popd
