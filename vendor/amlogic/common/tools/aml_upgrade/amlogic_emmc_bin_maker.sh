#!/bin/sh
#
##V1.2 fix <aml_emmc_logic_table.xml> length can not support [a-fA-F]*
####### change $unpackDir to temp dir, and del it if built successful

emmc_partition_h=$1 #Header file of emmc_partition.h
partInfoXml=$2
AmlBurnPkg=$3
EmmcBin=$4
SIMG2IMG=$5

shellDir=$(cd `dirname $0`; pwd)

UNPACKIMG=${shellDir}/aml_image_v2_packer
unpackDir=`cd $(mktemp -d); pwd`
DTB_PC=${shellDir}/dtb_pc

if [ 4 -gt $# ]; then
    echo "Usage:[$0] emmc_partition_h partInfoXml AmlBurnPkg EmmcBin"
    exit 1
fi
if [ ! -f $partInfoXml ]; then
    echo "partInfoXml($partInfoXml) not existed!"
    exit 1
fi
if [ ! -f $AmlBurnPkg ]; then
    echo "AmlBurnPkg($AmlBurnPkg) not existed!"
    exit 1
fi

for tool in $(echo $SIMG2IMG $UNPACKIMG $DTB_PC); do
    if [ ! -f $tool ]; then
        echo "tool[$tool] not existed\n"
        exit 1
    fi
done

rm -rf $unpackDir/*
if [ ! -d $unpackDir ]; then
    mkdir $unpackDir
fi

echo "To unpack AmlBurnPkg($AmlBurnPkg)"
$UNPACKIMG -d $AmlBurnPkg $unpackDir
if [ $? -ne 0 ]; then 
    echo "fail in UNPACKIMG"
    exit $?
fi
rm -f $EmmcBin

SZ_1M=1024*1024
tmpPartsSz=""
szMacroDef="MMC_BOOT_DEVICE_SIZE MMC_RESERVED_SIZE MMC_ENV_SIZE MMC_KEY_SIZE MMC_BOOT_PARTITION_RESERVED PARTITION_RESERVED"
for definedSz in $(echo $szMacroDef); do
    tmp=`sed -n "/#define[ \t]\+${definedSz}/p" ${emmc_partition_h}`
    if [ -z "$tmp" ]; then
        printf "Val(%s) undefined, pls check emmc_partition_h(%s)\n" $definedSz $emmc_partition_h
        exit 1
    fi
    tmp=`echo $tmp | sed "s/SZ_1M/${SZ_1M}/" | awk -F[\(\)] '{print $2}'`
    printf "%s is %s," $definedSz $tmp
    tmp=$(($tmp))
    printf "(0x%x)\n" $tmp
    tmpPartsSz="${tmpPartsSz},${tmp}"
done
echo tmpPartsSz is [${tmpPartsSz}]
PART_SZ_bootloader=`echo $tmpPartsSz | awk -F, '{print $2}'`
PART_SZ_reserved=`echo $tmpPartsSz | awk -F, '{print $3}'`
PART_SZ_env=`echo $tmpPartsSz | awk -F, '{print $4}'`
PART_SZ_unifykey=`echo $tmpPartsSz | awk -F, '{print $5}'`
GAP_SZ_bootloader=`echo $tmpPartsSz | awk -F, '{print $6}'`
GAP_SZ_logic=`echo $tmpPartsSz | awk -F, '{print $7}'`

#Add one sector for bootloader
bootloader=$unpackDir/$(awk -v subtype="bootloader" -F[=\"] '$9 == subtype {print $3}' $unpackDir/image.cfg)
echo "bootloader is $bootloader"
if [ ! -f $bootloader ]; then
    echo "bootloader(${bootloader}) NOT existed"
    exit 1
fi
dd if=$bootloader of=${bootloader}.sdd seek=1 bs=512
mv ${bootloader}.sdd $unpackDir/bootloader.PARTITION

#dtb.img change to 512k
dtbPart=${unpackDir}/_aml_dtb.PARTITION

if [ ! -f ${dtbPart} ]; then
    dtbSrc=${unpackDir}/meson1.dtb
else
    mv ${dtbPart} ${dtbPart}.tmp 
    dtbSrc=${dtbPart}.tmp
fi
if [ ! -f ${dtbSrc} ]; then
    echo "dtbSrc ${dtbSrc} not existed"
    exit 1
fi
$DTB_PC ${dtbSrc} ${dtbPart}

#Move line of Partition cache to line1
sed -n "/cache/ s/^\(<Part PartitionName=\"\)cache\(.*\)/&\n\1env\2/p" $partInfoXml \
    | cat - $partInfoXml | \
    awk '{if (data[$0]++ == 0) lines[++count] = $0} END{for (i = 1; i <= count; i++) print lines[i]}' | \
    #update for each Logic partition \
    awk --non-decimal-data -F[=\"] 'BEGIN {\
        partOffset = 0; \
        print 0 "," "bootloader" "," "raw" "," '$PART_SZ_bootloader'/2  "," partOffset;\
        partOffset += '$PART_SZ_bootloader' + '$GAP_SZ_bootloader'; \
        print 0 "," "unifykey"   "," "raw" "," '$PART_SZ_unifykey'      "," partOffset; \
        part_sz_dtb = 512 * 1024; gapSzLogic = '$GAP_SZ_logic';\
        print 0 "," "_aml_dtb"   "," "raw" "," part_sz_dtb              "," partOffset + 4 * 1024 * 1024; \
        partOffset += '$PART_SZ_reserved' + gapSzLogic; \
    } \
    /^<Part /{thisPart = $3; fsType = $6; partCap = $9;\
              if(thisPart == "env") thisPartSz = '$PART_SZ_env'; else thisPartSz = partCap;\
              print NR "," thisPart "," fsType "," thisPartSz, "," partOffset; \
              partOffset += thisPartSz + gapSzLogic};' \
| while read line
do 
     LineNo=`echo $line|awk -F ',' '{print $1}'`
     partName=`echo $line|awk -F ',' '{print $2}'`
     FileSystem=`echo $line|awk -F ',' '{print $3}'`
     Length=`echo $line|awk -F ',' '{print $4}'`
     Start=`echo $line|awk -F ',' '{print $5}'`

     printf "\npartName=%-16s, Length=0x%09x, FileSystem=%-8s, Start=0x%09x\n" $partName $Length $FileSystem $Start
     echo [$line]
     burnFile=${partName}.PARTITION
     bsUnit=1M

     burnFile=$unpackDir/$burnFile
     if [ ! -f $burnFile ]; then 
         echo "Skip Not existed item ($burnFile)!"
         continue
     fi

     partOffsetInB=$Start
     partOffsetInMB=$((${partOffsetInB} / 1024 / 1024))
     partSzInB=`echo $Length | sed -n 's/^\(0x\)\?\([0-9a-fA-F]\+\)$/\1\2/p'`
     if [ -z $partSzInB ]; then 
         printf "Line(%d) fmt err at Length(%s)\nPls copy length from /partitions of dts\n" $LineNo $Length
         break 1;
     fi
     #on 64-bit platform, donot need worry about bit63 treated sign bit, as it already biggest enough
     #partSzInMB=$((${partSzInB} >> 20)) 
     partSzInMB=`awk --non-decimal-data -v bytes="${partSzInB}" 'BEGIN{printf "0x%x", bytes / 1024 / 1024}'`
     printf "Cap of part(%s) is 0x%x(0x%xM)\n"  $partName $partSzInB $partSzInMB
     partSzInB=$((${partSzInB}))

     if [ "$FileSystem" = "ext4" ]; then
         echo "[EXT4] to unsparse $burnFile"
         $SIMG2IMG $burnFile $burnFile.unsparse
         status=$?
         if [ $status -ne 0 ]; then 
             echo "fail in unsparse, err=$status"
             rm $EmmcBin
             break $status
         fi
         burnFile=$burnFile.unsparse
         if [ ! -f $burnFile ]; then
             echo "ext4 img ($burnFile) not existed"
             rm $EmmcBin
             break 1
         fi
     fi

     #targetFileSz=`stat -L -c %s $burnFile`
     targetFileSz=`du -b $burnFile | awk '{print $1}'`
     printf "targetFileSz is %s Bytes(0x%xM)\n" $targetFileSz $(($targetFileSz/1024/1024))
     if [ $targetFileSz -gt $partSzInB ]; then
         echo "targetFileSz($targetFileSz) > partSz($Length)"
         rm $EmmcBin
         break 1;
     fi

     echo "dd if=$burnFile of=$EmmcBin bs=$bsUnit seek=$partOffsetInMB"
     dd if=$burnFile of=$EmmcBin bs=$bsUnit seek=$partOffsetInMB 
     if [ $? -ne 0 ]; then 
         echo "fail in packing $burnFile to $EmmcBin"
         rm $EmmcBin
         break 1
     fi
done   

if [ ! -f $EmmcBin ]; then 
    echo "\nfail in packing $burnFile to $EmmcBin"
    exit 1
fi
targetFileSz=`du -b $EmmcBin | awk '{print $1}'`
printf "\nSuccessful to generate $%s, sz  0x%x(%dM)^^\n" $EmmcBin $targetFileSz $(($targetFileSz / 1024 /1024))
rm -rf $unpackDir
exit 0
