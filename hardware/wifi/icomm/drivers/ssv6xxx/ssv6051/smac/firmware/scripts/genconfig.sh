#!/bin/sh

if [ $# -ne 1 ] ; then
    echo "Usage: genconfig.sh file1 file2"
    echo ""
    exit 1
fi

echo "#ifndef _CONFIG_H_"
echo "#define _CONFIG_H_"
echo
echo "#include <types.h>"
echo "#include <rtos.h>"
echo
echo
echo
echo
echo
echo

cat $1 | grep -e '^CONFIG_' | sed 's/#/\/\//g' | sed 's/^CONFIG_/#define /g' |
sed 's/\t/    /g' | sed 's/=/  /g'


#sed 's/ /*/g' | sed 's/\t/*/g'




#awk '($3 == "=") { printf "#define %-50s%s\n", $2, $4  }'
#awk '($3 == "=") { defstr=sprintf("%-50s//\n", $2); sub(/$$2/, defstr, $_); \
#print "hhh"$_"kkk"; }'
#sed 's/[[:blank:]]*=[[:blank:]]*/ /g' | \
#awk ' { print NF }'
#awk '($2 == "=") { printf "#define %-50s%s\n", $1, $3  }'


echo
echo
echo
echo "#endif /* _CONFIG_H_ */"
echo



