#!/bin/bash
#This shell script for parse the dtsi file to get partition name and partition size
#To generate the partition table xml for emmc burnimg image create
#Author : zhigang.yu@amlogic.com

if [ $# != 2 ];then
  echo "eg: dtsi2xml.sh partition_mbox_normal_P_32.dtsi p212.xml"
  exit
fi

echo  "Generate Partition Table Xml From " $1 "to" $2

filename=$1
out_xml_name=$2

i=0
index=10
tmp3=0
node=10
base="part-"

#get cache partition size
for line in `cat ${filename}`
do
  if [ $line = "\"cache\";" ];then
    #find cache partition name
    index=0;
  fi

  let index=index+1

  #get partition size
  if [ $index = 5 ];then
    tmp=${line%?}
    #add cache partition
    echo -n "    <partition name=\"cache\" size=\"" >> $out_xml_name
    echo -n ${tmp%?} >> $out_xml_name
    echo "\" path=\"\" />" >> $out_xml_name
    break;
  fi
done

#add env partition
echo "    <partition name=\"env\" size=\"0x800000\" path=\"\" />" >> $out_xml_name

index=10

#get all partition name and size
for line in `cat ${filename}`
do
  #set hh value equal "part-0/1/2/3/..."
  hh=$base$i

  if [ $line = $hh ];then
    #find a part-i name
    index=0;
    let i=i+1
  fi

  let index=index+1

  #get partition name
  if [ $index = 3 ];then
    tmp1=${line:2}
    tmp2=${tmp1%?}
    tmp3=${tmp2%?}
  fi

  #cache partition already add, continue
  if [ $tmp3 = "cache" ];then
    index=10
    tmp3=0
    continue
  fi

  #fine partition size by partition name(tmp3)
  if [ $tmp3 != 0 ];then
    for s_line in `cat ${filename}`
    do
      #find pname
      if [ $s_line = "\"$tmp3\";" ];then
        node=0
      fi

      let node=node+1
      #find partition size
      if [ $node = 5 ];then
        tmp4=${s_line%?}
        echo -e "$hh  \t$tmp3     \t${tmp4%?}"
        echo -n "    <partition name=\"" >> $out_xml_name
        echo -n $tmp3 >> $out_xml_name
        echo -n "\" size=\"" >>  $out_xml_name
        echo -n ${tmp4%?} >>  $out_xml_name
        echo "\" path=\"\" />" >> $out_xml_name
      fi
    done
    tmp3=0
  fi
done
echo "</partitions>" >> $out_xml_name
echo  "Generate Partition Table Xml Sucess"