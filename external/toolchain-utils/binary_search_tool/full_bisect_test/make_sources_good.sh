#!/bin/bash -u
#
#  There are two versions (good & bad) of inorder_norecurse.c and
#  preorder_norecurse.c.  This script makes sure the good versions
#  are copied into the .c files that will be built and copied into
#  the good-objects directory, for the bisection test.  It is called
#  from run-test-nowrapper.sh.
#

pushd full_bisect_test

cp inorder_norecurse.c.good inorder_norecurse.c
cp preorder_norecurse.c.good preorder_norecurse.c

popd
