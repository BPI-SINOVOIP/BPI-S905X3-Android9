#!/bin/bash

preIndex=0
count=0
CsvPath=""
definePath=""
version=""
rootPath="vendor/amlogic/common/frameworks/services/systemcontrol/PQ"

function Prepare() {
    CsvPath=`find ${rootPath} -name ssmHeader.csv`
    CsvPath=`echo ${CsvPath%/*}`

    definePath=`find ${rootPath} -name PQType.h`
    definePath=`echo ${definePath%/*}`

    version=`cat ${CsvPath}/ssmHeader.csv ${definePath}/PQType.h | cksum | awk '{print $1}'`

    if [ $CsvPath == "" ] || [ $definePath == "" ];then
        return 1
    fi

    for i in `cat ${CsvPath}/ssmHeader.csv`;
    do
        index=`echo $i | awk -F',' '{print $1}'`

        if [ $index -ge 0 ] 2>/dev/null;then
            continue
        elif [ $index == "Id" ];then
            continue
        else
            return 2
        fi
    done

    for i in `cat ${CsvPath}/ssmHeader.csv`;
    do
        size=`echo $i | awk -F',' '{print $3}'`

        if [ $size -ge 0 ] 2>/dev/null;then
            continue
        elif [ $size == "Size" ];then
            continue
        else
            if test ! `grep -w $size ${definePath}/PQType.h`;then
                index=1
                while ((1==1))
                do
                    split=`echo $size|cut -d "*" -f$index`
                    if [ "$split" != "" ];then
                        ((index++))
                        if test ! `grep -w $split ${definePath}/PQType.h`;then
                            return 3
                        fi
                    else
                        break
                    fi
                done

                if [ $index -lt 2 ];then
                    return 3
                fi
            fi
        fi
    done

    return 0
}

function Generate_Setting_Cfg() {
    cat ${CsvPath}/ssmHeader.csv | \
    awk -F ',' 'BEGIN {printf "#ifndef __PQ_SETTING_CFG__H__\n#define __PQ_SETTING_CFG__H__\n\nenum{\n"} 
    {
        if(NR>1) {
            print "    "$2" = "$1","
        }
    }
    END{print "};\n#endif"}'
}

function AutoGenSrcFiles() {

    printf "#include <stdio.h>\n#include \"SSMHeader.h\"\n"
    printf "#include \"PQSettingCfg.h\"\n\n"

    echo "struct SSMHeader_section2_t gSSMHeader_section2[] = {"

    for i in `cat ${CsvPath}/ssmHeader.csv`;
    do
        index=`echo $i | awk -F',' '{print $1}'`
        size=`echo $i | awk -F',' '{print $3}'`
        name=`echo $i | awk -F',' '{print $2}'`

        #echo preindex:${preIndex}
        #echo index:${index}" "name:${name}" "count:${count}

        if [ $index == "Id" ];then
            continue
        fi

        count=$[$count+1]

        while (($index - $preIndex > 1))
        do
            count=$[$count+1]
            preIndex=$[$preIndex+1]
            #echo $preIndex
            printf "    {.addr = 0, .size = 0, .valid = 0, .rsv = {0}},\n"
        done

        printf "    {.addr = 0, .size = %s, .valid = 0, .rsv = {0}},\n" ${size}
        preIndex=${index}
    done

    echo "};"

    printf "\nstruct SSMHeader_section1_t gSSMHeader_section1 = \n{
    .magic = 0x8f8f8f8f, .version = %s, .count = %d, .rsv = {0}\n};\n" $version $count
}

Prepare
ret=$?
echo ret:$ret

if [ $ret -ne 0 ];then
    exit $ret
fi

AutoGenSrcFiles > ${CsvPath}/SSMHeader.cpp
Generate_Setting_Cfg > ${CsvPath}/include/PQSettingCfg.h

exit 0
