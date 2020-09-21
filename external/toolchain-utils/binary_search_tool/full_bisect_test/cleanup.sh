#!/bin/bash

# In keeping with the normal way of doing bisectin, this script is meant to
# remove files specific to the particular run of the bisector.
#
# This file is called from main-bisect-test.sh

rm full_bisect_test/common.sh
