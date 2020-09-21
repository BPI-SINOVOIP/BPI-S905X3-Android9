#!/bin/bash -e

# Copyright (c) 2018 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.

#set -x

VERSION=1.6

# Path to aml_encrypt_gxl
TOOL_PATH=$(dirname $(readlink -f $0))

SCRIPT_PATH=$(dirname $(readlink -f $0))

if [ -z "$EXTERNAL_ENCRYPT_PATH" ]; then
    EXTERNAL_ENCRYPT_PATH=.
fi

if [ -z "$PRIVATE_KEY_PATH" ]; then
    PRIVATE_KEY_PATH=.
fi

usage() {
    cat << EOF
Usage: $(basename $0) --help
       $(basename $0) --version
       $(basename $0) --generate-efuse-pattern \\
                      --soc [gxl | txlx | axg | g12a | sm1 | a1 | c1 ] \\
                      [--soc-rev [a | b]] \\
                      [--root-hash rootkeys.hash] \\
                      [--password-hash password.hash] \\
                      [--scan-password-hash password.hash] \\
                      [--aes-key aeskey.bin] \\
                      [--m4-root-hash rootkeys.hash] \\
                      [--m4-aes-key aeskey.bin] \\
                      [--enable-sb true] \\
                      [--enable-aes true] \\
                      [--enable-jtag-password true] \\
                      [--enable-usb-password true] \\
                      [--enable-scan-password true] \\
                      [--enable-anti-rollback true] \\
                      [--disable-boot-usb true] \\
                      [--disable-boot-spi true] \\
                      [--disable-boot-sdcard true] \\
                      [--disable-boot-nand-emmc true] \\
                      [--disable-boot-recover true] \\
                      [--disable-scan-chain true] \\
                      [--disable-print true] \\
                      [--disable-jtag true] \\
                      [--revoke-rsk-0 true] \\
                      [--revoke-rsk-1 true] \\
                      [--revoke-rsk-2 true] \\
                      [--revoke-rsk-3 true] \\
                      [--user-efuse-file <path-to-general-purpose-user-efuse-data>] \\
                      [--kwrap <kwrap file>] \\
                      [--key-hash-ver [1 | 2]] \\
                      [--raw-otp-pattern true] \\
                      -o pattern.efuse

       Additional options for a1/c1 soc:
            --disable-boot-spi-nand true
            --disable-boot-spi-nor  true
            --block1 block1.bin
            --acgk acgk.bin
            --acrk acrk.bin
            --dvgk dvgk.bin
            --dvuk dvuk.bin
       Additional options for c1 soc:
            --disable-boot-emmc true
            --disable-boot-nand true
EOF
    exit 1
}

check_file() {
    if [ ! -f "$2" ]; then echo Error: Unable to open $1: \""$2"\"; exit 1 ; fi
}

# Check file is size or exit. $1: file, $2: size
check_size() {
    local filesize=$(wc -c < "$1")
    if [ $filesize -ne $2 ]; then
        echo "Error: File \"$1\" incorrect size. Was $filesize, expected $2"
        exit 1
    fi
}

# Check optional file argument exists and is given size
# $1 arg name
# $2 size
# $3 file
check_opt_file() {
    if [ -n "$3" ]; then
        check_file "$1" "$3"
        local filesize=$(wc -c < "$3")
        if [ $filesize -ne $2 ]; then
            echo "Incorrect size $filesize != $2 for $1 $3"
            exit 1
        fi
    fi
}

check_boolean() {
    if [ "$2" != "true" ] && [ "$2" != "false" ]; then
        echo Error: invalid value $1: \""$2"\"
        exit 1
    fi
}

kwrap=""
wrlock_kwrap="false"
roothash=""
passwordhash=""
scanpasswordhash=""
userefusefile=""
aeskey=""
m4roothash=""
m4aeskey=""
enablesb="false"
enableaes="false"
enablejtagpassword="false"
enableusbpassword="false"
enablescanpassword="false"
enableantirollback="false"
disablebootusb="false"
disablebootspi="false"
disablebootspinand="false"
disablebootspinor="false"
disablebootsdcard="false"
disablebootemmc="false"
disablebootnand="false"
disablebootnandemmc="false"
disablebootrecover="false"
disableprint="false"
disablejtag="false"
disablescanchain="false"
revokersk0="false"
revokersk1="false"
revokersk2="false"
revokersk3="false"
output=""
sigver=""
keyhashver=""
soc="gxl"
socrev="a"
opt_raw_otp_pattern="false"
block1=""
acgk=""
acrk=""
dvgk=""
dvuk=""

# SBC option ONLY applicable to GXL
# Enabled automatically if SoC is GXL using this script
enablesbc="false"

# FKC option ONLY applicable to GXL *AND* TXLX Rev-A
# FKC *MUST* use keyhash v2 in GXL and TXLX Rev-A
# FKC automatically enable SBC in GXL
enablefkc="false"

