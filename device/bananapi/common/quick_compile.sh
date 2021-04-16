#! /bin/bash

if [ $# -eq 1 ] && [ $1 == "help" ]; then
    printf "Usage:\n"
    printf "./device/bananapi/common/quick_compile.sh\n"
    printf "    select project name number and drm type to compile otapackage\n\n"
    printf "./device/bananapi/common/quick_compile.sh uboot\n"
    printf "    select project name number and drm type to compile someone uboot\n\n"
    printf "./device/bananapi/common/quick_compile.sh all\n"
    printf "    select project name number and drm type to compile all chip uboot\n\n"
    printf "./device/bananapi/common/quick_compile.sh [bootimage|dtbimage|vendorimage]\n"
    printf "    select project name number and drm type to compile sub-image\n\n"
    exit
fi
#Project Name                  SOC Name                 Hardware Name                   device/bananapi project name     uboot compile params             tdk path
project[1]="m5_mbox"      ;soc[1]="S905X3"        ;hardware[1]="BANANAPI_M5"     ; module[1]="m5_mbox"      ;        uboot[1]="bananapi_m5_v1"      ;tdk[1]="g12a/bl32.img"
project[2]="m5_tablet"    ;soc[2]="S905X3"        ;hardware[2]="BANANAPI_M5"     ; module[2]="m5_tablet"    ;        uboot[2]="bananapi_m5_v1"      ;tdk[2]="g12a/bl32.img"

platform_avb_param=""
platform_type=1
uboot_drm_type=1
project_path="null"
uboot_name="null"
tdk_name="null"

read_platform_type() {
    while true :
    do
        printf "[%3s]   [%15s]   [%15s]  [%15s]\n" "NUM" "PROJECT" "SOC TYPE" "HARDWARE TYPE"
        echo "---------------------------------------------"
        for i in `seq ${#project[@]}`;do
            printf "[%3d]   [%15s]  [%15s]  [%15s]\n" $i ${project[i]} ${soc[i]} ${hardware[i]}
        done

        echo "---------------------------------------------"
        read -p "please select platform type (default 1(Ampere)):" platform_type
        if [ ${#platform_type} -eq 0 ]; then
            platform_type=1
        fi
        if [[ $platform_type -lt 1 || $platform_type -gt ${#project[@]} ]]; then
            echo -e "The platform type is illegal!!! Need select again [1 ~ ${#project[@]}]\n"
        else
            break
        fi
    done
    project_path=${module[platform_type]}
    uboot_name=${uboot[platform_type]}
    tdk_name=${tdk[platform_type]}
}

read_android_type() {
    while true :
    do
        echo -e \
        "Select compile Android verion type lists:\n"\
        "[NUM]   [Android Version]\n" \
        "[  1]   [AOSP]\n" \
        "--------------------------------------------\n"

        read -p "Please select Android Version (default 1 (AOSP)):" uboot_drm_type
        if [ ${#uboot_drm_type} -eq 0 ]; then
            uboot_drm_type=1
            break
        fi
        if [[ $uboot_drm_type -ne 1 ]];then
            echo -e "The Android Version is illegal, please select again [1]}\n"
        else
            break
        fi
    done
}

compile_uboot(){
    if [ $uboot_drm_type -gt 1 ]; then
        echo -e "[./mk $uboot_name --bl32 ../../vendor/amlogic/common/tdk/secureos/$tdk_name --systemroot]\n"
        ./mk $uboot_name --bl32 ../../vendor/amlogic/common/tdk/secureos/$tdk_name --systemroot ;
    else
        echo -e "[./mk $uboot_name --systemroot]"
        ./mk $uboot_name --systemroot;
    fi

    cp build/u-boot.bin ../../device/bananapi/$project_path/bootloader.img;
    cp build/u-boot.bin.usb.bl2 ../../device/bananapi/$project_path/upgrade/u-boot.bin.usb.bl2;
    cp build/u-boot.bin.usb.tpl ../../device/bananapi/$project_path/upgrade/u-boot.bin.usb.tpl;
    cp build/u-boot.bin.sd.bin ../../device/bananapi/$project_path/upgrade/u-boot.bin.sd.bin;
}

if [ $# -eq 1 ]; then
    if [ $1 == "uboot" ]; then
        read_platform_type
        read_android_type
        cd bootloader/uboot-repo
        compile_uboot
        cd ../../
        exit
    fi

    if [ $1 == "all" ]; then
        read_android_type
        cd bootloader/uboot-repo
        for platform_type in `seq ${#project[@]}`;do
            project_path=${module[platform_type]}
            uboot_name=${uboot[platform_type]}
            tdk_name=${tdk[platform_type]}
            compile_uboot
        done
        cd ../../
        echo -e "device: update uboot [1/1]\n"
        echo -e "PD#SWPL-919\n"
        echo -e "Problem:"
        echo -e "need update bootloader\n"
        echo "Solution:"
        cd bootloader/uboot-repo/bl2/bin/
        echo "bl2       : "$(git log --pretty=format:"%H" -1); cd ../../../../
        cd bootloader/uboot-repo/bl30/bin/
        echo "bl30      : "$(git log --pretty=format:"%H" -1); cd ../../../../
        cd bootloader/uboot-repo/bl31/bin/
        echo "bl31      : "$(git log --pretty=format:"%H" -1); cd ../../../../
        cd bootloader/uboot-repo/bl31_1.3/bin/
        echo "bl31_1.3  : "$(git log --pretty=format:"%H" -1); cd ../../../../
        cd bootloader/uboot-repo/bl33/v2015
        echo "bl33      : "$(git log --pretty=format:"%H" -1); cd ../../../../
        cd bootloader/uboot-repo/fip/
        echo "fip       : "$(git log --pretty=format:"%H" -1)
        echo -e; cd ../../../../
        echo "Verify:"; echo "no need verify"
        exit
    fi
fi

read_platform_type
read_android_type
source build/envsetup.sh
if [ $uboot_drm_type -eq 2 ]; then
    export  BOARD_COMPILE_CTS=true
elif [ $uboot_drm_type -eq 3 ]; then
    if [ ! -d "vendor/google" ];then
        echo "==========================================="
        echo "There is not Google GMS in vendor directory"
        echo "==========================================="
        exit
    fi
    export  BOARD_COMPILE_ATV=true
fi
lunch "${project_path}-userdebug"
if [ $# -eq 1 ]; then
    if [ $1 == "bootimage" ] \
    || [ $1 == "logoimg" ] \
    || [ $1 == "recoveryimage" ] \
    || [ $1 == "systemimage" ] \
    || [ $1 == "vendorimage" ] \
    || [ $1 == "odm_image" ] \
    || [ $1 == "dtbimage" ] ; then
        make $1 -j8
        exit
    fi
fi
cd bootloader/uboot-repo
compile_uboot
cd ../../
make otapackage -j8


