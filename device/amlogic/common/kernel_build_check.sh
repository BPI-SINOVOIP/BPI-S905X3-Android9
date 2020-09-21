#!/bin/bash
#
#       By Victor Wan <victor.wan@amlogic.com>
#       (c) 2014.12 @ Shanghai
#

VERSION="1.0 (2014.12.4)"

SCRIPT_PATH=$PWD/$(dirname "$0")

#assume default script located in device/amlogic/common
ANDROID_PATH=${SCRIPT_PATH}/../../..
KERNEL_PATH=${ANDROID_PATH}/common
OUTPUT_PATH=/var/tmp/${USER}/build_tmp/
REPORT_NAME=.report
REPORT_FILE=
MESON_CHIPS="6 6tv 8 8_tee 8b 8b_tee g9tv"
NR_JOBS_CFG=1
NR_JOBS=
USER_JOBS=1
VERBOSE=no
HELP=yes
NULL_DEV="> /dev/null"
COLOR_Y_S="\033[33;40;1m"
COLOR_Y_E="\033[0m"
COLOR_R_S="\033[31;40;1m"
COLOR_R_E="\033[0m"

function usage()
{
	echo "Usage: $0 -c [-j jobs] [-k kernel_path] [-o output_path] [-v] [-h]"
	echo " -c enable check"
	echo " -j number of build jobs"
	echo " -k kernel path: default kernel path: ${KERNEL_PATH}"
	echo " -o output path: default output path: ${OUTPUT_PATH}"
	echo " -v show verbose build info"
	echo " -h : show help info"
	echo "  script version: ${VERSION}"
	echo "  chip list: ${MESON_CHIPS}"
	exit 1
}

function args_check()
{

	if [ "${HELP}" = "yes" ]; then
		usage
	fi

	if [ -n "${USER_JOBS}" ]; then
		NR_JOBS=${USER_JOBS}
	#	echo "override NR_JOBS=${USER_JOBS}!"
	else
		NR_CPU=`cat /proc/cpuinfo | grep "processor"|wc -l`
		let "NR_JOBS=${NR_CPU}*${NR_JOBS_CFG}"
		NR_JOBS=${NR_CPU}
	fi
	if [ ! -d ${KERNEL_PATH} ];then
		echo "Can't open ${KERNEL_PATH} !"
		exit 1
	fi

	if [ ! -d ${OUTPUT_PATH} ];then
		echo "Output dir ${OUTPUT_PATH} not exist, will create it!"
		mkdir -p ${OUTPUT_PATH}
	fi

	#echo "concurrent build jobs = ${NR_JOBS}"
	if [ "${VERBOSE}" = "yes" ]; then
		NULL_DEV=
	fi

	REPORT_FILE=${OUTPUT_PATH}/${REPORT_NAME}
	
	return 0
}

function build_busybox()
{
#current dir in Kernel root
	local MESON_GEN=$1
	local ROOTFS=rootfs.cpio
	local BOARD_NAME=meson${MESON_GEN}_skt
	local IMG_FILE=${OUTPUT_PATH}m${MESON_GEN}boot.img
	local DTD_FILE=arch/arm/boot/dts/amlogic/${BOARD_NAME}.dtb

	if test ! -e ${OUTPUT_PATH}/.config; then
		echo "The file .config is absent"
		echo "You must make mesonXX_defconfig firstly, exiting..."
		return 1
	fi


	rm -f ${IMG_FILE}
	#make modules
	make  O=${OUTPUT_PATH} uImage -j ${NR_JOBS} ${NULL_DEV} 
	if [ $? -eq 0 ]; then
		echo -e "${COLOR_Y_S}Kernel uImage Build OK${COLOR_Y_E}"
	else
		echo -e "${COLOR_R_S}Kernel uImage Build failed!${COLOR_R_E}"
		return 1
	fi

	find ${ANDROID_PATH}/hardware/amlogic/ -name "*.ko" |xargs rm -f
	find ${ANDROID_PATH}/hardware/amlogic/ -name "*.o" |xargs rm -f
	find ${ANDROID_PATH}/hardware/amlogic/ -name "*.o.cmd" |xargs rm -f
	
#	make O=${OUTPUT_PATH} modules -j ${NR_JOBS} ${NULL_DEV}
#	if [ $? -eq 0 ]; then
#		echo -e "${COLOR_Y_S}Kernel modules Build OK${COLOR_Y_E}"
#	else
#		echo -e "${COLOR_R_S}Kernel modules Build failed!${COLOR_R_E}"
#		return 1
#	fi
	
	if test ! -e ./arch/arm/boot/dts/amlogic/${BOARD_NAME}.dtd;then
		echo "dtd file ${BOARD_NAME}.dtd not found, using default"
		BOARD_NAME=meson8_skt
		DTD_FILE=./arch/arm/boot/dts/amlogic/${BOARD_NAME}.dtb
	fi
	make O=${OUTPUT_PATH} ${BOARD_NAME}.dtd ${NULL_DEV}
	make O=${OUTPUT_PATH} ${BOARD_NAME}.dtb	${NULL_DEV}

	./mkbootimg --kernel ${OUTPUT_PATH}/arch/arm/boot/uImage --ramdisk ./${ROOTFS} --second ${DTD_FILE} --output ${IMG_FILE}
	#ls -l ${IMG_FILE}
	rm -f ${IMG_FILE}

	echo -e "${COLOR_Y_S}${IMG_FILE} for m${MESON_GEN} done${COLOR_Y_E}"
	return 0
}

