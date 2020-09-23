#! /bin/bash

if [ $# -eq 1 ] && [ $1 == "help" ]; then
    printf "Usage:\n"
    printf "./device/amlogic/common/quick_compile.sh\n"
    printf "    select project name number and drm type to compile otapackage\n\n"
    printf "./device/amlogic/common/quick_compile.sh uboot\n"
    printf "    select project name number and drm type to compile someone uboot\n\n"
    printf "./device/amlogic/common/quick_compile.sh all\n"
    printf "    select project name number and drm type to compile all chip uboot\n\n"
    printf "./device/amlogic/common/quick_compile.sh [bootimage|dtbimage|vendorimage]\n"
    printf "    select project name number and drm type to compile sub-image\n\n"
    exit
fi
#Project Name                  SOC Name                    Hardware Name          device/amlogic project name     uboot compile params           tdk path
project[1]="Ampere"        ;soc[1]="S905X"          ;hardware[1]="P212/P215"    ; module[1]="ampere"        ;uboot[1]="gxl_p212_v1"          ;tdk[1]="gx/bl32.img"
project[2]="Anning"        ;soc[2]="S805Y"          ;hardware[2]="P244"         ; module[2]="anning"        ;uboot[2]="gxl_p244_v1"          ;tdk[2]="gx/bl32.img"
project[3]="Braun"         ;soc[3]="S905D"          ;hardware[3]="P230"         ; module[3]="braun"         ;uboot[3]="gxl_p230_v1"          ;tdk[3]="gx/bl32.img"
project[4]="Curie"         ;soc[4]="S805X"          ;hardware[4]="P241"         ; module[4]="curie"         ;uboot[4]="gxl_p241_v1"          ;tdk[4]="gx/bl32.img"
project[5]="Darwin"        ;soc[5]="T962E"          ;hardware[5]="R321"         ; module[5]="darwin"        ;uboot[5]="txlx_t962e_r321_v1"   ;tdk[5]="txlx/bl32.img"
project[6]="Einstein"      ;soc[6]="T962X"          ;hardware[6]="R311"         ; module[6]="einstein"      ;uboot[6]="txlx_t962x_r311_v1"   ;tdk[6]="txlx/bl32.img"
project[7]="Faraday"       ;soc[7]="S905Y2"         ;hardware[7]="U223"         ; module[7]="faraday"       ;uboot[7]="g12a_u223_v1"         ;tdk[7]="g12a/bl32.img"
project[8]="Fermi"         ;soc[8]="S905D2/S905D3"  ;hardware[8]="U200/AC200"   ; module[8]="fermi"         ;uboot[8]="g12a_u200_v1"         ;tdk[8]="g12a/bl32.img"
project[9]="Franklin"      ;soc[9]="S905X2/S905X3"  ;hardware[9]="U212/AC213"   ; module[9]="franklin"      ;uboot[9]="g12a_u212_v1"         ;tdk[9]="g12a/bl32.img"
project[10]="Galilei"      ;soc[10]="S922X/S922Z"   ;hardware[10]="W200"        ; module[10]="galilei"      ;uboot[10]="g12b_w200_v1"        ;tdk[10]="g12a/bl32.img"
project[11]="Hertz"        ;soc[11]="S912"          ;hardware[11]="Q201"        ; module[11]="hertz"        ;uboot[11]="gxm_q201_v1"         ;tdk[11]="gx/bl32.img"
project[12]="Lyell"        ;soc[12]="T962"          ;hardware[12]="P321"        ; module[12]="lyell"        ;uboot[12]="txl_p321_v1"         ;tdk[12]="gx/bl32.img"
project[13]="Marconi"      ;soc[13]="T962X2"        ;hardware[13]="X301"        ; module[13]="marconi"      ;uboot[13]="tl1_x301_v1"         ;tdk[13]="tl1/bl32.img"
project[14]="G12B_SKT"     ;soc[14]="S922X/S922Z"   ;hardware[14]="G12B_SKT"    ; module[14]="g12b_skt"     ;uboot[14]="g12b_skt_v1"         ;tdk[14]="g12a/bl32.img"
project[15]="T962_p321"    ;soc[15]="T962"          ;hardware[15]="P321"        ; module[15]="t962_p321"    ;uboot[15]="txl_p321_v1"         ;tdk[15]="gx/bl32.img"
project[16]="t962x2_skt"   ;soc[16]="T962X2"        ;hardware[16]="T962_SKT"    ; module[16]="t962x2_skt"   ;uboot[16]="tl1_skt_v1"          ;tdk[16]="tl1/bl32.img"
project[17]="t962x2_t309"  ;soc[17]="T962X2"        ;hardware[17]="T309"        ; module[17]="t962x2_t309"  ;uboot[17]="tl1_t309_v1"         ;tdk[17]="tl1/bl32.img"
project[18]="t962x_r314"   ;soc[18]="T962X"         ;hardware[18]="R314"        ; module[18]="t962x_r314"   ;uboot[18]="txlx_t962x_r314_v1"  ;tdk[18]="txlx/bl32.img"
project[19]="t962e2_skt"   ;soc[19]="T962E2"        ;hardware[19]="T962E2_SKT"  ; module[19]="t962e2_skt"   ;uboot[19]="tm2_t962e2_ab319_v1" ;tdk[19]="tm2/bl32.img"
project[20]="Dalton"       ;soc[20]="T962E2"        ;hardware[20]="AB311"       ; module[20]="dalton"       ;uboot[20]="tm2_t962e2_ab311_v1" ;tdk[20]="tm2/bl32.img"
project[21]="t962x3_skt"   ;soc[21]="T962X3"        ;hardware[21]="T962X3_SKT"  ; module[21]="t962x3_skt"   ;uboot[21]="tm2_t962x3_ab309_v1" ;tdk[21]="tm2/bl32.img"
project[22]="t962x3_ab301" ;soc[22]="T962X3"        ;hardware[22]="AB301"       ; module[22]="t962x3_ab301" ;uboot[22]="tm2_t962x3_ab301_v1" ;tdk[22]="tm2/bl32.img"
project[23]="U202"         ;soc[23]="S905D2/S905D3" ;hardware[23]="U202/AC202"  ; module[23]="u202"         ;uboot[23]="g12a_u202_v1"        ;tdk[23]="g12a/bl32.img"
project[24]="Newton"       ;soc[24]="S905X3"        ;hardware[24]="AC214"       ; module[24]="newton"       ;uboot[24]="sm1_ac214_v1"        ;tdk[24]="g12a/bl32.img"
project[25]="Neumann"      ;soc[25]="S905D3"        ;hardware[25]="AC200"       ; module[25]="neumann"      ;uboot[25]="sm1_ac200_v1"        ;tdk[25]="g12a/bl32.img"
project[26]="P281"         ;soc[26]="S905W"         ;hardware[26]="P281"        ; module[26]="p281"         ;uboot[26]="gxl_p281_v1"         ;tdk[26]="gx/bl32.img"
project[27]="T962x3_t312"  ;soc[27]="T962X3"        ;hardware[27]="T312"        ; module[27]="t962x3_t312"  ;uboot[27]="tm2_t962x3_t312_v1"  ;tdk[27]="tm2/bl32.img"
project[28]="AC214"        ;soc[28]="S905X3"        ;hardware[28]="AC214"       ; module[28]="ac214"        ;uboot[28]="sm1_ac214_v1"        ;tdk[28]="g12a/bl32.img"
project[29]="bananapi_m5"  ;soc[29]="S905X3"        ;hardware[29]="BANANAPI_M5" ; module[29]="bananapi_m5"  ;uboot[29]="bananapi_m5_v1"      ;tdk[29]="g12a/bl32.img"

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
        "[  2]   [ DRM]\n" \
        "[  3]   [GTVS](need google gms zip)\n" \
        "--------------------------------------------\n"

        read -p "Please select Android Version (default 1 (AOSP)):" uboot_drm_type
        if [ ${#uboot_drm_type} -eq 0 ]; then
            uboot_drm_type=1
            break
        fi
        if [[ $uboot_drm_type -lt 1 || $uboot_drm_type -gt 3 ]];then
            echo -e "The Android Version is illegal, please select again [1 ~ 3]}\n"
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

    cp build/u-boot.bin ../../device/amlogic/$project_path/bootloader.img;
    cp build/u-boot.bin.usb.bl2 ../../device/amlogic/$project_path/upgrade/u-boot.bin.usb.bl2;
    cp build/u-boot.bin.usb.tpl ../../device/amlogic/$project_path/upgrade/u-boot.bin.usb.tpl;
    cp build/u-boot.bin.sd.bin ../../device/amlogic/$project_path/upgrade/u-boot.bin.sd.bin;
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


