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
#Project Name             SOC Name                Hardware Name                  device/bananapi project name   uboot compile params                 tdk path
project[1]="m5_mbox"      ;soc[1]="S905X3"        ;hardware[1]="BANANAPI_M5"     ;module[1]="m5_mbox"           ;uboot[1]="bananapi_m5_v1"           ;tdk[1]="g12a/bl32.img"
project[2]="m5_tablet"    ;soc[2]="S905X3"        ;hardware[2]="BANANAPI_M5"     ;module[2]="m5_tablet"         ;uboot[2]="bananapi_m5_v1"           ;tdk[2]="g12a/bl32.img"

platform_avb_param=""
platform_type=1
project_path="null"
uboot_name="null"

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
}

compile_uboot(){
    echo -e "[./mk $uboot_name --systemroot]"
    ./mk $uboot_name --systemroot;

    cp build/u-boot.bin ../../device/bananapi/$project_path/bootloader.img;
    cp build/u-boot.bin.usb.bl2 ../../device/bananapi/$project_path/upgrade/u-boot.bin.usb.bl2;
    cp build/u-boot.bin.usb.tpl ../../device/bananapi/$project_path/upgrade/u-boot.bin.usb.tpl;
    cp build/u-boot.bin.sd.bin ../../device/bananapi/$project_path/upgrade/u-boot.bin.sd.bin;
}

read_platform_type
source build/envsetup.sh
lunch "${project_path}-userdebug"
make installclean

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