function build_meson()
{

	INT_CNT=0

	if [ -e ${REPORT_FILE} ];then
		rm -rf ${REPORT_FILE}
	fi
	
	cd ${KERNEL_PATH}
	for MESON_GEN in ${MESON_CHIPS}
	do

		echo -e "\n${COLOR_Y_S}Build meson${MESON_GEN}${COLOR_Y_E}"

		make V=0 distclean

		echo -e "${COLOR_Y_S}   DISTCLEAN${COLOR_Y_E}"
		make V=0 O=${OUTPUT_PATH} distclean ${NULL_DEV} 


		echo -e "${COLOR_Y_S}   DEFCONFIG${COLOR_Y_E}"
		make V=0 O=${OUTPUT_PATH} meson${MESON_GEN}_defconfig ${NULL_DEV}

		echo -e "${COLOR_Y_S}   BUILDING...${COLOR_Y_E}"
		BUILD_RET=OK
		IMG_SIZE=0M

		build_busybox $MESON_GEN
		if [ $? -ne 0 ]; then
			BUILD_RET=FAILED
		else
			IMG_SIZE=`ls ${OUTPUT_PATH}/arch/arm/boot/uImage -hs | awk '{print $1}'`
		fi

		if [ "$INT_CNT" -eq 0 ]; then
			echo -n > ${REPORT_FILE}
			echo -e "Machine\t\tSize\t\tStatus" >> ${REPORT_FILE}
		fi 
		let "INT_CNT+=1"
		echo -e "m${MESON_GEN}\t\t${IMG_SIZE}\t\t${BUILD_RET}" \
			>> ${REPORT_FILE}

	done

	cd ${SCRIPT_PATH}
	return 0
}

function build_report()
{
	if [ ! -e ${REPORT_FILE} ];then
		echo "report file : ${REPORT_FILE} absent!"
		return 1
	fi

	echo -e "\n${COLOR_Y_S}---------- Build report ----------"
	cat ${REPORT_FILE}
	echo -e "\n${COLOR_Y_E}"
	return 0
}

function time_distance()
{
	T_START=$1
	T_STOP=$2

	DELTA=$(expr ${T_STOP} - ${T_START})
	HOUR=$(expr ${DELTA} / 3600)
	HOUR_REM=$(expr ${DELTA} % 3600)
	MIN=$(expr ${HOUR_REM} /  60)
	MIN_REM=$(expr ${HOUR_REM} % 60)

	#echo "T_START: ${T_START}, T_STOP: ${T_STOP}"
	#echo "DELTA: ${DELTA}, HOUR: ${HOUR}, HOUR_REM: ${HOUR_REM}, MIN: ${MIN}, MIN_REM: ${MIN_REM}"	
	echo -e "Time elapse: ${HOUR} hour, ${MIN} min, ${MIN_REM} sec\n"
}
#################################################################
#
# main()
#
#################################################################
while getopts :j:k:o:chvj var
do
	case ${var} in
	a)  	ANDROID_PATH=${OPTARG} ;;
	j)	USER_JOBS=${OPTARG} ;;
	k)	KERNEL_PATH=${OPTARG} ;;
	o)	OUTPUT_PATH=${OPTARG} ;;
	c)	HELP=no ;;
	h)	HELP=yes ;;
	v)	VERBOSE=yes;;
	\?)     usage ;;
	esac
done

args_check

TIME_START=`date +"%s"`
echo -e "\n[$$]check start at `date` with ${NR_JOBS} jobs, version: ${VERSION}"
echo -e "chip list: ${MESON_CHIPS}" 

build_meson
build_report

TIME_END=`date +"%s"`
echo -e "[$$]check finished at `date`"
time_distance ${TIME_START} ${TIME_END}

