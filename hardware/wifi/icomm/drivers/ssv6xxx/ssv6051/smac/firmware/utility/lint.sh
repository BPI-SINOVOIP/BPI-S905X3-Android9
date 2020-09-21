#!/bin/sh
PROJ=`cygpath $1`
cd $PROJ
if [ -n "$2" ]; then
  echo $2 > lint_file.lnt;
  make SHOW_LINT=1 LINT_FILE_LIST=lint_file lint;
else
  make SHOW_LINT=1 lint;
fi

