#!/bin/sh
PROJ=`cygpath "$1"`
cd $PROJ
make $2 $3 $4 $5 $6 $7 $8 $9