generate_efuse_pattern() {
    local argv=("$@")
    local i=0

    local patt=$(mktemp --tmpdir)
    local license=$(mktemp --tmpdir)
    local license2=$(mktemp --tmpdir)
    local wrlock=$(mktemp --tmpdir)

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        #echo "i=$i arg=\"$arg\""
        i=$((i + 1))
        #echo "i=$i argv[$i]=${argv[$i]}"
        case "$arg" in
            --kwrap)
                kwrap="${argv[$i]}" ;;
            --root-hash)
                roothash="${argv[$i]}" ;;
            --password-hash)
                passwordhash="${argv[$i]}" ;;
            --scan-password-hash)
                scanpasswordhash="${argv[$i]}" ;;
            --aes-key)
                aeskey="${argv[$i]}" ;;
            --m4-root-hash)
                m4roothash="${argv[$i]}" ;;
            --m4-aes-key)
                m4aeskey="${argv[$i]}" ;;
            --enable-sb)
                enablesb="${argv[$i]}" ;;
            --enable-aes)
                enableaes="${argv[$i]}" ;;
            --enable-jtag-password)
                enablejtagpassword="${argv[$i]}" ;;
            --enable-usb-password)
                enableusbpassword="${argv[$i]}" ;;
            --enable-scan-password)
                enablescanpassword="${argv[$i]}" ;;
            --enable-anti-rollback)
                enableantirollback="${argv[$i]}" ;;
            --disable-boot-usb)
                disablebootusb="${argv[$i]}" ;;
            --disable-boot-spi)
                disablebootspi="${argv[$i]}" ;;
            --disable-boot-spi-nand)
                disablebootspinand="${argv[$i]}" ;;
            --disable-boot-spi-nor)
                disablebootspinor="${argv[$i]}" ;;
            --disable-boot-sdcard)
                disablebootsdcard="${argv[$i]}" ;;
            --disable-boot-emmc)
                disablebootemmc="${argv[$i]}" ;;
            --disable-boot-nand)
                disablebootnand="${argv[$i]}" ;;
            --disable-boot-nand-emmc)
                disablebootnandemmc="${argv[$i]}" ;;
            --disable-boot-recover)
                disablebootrecover="${argv[$i]}" ;;
            --disable-print)
                disableprint="${argv[$i]}" ;;
            --disable-jtag)
                disablejtag="${argv[$i]}" ;;
            --disable-scan-chain)
                disablescanchain="${argv[$i]}" ;;
            --revoke-rsk-0)
                revokersk0="${argv[$i]}" ;;
            --revoke-rsk-1)
                revokersk1="${argv[$i]}" ;;
            --revoke-rsk-2)
                revokersk2="${argv[$i]}" ;;
            --revoke-rsk-3)
                revokersk3="${argv[$i]}" ;;
            --user-efuse-file)
                userefusefile="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            --sig-ver)
                sigver="${argv[$i]}" ;;
            --key-hash-ver)
                keyhashver="${argv[$i]}" ;;
            --generate-efuse-pattern)
                i=$((i - 1))
                ;;
            --raw-otp-pattern)
                opt_raw_otp_pattern="${argv[$i]}" ;;
            --soc)
                soc="${argv[$i]}" ;;
            --soc-rev)
                socrev="${argv[$i]}" ;;
            --block1) block1="${argv[$i]}" ;;
            --acgk) acgk="${argv[$i]}" ;;
            --acrk) acrk="${argv[$i]}" ;;
            --dvgk) dvgk="${argv[$i]}" ;;
            --dvuk) dvuk="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done

    # Verify args
    if [ -z "$output" ]; then echo Error: Missing output file option -o; exit 1; fi

    if [ -z "$sigver" ]; then
        sigver=1
    fi

    if [ -z "$keyhashver" ]; then
        keyhashver=1
    fi

    # Alias
    if [ "$soc" == "a112" ]; then
        soc=gxl
    fi

    if [ "$soc" == "a113" ]; then
        soc=axg
    fi
    if [ "$soc" == "s905d2" ]; then
        soc=g12a
    fi
    if [ "$soc" == "s905d3" ]; then
        soc=sm1
    fi

    if [ "$soc" != "gxl" ] && [ "$soc" != "axg" ] && [ "$soc" != "txlx" ] &&
            [ "$soc" != "g12a" ] && [ "$soc" != "sm1" ] && [ "$soc" != "a1" ] &&
            [ "$soc" != "c1" ]; then
        echo Error: invalid soc: \"$soc\"
        exit 1
    fi

    # Starting from TXLX and AXG, only full key hash (v2) is supported
    if [ "$soc" == "txlx" ] || [ "$soc" == "axg" ] || [ "$soc" == "g12a" ] || [ "$soc" == "sm1" ]; then
        keyhashver=2
    fi
    if [ "$soc" == "a1" ] || [ "$soc" == "c1" ]; then
        keyhashver=3
    fi

    if [ "$soc" == "gxl" ]; then
        # Safe to enable SBC for *ALL* version of GXL
        # SBC has no effect on pre-Rev-D GXL
        enablesbc="true"

        # Keyhash v2 requires enable FKC in GXL
        # FKC is *ONLY* available in GXL Rev-D or later
        # Pre-Rev-D GXL should *NOT* use FKC
        if [ $keyhashver -eq 2 ]; then
            enablefkc="true"
        fi
    fi

    if [ "$soc" == "txlx" ]; then
        # Default to Rev-A as pattern for Rev-A is compatible for B or newer
        if [ -z "$socver" ]; then
            socver="a"
        fi

        if [ "$socrev" == "a" ]; then
            enablefkc="true"
        fi
    fi

    if [ "$kwrap" != "" ]; then
        check_file kwrap "$kwrap"
        filesize=$(wc -c < ${kwrap})
        if [ $filesize -ne 16 ]; then
            echo "kwrap incorrect size $filesize != 16"
            exit 1
        fi
        wrlock_kwrap=true
    fi
    if [ "$soc" != "g12a" ] && [ "$soc" != "sm1" ] ; then
        if [ "$m4roothash" != "" ] || [ "$m4aeskey" != "" ]; then
            echo "M4 key given but not supported for $soc"
            exit 1
        fi
    fi
    if [ "$soc" != "a1" ] && [ "$soc" != "c1" ]; then
        if [ -n "$block1" ] || [ -n "$acgk" ] || [ -n "$acrk" ] ||
                [ -n "$dvgk" ] || [ -n "$dvuk" ]; then
            echo "block1|acgk|acrk|dvgk|dvuk given but not supported for $soc"
            exit 1
        fi
    fi

    check_boolean enable-sb "$enablesb"
    check_boolean enable-aes "$enableaes"
    check_boolean enable-jtag-password "$enablejtagpassword"
    check_boolean enable-usb-password "$enableusbpassword"
    check_boolean enable-jtag-password "$enablescanpassword"
    check_boolean enable-anti-rollback "$enableantirollback"
    check_boolean disable-boot-usb "$disablebootusb"
    check_boolean disable-boot-spi "$disablebootspi"
    check_boolean disable-boot-spi-nand "$disablebootspinand"
    check_boolean disable-boot-spi-nor "$disablebootspinor"
    check_boolean disable-boot-sdcard "$disablebootsdcard"
    check_boolean disable-boot-emmc "$disablebootemmc"
    check_boolean disable-boot-nand "$disablebootnand"
    check_boolean disable-boot-nand-emmc "$disablebootnandemmc"
    check_boolean disable-boot-recover "$disablebootrecover"
    check_boolean disable-print "$disableprint"
    check_boolean disable-jtag "$disablejtag"
    check_boolean disable-scan-chain "$disablescanchain"
    check_boolean revoke-rsk-0 "$revokersk0"
    check_boolean revoke-rsk-1 "$revokersk1"
    check_boolean revoke-rsk-2 "$revokersk2"
    check_boolean revoke-rsk-3 "$revokersk3"
    check_boolean fkc "$enablefkc"
    check_boolean raw-otp-pattern "$opt_raw_otp_pattern"

    if [ "$soc" != "a1" ] && [ "$soc" != "c1" ] && ( $disablebootspinand || $disablebootspinor ); then
        echo "Error: disable-boot-spi-nand and disable-boot-spi-nor not supported"
        exit 1;
    fi

    if [ "$roothash" != "" ]; then
        check_file roothash "$roothash"
        # Check size
        local filesize=$(wc -c < ${roothash})
        if [ $filesize -ne 32 ]; then
            echo "Root key hash incorrect size $filesize != 32"
            exit 1
        fi
    fi
    if [ "$passwordhash" != "" ]; then
        check_file passwordhash "$passwordhash"
        # Check size
        local filesize=$(wc -c < ${passwordhash})
        if [ $filesize -ne 32 ]; then
            echo "Password key hash incorrect size $filesize != 32"
            exit 1
        fi
    fi
    if [ "$scanpasswordhash" != "" ]; then
        check_file scanpasswordhash "$scanpasswordhash"
        check_size "$scanpasswordhash" 32
    fi
    if [ "$aeskey" != "" ]; then
        check_file aeskey "$aeskey"
        filesize=$(wc -c < ${aeskey})
        if [ $filesize -ne 32 ]; then
            echo "AES key incorrect size $filesize != 32"
            exit 1
        fi
    fi
    if [ "$m4roothash" != "" ]; then
        check_file m4roothash "$m4roothash"
        # Check size
        local filesize=$(wc -c < ${m4roothash})
        if [ $filesize -ne 32 ]; then
            echo "M4 Root key hash incorrect size $filesize != 32"
            exit 1
        fi
    fi
    if [ "$m4aeskey" != "" ]; then
        check_file m4aeskey "$m4aeskey"
        filesize=$(wc -c < ${m4aeskey})
        if [ $filesize -ne 16 ]; then
            echo "M4 AES key incorrect size $filesize != 16"
            exit 1
        fi
    fi

    if [ "$soc" == "gxl" ] || [ "$soc" == "txlx" ]; then
        generate_efuse_pattern_gxl
    elif [ "$soc" == "g12a" ]; then
        generate_efuse_pattern_g12a
    elif [ "$soc" == "sm1" ]; then
        generate_efuse_pattern_sm1
    elif [ "$soc" == "a1" ] || [ "$soc" == "c1" ]; then
        generate_efuse_pattern_a1
    elif [ "$soc" == "axg" ]; then
        generate_efuse_pattern_axg
    else
        echo "Error: internal error"
    fi

    rm -f $patt
    rm -f $license
    rm -f $license2
    rm -f $wrlock
}

