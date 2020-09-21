#!/bin/bash

# This file compiles all the source files into .o files, then links them to form
# the test binary, 'bin-trees'.  The .o files all go into the 'work' directory.
# There are 'good' and 'bad' versions of inorder_norecurse and preorder_norecurse
# (e.g. inorder_norecurse.c.good and inorder_norecurse.c.bad).  This script
# assumes that the desired versions of those files have been copied into
# inorder_norecurse.c and preorder_norecurse.c.  The script files
# make_sources_good.sh and make_sources_bad.sh are meant to handle this.
#
#  This script is meant to be run directly in the full_bisect_test directory.
#  Most other scripts assume they are being run from the parent directory.

gcc -c build.c -o work/build.o
gcc -c preorder.c -o work/preorder.o
gcc -c inorder.c -o work/inorder.o
gcc -c main.c -o work/main.o
gcc -c stack.c -o work/stack.o 
gcc -c preorder_norecurse.c -o work/preorder_norecurse.o
gcc -c inorder_norecurse.c -o work/inorder_norecurse.o
gcc -o bin-trees work/main.o work/preorder.o work/inorder.o work/build.o work/preorder_norecurse.o work/inorder_norecurse.o work/stack.o