generate_efuse_pattern_gxl() {
    echo -n "Generate OTP pattern for GXL/TXLX (A112, S905X, S905D, T962E, T962X) ... "

    # Additional args check
    if [ "$userefusefile" != "" ]; then
        check_file userefusefile "$userefusefile"
        local filesize=$(wc -c < ${userefusefile})
        if [ $filesize -ne 192 ]; then
            echo "User efuse file incorrect size $filesize != 192"
            exit 1
        fi
    fi

    # Create input pattern

    # OTP write lock control
    b_18="00"
    b_19="00"
    b_1b="00"

    # Construct license bits
    b4="00"
    if [ "$disablebootusb" == "true" ]; then
       b4="$(printf %02x $(( 0x$b4 | 0x10 )))"
    fi
    if [ "$disablebootspi" == "true" ]; then
       b4="$(printf %02x $(( 0x$b4 | 0x20 )))"
    fi
    if [ "$disablebootsdcard" == "true" ]; then
       b4="$(printf %02x $(( 0x$b4 | 0x40 )))"
    fi
    if [ "$disablebootnandemmc" == "true" ]; then
       b4="$(printf %02x $(( 0x$b4 | 0x80 )))"
    fi

    b5="00"
    if [ "$disablebootrecover" == "true" ]; then
       b5="$(printf %02x $(( 0x$b5 | 0x01 )))"
    fi
    if [ "$disableprint" == "true" ]; then
       b5="$(printf %02x $(( 0x$b5 | 0x02 )))"
    fi
    if [ "$disablejtag" == "true" ]; then
       b5="$(printf %02x $(( 0x$b5 | 0x04 )))"
    fi
    if $revokersk0; then
       b5="$(printf %02x $(( 0x$b5 | 0x10 )))"
    fi
    if $revokersk1; then
       b5="$(printf %02x $(( 0x$b5 | 0x20 )))"
    fi
    if $revokersk2; then
       b5="$(printf %02x $(( 0x$b5 | 0x40 )))"
    fi
    if $revokersk3; then
       b5="$(printf %02x $(( 0x$b5 | 0x80 )))"
    fi

    b6="00"
    if [ "$enablesb" == "true" ]; then
        b6="$(printf %02x $(( 0x$b6 | 0x04 )))"
    fi
    if [ "$enablescanpassword" == "true" ]; then
        b6="$(printf %02x $(( 0x$b6 | 0x08 )))"
    fi
    if [ "$enablejtagpassword" == "true" ]; then
        b6="$(printf %02x $(( 0x$b6 | 0x10 )))"
    fi

    b7="00"
    if [ "$enableaes" == "true" ]; then
        b7="$(printf %02x $(( 0x$b7 | 0x10 )))"
    fi

    b8="00"
    if [ "$enableantirollback" == "true" ]; then
        b8="$(printf %02x $(( 0x$b8 | 0x01 )))"
    fi
    if [ "$enableusbpassword" == "true" ]; then
        b8="$(printf %02x $(( 0x$b8 | 0x02 )))"
    fi
    if [ "$enablesbc" == "true" ]; then
        b8="$(printf %02x $(( 0x$b8 | 0x10 )))"
    fi
    if [ "$enablefkc" == "true" ]; then
        b8="$(printf %02x $(( 0x$b8 | 0x30 )))"
    fi

    b9="00"
    if [ "$disablescanchain" == "true" ]; then
        b9="$(printf %02x $(( 0x$b9 | 0x04 )))"
    fi

    # Generate empty eFUSE pattern data
    dd if=/dev/zero of=$patt count=512 bs=1 &> /dev/null

    # Create and write license block
    if [ ${#b4} -ne 2 ] || [ ${#b5} -ne 2 ] || [ ${#b6} -ne 2 ] || [ ${#b7} -ne 2 ] || [ ${#b8} -ne 2 ] || [ ${#b9} -ne 2 ]; then
        echo Internal Error
        exit 1
    fi

    echo 00 00 00 00 $b4 $b5 $b6 $b7 $b8 $b9 00 00 00 00 00 00 | xxd -r -p > $license
    filesize=$(wc -c < $license)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid license pattern length
        exit 1
    fi

    dd if=$license of=$patt bs=1 seek=0 count=16 \
        conv=notrunc >& /dev/null

    if [ "$roothash" != "" ]; then
        dd if=$roothash of=$patt bs=1 seek=48 count=32 \
            conv=notrunc >& /dev/null

        # Lock root hash blocks
        b_18="$(printf %02x $(( 0x$b_18 | 0x18 )))"
    fi

    if [ "$aeskey" != "" ]; then
        dd if=$aeskey of=$patt bs=1 seek=80 count=32 \
            conv=notrunc >& /dev/null

        # Lock aes key blocks
        b_18="$(printf %02x $(( 0x$b_18 | 0x60 )))"
    fi

    if [ "$passwordhash" != "" ]; then
        dd if=$passwordhash of=$patt bs=1 seek=128 count=32 \
            conv=notrunc >& /dev/null

        # Lock jtag password hash blocks
        b_19="$(printf %02x $(( 0x$b_19 | 0x03 )))"
    fi
    if [ "$scanpasswordhash" != "" ]; then
        dd if=$scanpasswordhash of=$patt bs=1 seek=160 count=32 \
            conv=notrunc >& /dev/null

        # Lock scan password hash blocks
        b_19="$(printf %02x $(( 0x$b_19 | 0x0c )))"
    fi

    if [ "$userefusefile" != "" ]; then
        dd if=$userefusefile of=$patt bs=1 seek=320 count=192 \
            conv=notrunc >& /dev/null
    fi

    # Write kwrap (in user data area) last
    if [ "$kwrap" != "" ]; then
        dd if=$kwrap of=$patt bs=1 seek=496 count=16 \
            conv=notrunc >& /dev/null

        if $wrlock_kwrap; then
            b_1b="$(printf %02x $(( 0x$b_1b | 0x80 )))"
        fi
    fi

    # Create and write write-lock
    if [ ${#b_18} -ne 2 ] || [ ${#b_19} -ne 2 ] || [ ${#b_1b} -ne 2 ]; then
        echo Internal Error
        exit 1
    fi

    echo 00 00 00 00 00 00 00 00 $b_18 $b_19 00 $b_1b 00 00 00 00 | xxd -r -p > $wrlock
    filesize=$(wc -c < $wrlock)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid write-lock pattern length
        exit 1
    fi

    dd if=$wrlock of=$patt bs=1 seek=16 count=16 \
        conv=notrunc >& /dev/null

    if [ "$opt_raw_otp_pattern" == "true" ]; then
        cp $patt $output
    else
        ${TOOL_PATH}/aml_encrypt_gxl --efsgen3 --input $patt --output $output
    fi

    echo "done"
}

generate_efuse_pattern_axg() {
    echo -n "Generate OTP pattern for AXG (A113) ... "

    # Additional args check
    if [ "$userefusefile" != "" ]; then
        check_file userefusefile "$userefusefile"
        local filesize=$(wc -c < ${userefusefile})
        if [ $filesize -ne 16 ]; then
            echo "User efuse file incorrect size $filesize != 16"
            exit 1
        fi
    fi

    if [ "$disablebootrecover" == "true" ]; then
        echo Boot recovery is not supported on $soc, option ignored
    fi

    # Create input pattern

    # OTP write lock control
    b_b1="00"

    # Construct license bits
    b5="00"
    if [ "$disablebootusb" == "true" ]; then
        b5="$(printf %02x $(( 0x$b5 | 0x01 )))"
    fi
    if [ "$disablebootspi" == "true" ]; then
        b5="$(printf %02x $(( 0x$b5 | 0x02 )))"
    fi
    if [ "$disablebootsdcard" == "true" ]; then
        b5="$(printf %02x $(( 0x$b5 | 0x04 )))"
    fi
    if [ "$disablebootnandemmc" == "true" ]; then
        b5="$(printf %02x $(( 0x$b5 | 0x08 )))"
    fi
    if [ "$disableprint" == "true" ]; then
        b5="$(printf %02x $(( 0x$b5 | 0x10 )))"
    fi
    if [ "$disablejtag" == "true" ]; then
        b5="$(printf %02x $(( 0x$b5 | 0x20 )))"
    fi
    if [ "$enablejtagpassword" == "true" ]; then
        b5="$(printf %02x $(( 0x$b5 | 0x40 )))"
    fi
    if [ "$enableusbpassword" == "true" ]; then
        b5="$(printf %02x $(( 0x$b5 | 0x80 )))"
    fi

    b6="00"
    if [ "$enablesb" == "true" ]; then
        b6="$(printf %02x $(( 0x$b6 | 0x01 )))"
    fi
    if [ "$enableaes" == "true" ]; then
        b6="$(printf %02x $(( 0x$b6 | 0x02 )))"
    fi
    if $revokersk0; then
        b6="$(printf %02x $(( 0x$b6 | 0x04 )))"
    fi
    if $revokersk1; then
        b6="$(printf %02x $(( 0x$b6 | 0x08 )))"
    fi
    if $revokersk2; then
        b6="$(printf %02x $(( 0x$b6 | 0x10 )))"
    fi
    if $revokersk3; then
        b6="$(printf %02x $(( 0x$b6 | 0x20 )))"
    fi
    if [ "$enableantirollback" == "true" ]; then
        b6="$(printf %02x $(( 0x$b6 | 0x80 )))"
    fi

    b7="00"
    if [ "$disablescanchain" == "true" ]; then
        b7="$(printf %02x $(( 0x$b7 | 0x01 )))"
    fi
    if [ "$enablescanpassword" == "true" ]; then
        b7="$(printf %02x $(( 0x$b7 | 0x02 )))"
    fi

    # Generate empty eFUSE pattern data
    dd if=/dev/zero of=$patt count=256 bs=1 &> /dev/null

    if [ "$roothash" != "" ]; then
        dd if=$roothash of=$patt bs=1 seek=32 count=32 \
            conv=notrunc >& /dev/null
        #TODO lock root hash block
    fi

    if [ "$aeskey" != "" ]; then
        dd if=$aeskey of=$patt bs=1 seek=64 count=32 \
            conv=notrunc >& /dev/null
        #TODO lock aes key block
    fi

    if [ "$passwordhash" != "" ]; then
        dd if=$passwordhash of=$patt bs=1 seek=96 count=32 \
            conv=notrunc >& /dev/null
        #TODO lock jtag password hash block
    fi

    if [ "$scanpasswordhash" != "" ]; then
        dd if=$scanpasswordhash of=$patt bs=1 seek=128 count=32 \
            conv=notrunc >& /dev/null
        #Lock scan password hash block
        b_b1="$(printf %02x $(( 0x$b_b1 | 0x03 )))"
    fi

    if [ "$userefusefile" != "" ]; then
        dd if=$userefusefile of=$patt bs=1 seek=240 count=16 \
            conv=notrunc >& /dev/null
    fi

    # Write kwrap (in user data area) last
    if [ "$kwrap" != "" ]; then
        dd if=$kwrap of=$patt bs=1 seek=240 count=16 \
            conv=notrunc >& /dev/null

        # Lock kwrap block
        if $wrlock_kwrap; then
            b_b1="$(printf %02x $(( 0x$b_b1 | 0x80 )))"
        fi
    fi

    # Create and write license (including write-lock)
    if [ ${#b5} -ne 2 ] || [ ${#b6} -ne 2 ] || [ ${#b7} -ne 2 ] || [ ${#b_b1} -ne 2 ]; then
        echo Internal Error
        exit 1
    fi

    echo 00 $b_b1 00 00 00 $b5 $b6 $b7 00 00 00 00 00 00 00 00 | xxd -r -p > $license
    filesize=$(wc -c < $license)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid license pattern length
        exit 1
    fi

    dd if=$license of=$patt bs=1 seek=176 count=16 \
        conv=notrunc >& /dev/null

    if [ "$opt_raw_otp_pattern" == "true" ]; then
        cp $patt $output
    else
        ${TOOL_PATH}/aml_encrypt_axg --efsproc --input $patt --output $output
    fi

    echo "done"
}

generate_efuse_pattern_g12a() {
    echo -n "Generate OTP pattern for G12A (S905X2, S905D2) ... "

    # Additional args check
    if [ "$userefusefile" != "" ]; then
        check_file userefusefile "$userefusefile"
        local filesize=$(wc -c < ${userefusefile})
        if [ $filesize -ne 96 ]; then
            echo "User efuse file incorrect size $filesize != 96"
            exit 1
        fi
    fi

    # Create input pattern

    # OTP write lock control
    b_b0="00"
    b_b1="00"
    b_b2="00"
    b_b3="00"

    # Construct license bits
    b_a3="00"
    if [ "$disablejtag" == "true" ]; then
        # Disable *ALL* JTAG (M3, M4, AP) in G12A
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x80 )))"
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x40 )))"
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x20 )))"
    fi

    b_a2="00"
    if [ "$disableprint" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x40 )))"
    fi
    if [ "$disablebootusb" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x20 )))"
    fi
    if [ "$disablebootspi" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x10 )))"
    fi
    if [ "$disablebootsdcard" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x08 )))"
    fi
    if [ "$disablebootnandemmc" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x04 )))"
    fi
    if [ "$disablebootrecover" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x02 )))"
    fi
    if [ "$disablescanchain" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x01 )))"
    fi

    b_a1="00"
    if [ "$enableusbpassword" == "true" ]; then
        b_a1="$(printf %02x $(( 0x$b_a1 | 0x02 )))"
    fi
    if [ "$enablejtagpassword" == "true" ]; then
        b_a1="$(printf %02x $(( 0x$b_a1 | 0x01 )))"
    fi

    b_a0="00"
    if [ "$enablescanpassword" == "true" ]; then
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x80 )))"
    fi
    if [ "$enableantirollback" == "true" ]; then
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x40 )))"
    fi
    if [ "$enablesb" == "true" ]; then
        # Enable both M3 and AP secure boot
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x01 )))"
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x04 )))"
        # Enable M4 secure boot:
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x10 )))"
    fi
    if [ "$enableaes" == "true" ]; then
        # Enable both M3 and AP encryption
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x02 )))"
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x08 )))"
        # Enable M4 encryption:
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x20 )))"
    fi

    b_196="00"
    if $revokersk0; then
        b_196="$(printf %02x $(( 0x$b_196 | 0x10 )))"
    fi
    if $revokersk1; then
        b_196="$(printf %02x $(( 0x$b_196 | 0x08 )))"
    fi
    if $revokersk2; then
        b_196="$(printf %02x $(( 0x$b_196 | 0x04 )))"
    fi
    if $revokersk3; then
        b_196="$(printf %02x $(( 0x$b_196 | 0x02 )))"
    fi

    # Generate empty eFUSE pattern data
    dd if=/dev/zero of=$patt count=512 bs=1 &> /dev/null

    # Create and write license block
    if [ ${#b_a0} -ne 2 ] || [ ${#b_a1} -ne 2 ] || [ ${#b_a2} -ne 2 ] || [ ${#b_a3} -ne 2 ] || [ ${#b_196} -ne 2 ]; then
        echo Internal Error
        exit 1
    fi

    echo $b_a0 $b_a1 $b_a2 $b_a3 00 00 00 00 00 00 00 00 00 00 00 00 | xxd -r -p > $license
    echo 00 00 00 00 00 00 $b_196 00 00 00 00 00 00 00 00 00 | xxd -r -p > $license2

    filesize=$(wc -c < $license)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid license pattern length
        exit 1
    fi

    filesize=$(wc -c < $license2)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid license pattern length
        exit 1
    fi

    dd if=$license of=$patt bs=16 seek=10 count=1 \
        conv=notrunc >& /dev/null

    dd if=$license2 of=$patt bs=16 seek=25 count=1 \
        conv=notrunc >& /dev/null

    if [ "$roothash" != "" ]; then
        dd if=$roothash of=$patt bs=16 seek=20 count=2 \
            conv=notrunc >& /dev/null

        # Lock root hash blocks
        b_b2="$(printf %02x $(( 0x$b_b2 | 0x30 )))"
    fi

    if [ "$aeskey" != "" ]; then
        dd if=$aeskey of=$patt bs=16 seek=2 count=2 \
            conv=notrunc >& /dev/null

        # Lock aes key blocks
        b_b0="$(printf %02x $(( 0x$b_b0 | 0x0c )))"
    fi

    if [ "$m4roothash" != "" ]; then
        dd if=$m4roothash of=$patt bs=16 seek=22 count=2 \
            conv=notrunc >& /dev/null

        # Lock m4 root hash blocks
        b_b2="$(printf %02x $(( 0x$b_b2 | 0xc0 )))"
    fi

    if [ "$m4aeskey" != "" ]; then
        dd if=$m4aeskey of=$patt bs=16 seek=5 count=1 \
            conv=notrunc >& /dev/null

        # Lock m4 aes key block
        b_b0="$(printf %02x $(( 0x$b_b0 | 0x20 )))"
    fi

    if [ "$passwordhash" != "" ]; then
        dd if=$passwordhash of=$patt bs=16 seek=6 count=2 \
            conv=notrunc >& /dev/null

        # Lock jtag password hash blocks
        b_b0="$(printf %02x $(( 0x$b_b0 | 0xc0 )))"
    fi

    if [ "$scanpasswordhash" != "" ]; then
        dd if=$scanpasswordhash of=$patt bs=16 seek=8 count=2 \
            conv=notrunc >& /dev/null

        # Lock scan password hash blocks
        b_b1="$(printf %02x $(( 0x$b_b1 | 0x03 )))"
    fi

    if [ "$userefusefile" != "" ]; then
        dd if=$userefusefile of=$patt bs=16 seek=26 count=6 \
            conv=notrunc >& /dev/null
    fi

    # Write kwrap (in user data area) last
    if [ "$kwrap" != "" ]; then
        dd if=$kwrap of=$patt bs=16 seek=31 count=1 \
            conv=notrunc >& /dev/null

        if $wrlock_kwrap; then
            b_b3="$(printf %02x $(( 0x$b_b3 | 0x80 )))"
        fi
    fi

    # Create and write write-lock
    if [ ${#b_b0} -ne 2 ] || [ ${#b_b1} -ne 2 ] || [ ${#b_b2} -ne 2 ] || [ ${#b_b3} -ne 2 ]; then
        echo Internal Error
        exit 1
    fi

    echo $b_b0 $b_b1 $b_b2 $b_b3 00 00 00 00 00 00 00 00 00 00 00 00 | xxd -r -p > $wrlock
    filesize=$(wc -c < $wrlock)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid write-lock pattern length
        exit 1
    fi
    dd if=$wrlock of=$patt bs=16 seek=11 count=1 conv=notrunc >& /dev/null

    if [ "$opt_raw_otp_pattern" == "true" ]; then
        cp $patt $output
    else
        ${TOOL_PATH}/aml_encrypt_g12a --efsgen3 --input $patt --output $output
    fi

    echo "done"
}

generate_efuse_pattern_sm1() {
    echo -n "Generate OTP pattern for SM1 (S905X3/Y3/D3) ... "

    # Additional args check
    if [ "$userefusefile" != "" ]; then
        check_file userefusefile "$userefusefile"
        local filesize=$(wc -c < ${userefusefile})
        if [ $filesize -ne 96 ]; then
            echo "User efuse file incorrect size $filesize != 96"
            exit 1
        fi
    fi

    # Create input pattern

    # OTP write lock control
    b_b0="00"
    b_b1="00"
    b_b2="00"
    b_b3="00"

    # Construct license bits
    b_a0="00"
    b_a1="00"
    b_a2="00"
    b_a3="00"
    if [ "$disablejtag" == "true" ]; then
        # Disable *ALL* JTAG (M3, M4, AP) in SM1
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x80 )))"
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x40 )))"
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x20 )))"
    fi

    if [ "$disableprint" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x40 )))"
    fi
    if [ "$disablebootusb" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x20 )))"
    fi
    if [ "$disablebootspi" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x10 )))"
    fi
    if [ "$disablebootsdcard" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x08 )))"
    fi
    if [ "$disablebootnandemmc" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x04 )))"
    fi
    if [ "$disablebootrecover" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x02 )))"
    fi
    if [ "$disablescanchain" == "true" ]; then
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x01 )))"
    fi

    if [ "$enableusbpassword" == "true" ]; then
        b_a1="$(printf %02x $(( 0x$b_a1 | 0x02 )))"
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x04 )))"
    fi
    if [ "$enablejtagpassword" == "true" ]; then
        b_a1="$(printf %02x $(( 0x$b_a1 | 0x01 )))"
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x02 )))"
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x08 )))"
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x10 )))"
    fi

    if [ "$enablescanpassword" == "true" ]; then
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x80 )))"
        b_a3="$(printf %02x $(( 0x$b_a3 | 0x01 )))"
    fi
    if [ "$enableantirollback" == "true" ]; then
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x40 )))"
    fi
    if [ "$enablesb" == "true" ]; then
        # Enable SCP, AP, SP secure boot
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x01 )))"
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x04 )))"
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x10 )))"
        b_a1="$(printf %02x $(( 0x$b_a1 | 0x08 )))"
        b_a1="$(printf %02x $(( 0x$b_a1 | 0x20 )))"
        b_a1="$(printf %02x $(( 0x$b_a1 | 0x80 )))"
    fi
    if [ "$enableaes" == "true" ]; then
        # Enable SCP, AP, SP encryption
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x02 )))"
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x08 )))"
        b_a0="$(printf %02x $(( 0x$b_a0 | 0x20 )))"
        b_a1="$(printf %02x $(( 0x$b_a1 | 0x10 )))"
        b_a1="$(printf %02x $(( 0x$b_a1 | 0x40 )))"
        b_a2="$(printf %02x $(( 0x$b_a2 | 0x80 )))"
    fi

    b_196="00"
    if $revokersk0; then
        b_196="$(printf %02x $(( 0x$b_196 | 0x10 )))"
    fi
    if $revokersk1; then
        b_196="$(printf %02x $(( 0x$b_196 | 0x08 )))"
    fi
    if $revokersk2; then
        b_196="$(printf %02x $(( 0x$b_196 | 0x04 )))"
    fi
    if $revokersk3; then
        b_196="$(printf %02x $(( 0x$b_196 | 0x02 )))"
    fi

    # Generate empty eFUSE pattern data
    dd if=/dev/zero of=$patt count=512 bs=1 &> /dev/null

    # Create and write license block
    if [ ${#b_a0} -ne 2 ] || [ ${#b_a1} -ne 2 ] || [ ${#b_a2} -ne 2 ] || [ ${#b_a3} -ne 2 ] || [ ${#b_196} -ne 2 ]; then
        echo Internal Error
        exit 1
    fi

    echo $b_a0 $b_a1 $b_a2 $b_a3 00 00 00 00 00 00 00 00 00 00 00 00 | xxd -r -p > $license
    echo 00 00 00 00 00 00 $b_196 00 00 00 00 00 00 00 00 00 | xxd -r -p > $license2

    filesize=$(wc -c < $license)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid license pattern length
        exit 1
    fi

    filesize=$(wc -c < $license2)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid license pattern length
        exit 1
    fi

    dd if=$license of=$patt bs=16 seek=10 count=1 \
        conv=notrunc >& /dev/null

    dd if=$license2 of=$patt bs=16 seek=25 count=1 \
        conv=notrunc >& /dev/null

    if [ "$roothash" != "" ]; then
        dd if=$roothash of=$patt bs=16 seek=20 count=2 \
            conv=notrunc >& /dev/null

        # Lock root hash blocks
        b_b2="$(printf %02x $(( 0x$b_b2 | 0x30 )))"
    fi

    if [ "$aeskey" != "" ]; then
        dd if=$aeskey of=$patt bs=16 seek=2 count=2 \
            conv=notrunc >& /dev/null

        # Lock aes key blocks
        b_b0="$(printf %02x $(( 0x$b_b0 | 0x0c )))"
    fi

    if [ "$m4roothash" != "" ]; then
        dd if=$m4roothash of=$patt bs=16 seek=22 count=2 \
            conv=notrunc >& /dev/null

        # Lock m4 root hash blocks
        b_b2="$(printf %02x $(( 0x$b_b2 | 0xc0 )))"
    fi

    if [ "$m4aeskey" != "" ]; then
        dd if=$m4aeskey of=$patt bs=16 seek=5 count=1 \
            conv=notrunc >& /dev/null

        # Lock m4 aes key block
        b_b0="$(printf %02x $(( 0x$b_b0 | 0x20 )))"
    fi

    if [ "$passwordhash" != "" ]; then
        dd if=$passwordhash of=$patt bs=16 seek=6 count=2 \
            conv=notrunc >& /dev/null

        # Lock jtag password hash blocks
        b_b0="$(printf %02x $(( 0x$b_b0 | 0xc0 )))"
    fi

    if [ "$scanpasswordhash" != "" ]; then
        dd if=$scanpasswordhash of=$patt bs=16 seek=8 count=2 \
            conv=notrunc >& /dev/null

        # Lock scan password hash blocks
        b_b1="$(printf %02x $(( 0x$b_b1 | 0x03 )))"
    fi

    if [ "$userefusefile" != "" ]; then
        dd if=$userefusefile of=$patt bs=16 seek=26 count=6 \
            conv=notrunc >& /dev/null
    fi

    # Write kwrap (in user data area) last
    if [ "$kwrap" != "" ]; then
        dd if=$kwrap of=$patt bs=16 seek=31 count=1 \
            conv=notrunc >& /dev/null

        if $wrlock_kwrap; then
            b_b3="$(printf %02x $(( 0x$b_b3 | 0x80 )))"
        fi
    fi

    # Create and write write-lock
    if [ ${#b_b0} -ne 2 ] || [ ${#b_b1} -ne 2 ] || [ ${#b_b2} -ne 2 ] || [ ${#b_b3} -ne 2 ]; then
        echo Internal Error
        exit 1
    fi

    echo $b_b0 $b_b1 $b_b2 $b_b3 00 00 00 00 00 00 00 00 00 00 00 00 | xxd -r -p > $wrlock
    filesize=$(wc -c < $wrlock)
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid write-lock pattern length
        exit 1
    fi
    dd if=$wrlock of=$patt bs=16 seek=11 count=1 conv=notrunc >& /dev/null

    if [ "$opt_raw_otp_pattern" == "true" ]; then
        cp $patt $output
    else
        ${TOOL_PATH}/aml_encrypt_g12a --efsgen3 --input $patt --output $output
    fi

    echo "done"
}

generate_efuse_pattern_a1() {
    echo -n "Generate OTP pattern for A1/C1 ... "

    # Additional args check
    if [ -n "$userefusefile" ] && [ -n "$kwrap" ]; then
        echo "Error: Only one of user-efuse-file and kwrap options allowed"
        exit 1
    fi
    local user_gp_0=""
    if [ "$userefusefile" != "" ]; then
        user_gp_0="$userefusefile"
    fi
    if [ "$kwrap" != "" ]; then
        user_gp_0="$kwrap"
    fi
    check_opt_file user-efuse-file_or_kwrap 16 "$user_gp_0"
    if $disablebootspi; then
        echo "Error: A1 split disable-boot-spi to disable-boot-spi-nand and disable-boot-spi-nor"
        exit 1
    fi
    if $enablejtagpassword || $enableantirollback; then
        echo "Error: A1 unsupported flag enable-jtag-password=$enablejtagpassword || enable-anti-rollback=$enableantirollback"
        exit 1
    fi
    if [ -n "$m4roothash" ] || [ -n "$m4aeskey" ] || [ -n  "$passwordhash" ] ||
            [ -n "$scanpasswordhash" ]; then
        echo "Error: A1 unsupported option $m4roothash $m4aeskey $passwordhash $scanpasswordhash"
        exit 1
    fi
    check_opt_file block1 16 "$block1"
    check_opt_file acgk 16 "$acgk"
    check_opt_file acrk 16 "$acrk"
    check_opt_file dvgk 16 "$dvgk"
    check_opt_file dvuk 16 "$dvuk"

    # Create input pattern

    # Construct license bits
    b_60="00"
    b_61="00"
    b_62="00"
    b_67="00"

    if [ "$disablejtag" == "true" ]; then
        b_67="$(printf %02x $(( 0x$b_67 | 0x10 )))"
    fi
    if [ "$disableprint" == "true" ]; then
        b_62="$(printf %02x $(( 0x$b_62 | 0x40 )))"
    fi

    if [ "$soc" == "c1" ]; then
        if [ "$disablebootsdcard" == "true" ]; then
            b_62="$(printf %02x $(( 0x$b_62 | 0x01 )))"
        fi
        if [ "$disablebootemmc" == "true" ]; then
            b_62="$(printf %02x $(( 0x$b_62 | 0x02 )))"
        fi
        if [ "$disablebootnand" == "true" ]; then
            b_62="$(printf %02x $(( 0x$b_62 | 0x04 )))"
        fi
        if [ "$disablebootnandemmc" == "true" ]; then
            b_62="$(printf %02x $(( 0x$b_62 | 0x02 )))"
            b_62="$(printf %02x $(( 0x$b_62 | 0x04 )))"
        fi
    fi

    if [ "$disablebootusb" == "true" ]; then
        b_62="$(printf %02x $(( 0x$b_62 | 0x20 )))"
    fi
    if [ "$disablebootspinand" == "true" ]; then
        b_62="$(printf %02x $(( 0x$b_62 | 0x08 )))"
    fi
    if [ "$disablebootspinor" == "true" ]; then
        b_62="$(printf %02x $(( 0x$b_62 | 0x10 )))"
    fi

    if [ "$disablescanchain" == "true" ]; then
        b_67="$(printf %02x $(( 0x$b_67 | 0x40 )))"
    fi

    if [ "$enableusbpassword" == "true" ]; then
        b_60="$(printf %02x $(( 0x$b_60 | 0x08 )))"
    fi

    if [ "$enablescanpassword" == "true" ]; then
        b_67="$(printf %02x $(( 0x$b_67 | 0x20 )))"
    fi
    if [ "$enablesb" == "true" ]; then
        # Enable ACPU secure boot
        b_60="$(printf %02x $(( 0x$b_60 | 0x01 )))"
        b_61="$(printf %02x $(( 0x$b_61 | 0x01 )))"
    fi
    if [ "$enableaes" == "true" ]; then
        # Enable ACPU encryption
        b_60="$(printf %02x $(( 0x$b_60 | 0x02 )))"
        b_61="$(printf %02x $(( 0x$b_61 | 0x02 )))"
    fi

    if $revokersk0; then
        b_60="$(printf %02x $(( 0x$b_60 | 0x10 )))"
    fi
    if $revokersk1; then
        b_60="$(printf %02x $(( 0x$b_60 | 0x20 )))"
    fi
    if $revokersk2; then
        b_60="$(printf %02x $(( 0x$b_60 | 0x40 )))"
    fi
    if $revokersk3; then
        b_60="$(printf %02x $(( 0x$b_60 | 0x80 )))"
    fi

    # Generate empty eFUSE pattern data
    dd if=/dev/zero of="$patt" count=256 bs=1 &> /dev/null

    # Create and write license block
    if [ ${#b_60} -ne 2 ] || [ ${#b_61} -ne 2 ] || [ ${#b_62} -ne 2 ] || [ ${#b_67} -ne 2 ]; then
        echo Internal Error license $b_60 $b_61 $b_62 $b_67
        exit 1
    fi

    echo $b_60 $b_61 $b_62 00 00 00 00 $b_67 00 00 00 00 00 00 00 00 | xxd -r -p > "$license"

    filesize=$(wc -c < "$license")
    if [ $filesize -ne 16 ]; then
        echo Internal Error -- Invalid license pattern length
        exit 1
    fi

    dd if="$license" of="$patt" bs=16 seek=6 count=1 \
        conv=notrunc >& /dev/null

    if [ "$roothash" != "" ]; then
        dd if="$roothash" of="$patt" bs=16 seek=2 count=2 \
            conv=notrunc >& /dev/null
    fi

    if [ "$block1" != "" ]; then
        dd if="$block1" of="$patt" bs=16 seek=1 count=1 \
            conv=notrunc >& /dev/null
    fi

    if [ "$acgk" != "" ]; then
        dd if="$acgk" of="$patt" bs=16 seek=10 count=1 \
            conv=notrunc >& /dev/null
    fi

    if [ "$acrk" != "" ]; then
        dd if="$acrk" of="$patt" bs=16 seek=5 count=1 \
            conv=notrunc >& /dev/null
    fi

    if [ "$dvgk" != "" ]; then
        dd if="$dvgk" of="$patt" bs=16 seek=4 count=1 \
            conv=notrunc >& /dev/null
    fi

    if [ "$dvuk" != "" ]; then
        dd if="$dvuk" of="$patt" bs=16 seek=11 count=1 \
            conv=notrunc >& /dev/null
    fi

    # Write user data
    if [ "$user_gp_0" != "" ]; then
        dd if="$user_gp_0" of="$patt" bs=16 seek=15 count=1 \
            conv=notrunc >& /dev/null
    fi

    if [ "$opt_raw_otp_pattern" == "true" ]; then
        cp "$patt" "$output"
    else
        # Use g12a tool, but pad pattern to 512 bytes, just in case.
        check_size "$patt" 256
        head -c 256 /dev/zero >> "$patt"
        check_size "$patt" 512
        "${TOOL_PATH}"/aml_encrypt_c1 --efsgen3 --input "$patt" --output "$output"
    fi

    echo "done"
}

parse_main() {
    case "$@" in
        --help)
            usage
            ;;
        --version)
            echo "$(basename $0) version $VERSION"
            ;;
        *-o*)
            generate_efuse_pattern "$@"
            ;;
        *)
            usage "$@"
            ;;
    esac
}

parse_main "$@"
