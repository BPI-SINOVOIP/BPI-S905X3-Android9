#!/bin/bash -e

# Copyright (c) 2018 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.

#set -x

SCRIPT_PATH=${SCRIPT_PATH:-$(dirname $(readlink -f $0))}

# Path to sign-boot-c1 util
TOOL_PATH=${TOOL_PATH:-$(dirname $(readlink -f $0))/../signing-tool-c1/}

#source ${TOOL_PATH}/build.sh

# Temporary files directory
if [ -z "$TMP" ]; then
    TMP=${SCRIPT_PATH}/tmp
fi

trace ()
{
    echo ">>> $@" > /dev/null
}

usage() {
    cat << EOF
Usage: $(basename $0) --help

       Combine binaries into an unsigned bootloader:

       $(basename $0) --create-unsigned-bl \\
                      [--soc      (g12a|g12b|sm1|a1|c1)] \\
                      --bl2 <bl2_new.bin> \\
                      --bl30 <bl30_new.bin> \\
                      --bl31 <bl31.img> \\
                      [--bl32 <bl32.img>] \\
                      --bl33 <bl33.bin> \\
                      -o <bl.bin>

       Create a root hash for OTP:

       $(basename $0) --create-root-hash \\
                      --root-key-0 <work/root.pem> \\
                      --root-key-1 <work/root.pem> \\
                      --root-key-2 <work/root.pem> \\
                      --root-key-3 <work/root.pem> \\
                      -o work/rootkeys-hash.bin

       Create a root hash for OTP from public key only:

       $(basename $0) --create-root-hash-from-pub \\
                      --root-key-0 work/root.pub \\
                      --root-key-1 work/root.pub \\
                      --root-key-2 work/root.pub \\
                      --root-key-3 work/root.pub \\
                      -o work/rootkeys-hash.bin

      Environment Variables:

      TMP:          path to a temporary directory. Defaults to <this script's path>/tmp
EOF
    exit 1
}

## Global SoC Config
current_soc=""
bl30_required=true
keyhashver_min=2

set_soc() {
    if [ "$current_soc" != "" ]; then
        if [ "$1" != "$current_soc" ]; then
            echo "Internal error setting soc $1"; exit 1
        fi
        return
    fi
    case "$1" in
        g12a|g12b|sm1) ;;
        a1|c1)
            bl30_required=false
            keyhashver_min=3
            ;;
        *)
            echo "Invalid soc $1"; exit 1 ;;
    esac
    readonly bl30_required keyhashver_min
    readonly current_soc="$1"
}
## /Global SoC Config

check_file() {
    if [ ! -f "$2" ]; then echo Error: Unable to open $1: \""$2"\"; exit 1 ; fi
}

# Hash root/bl2 RSA keys'.
#
#   v1 only sha256(n[keylen] + e)
#   v2 sha256 the entire mincrypt keyblob including length
#
# $1: key hash version
# $2: precomputed binary key file
# $3: keylen
# $4: output hash file
hash_rsa_bin() {
    local keyhashver=$1
    local key=$2
    local keylen=$3
    local output=$4
    if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ] || [ -z "$4" ]; then
        echo "Argument error"
        exit 1
    fi
    local insize=$(wc -c < $key)
    if [ $insize -ne 1052 ] && [ $insize -ne 1036 ]; then
        echo "Keyfile wrong size"
        exit 1
    fi
    if [ "$keyhashver" == "3" ] && [ $insize -ne 1052 ]; then
        echo "Keyfile wrong size"
        exit 1
    fi
    if [ "$keyhashver" == "2" ] && [ $insize -ne 1036 ]; then
        echo "Keyfile wrong size"
        exit 1
    fi
    if [ $keylen -ne 1024 ] && [ $keylen -ne 2048 ] && [ $keylen -ne 4096 ]; then
        echo "Invalid keylen"
        exit 1
    fi
    local keylenbytes=$(( $keylen / 8 ))

    if [ "$keyhashver" == "3" ] || [ $keyhashver -eq 2 ]; then
        cp $key $TMP/keydata
    else
        # modulus - rsa_public_key_t.n[key length]
        dd if=$key of=$TMP/keydata bs=$keylenbytes count=1 >& /dev/null
        # exponent - rsa_public_key_t.e
        dd if=$key of=$TMP/keydata bs=1 skip=512 count=4 \
            oflag=append conv=notrunc >& /dev/null
    fi

    openssl dgst -sha256 -binary $TMP/keydata > $output
}

# --pack-bl2  -i bl2.bin  -o bl2.bin.img
pack_bl2() {
    local input=""
    local output=""
    local size=""
    local argv=("$@")
    local i=0

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        case "$arg" in
            -i)
                i=$((i + 1))
                input="${argv[$i]}"
                ;;
            -o)
                i=$((i + 1))
                output="${argv[$i]}"
                ;;
            -s)
                i=$((i + 1))
                size="${argv[$i]}"
                ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    check_file bl2  "$input"
    if [ -z "$output" ]; then echo Error: Missing output file option -o; exit 1; fi

    # Add header
    ${TOOL_PATH}/sign-boot-c1 --add-aml-block-header \
            -i $input \
            -s $size \
            -o $TMP/bl2.img-noiv

    # Add nonce
    dd if=/dev/urandom of=$TMP/nonce.bin bs=16 count=1 >& /dev/null
    cat $TMP/nonce.bin $TMP/bl2.img-noiv > $TMP/bl2.img

    # Truncate to correct length
    # TODO should check that end of output is all zeroes before truncating
    truncate -s $size $TMP/bl2.img

    # Add sha256 hash into header at byte offset 80
    dd if=$TMP/bl2.img of=$TMP/chkdata bs=1 skip=16 count=64 >& /dev/null
    dd if=$TMP/bl2.img of=$TMP/chkdata bs=1 skip=112 \
        oflag=append conv=notrunc  >& /dev/null
    openssl dgst -sha256 -binary $TMP/chkdata > $TMP/bl2.sha

    dd if=$TMP/bl2.sha of=$TMP/bl2.img bs=1 seek=80 \
        conv=notrunc  >& /dev/null

    cp $TMP/bl2.img $output
}

# Convert RSA public PEM key [from pkcs#8] to pkcs#1
# $1: input RSA private .PEM
# $2: output public key file
pub_to_pkcs1() {
    local input=$1
    local output=$2
    if [ ! -f "$1" ] || [ -z "$2" ]; then
        echo "Argument error, \"$1\", \"$2\" "
        exit 1
    fi

    openssl 2>/dev/null rsa -in $input -pubin -pubout -RSAPublicKey_out -out $output

    if [ $? -ne 0 ]; then
        echo "Error converting key"
        exit 1
    fi
}

create_root_hash() {
    local rootkey0=""
    local rootkey1=""
    local rootkey2=""
    local rootkey3=""
    local output=""
    local sigver=""
    local keyhashver=""
    local soc="g12a"
    local argv=("$@")
    local i=0

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --soc)
                soc="${argv[$i]}" ;;
            --root-key-0)
                rootkey0="${argv[$i]}" ;;
            --root-key-1)
                rootkey1="${argv[$i]}" ;;
            --root-key-2)
                rootkey2="${argv[$i]}" ;;
            --root-key-3)
                rootkey3="${argv[$i]}" ;;
            --sig-ver)
                sigver="${argv[$i]}" ;;
            --key-hash-ver)
                keyhashver="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    set_soc "$soc"
    check_file rootkey0 "$rootkey0"
    check_file rootkey1 "$rootkey1"
    check_file rootkey2 "$rootkey2"
    check_file rootkey3 "$rootkey3"
    if [ -z "$output" ]; then echo Error: Missing output file option -o; exit 1; fi
    if [ -z "$sigver" ]; then
        sigver=1
    fi
    if [ -z "$keyhashver" ]; then
        keyhashver=$keyhashver_min
    fi
    if [ "$keyhashver" != "2" ] && [ "$keyhashver" != "3" ]; then
        echo Error: bad key hash ver; exit 1
    fi
    if [ "$keyhashver" -lt $keyhashver_min ]; then
        echo Error: bad key hash ver; exit 1
    fi

    echo "Creating root hash using v"$keyhashver" key hash version"
    local keylenbytes=$(get_pem_key_len $rootkey0)
    local keylen=$(( $keylenbytes * 8 ))

    # Convert PEM key to rsa_public_key_t (precomputed RSA public key)
    if [ "$keyhashver" == "3" ]; then
        enable_n0inv128=True
    else
        enable_n0inv128=False
    fi

    pem_to_bin $rootkey0 $TMP/rootkey0.bin $enable_n0inv128
    pem_to_bin $rootkey1 $TMP/rootkey1.bin $enable_n0inv128
    pem_to_bin $rootkey2 $TMP/rootkey2.bin $enable_n0inv128
    pem_to_bin $rootkey3 $TMP/rootkey3.bin $enable_n0inv128

    # hash of keys
    hash_rsa_bin $keyhashver $TMP/rootkey0.bin $keylen $TMP/rootkey0.sha
    hash_rsa_bin $keyhashver $TMP/rootkey1.bin $keylen $TMP/rootkey1.sha
    hash_rsa_bin $keyhashver $TMP/rootkey2.bin $keylen $TMP/rootkey2.sha
    hash_rsa_bin $keyhashver $TMP/rootkey3.bin $keylen $TMP/rootkey3.sha

    cat $TMP/rootkey0.sha $TMP/rootkey1.sha $TMP/rootkey2.sha $TMP/rootkey3.sha > $TMP/rootkeys.sha
    openssl dgst -sha256 -binary $TMP/rootkeys.sha > $output
}

create_root_hash_from_pub() {
    local rootkey0=""
    local rootkey1=""
    local rootkey2=""
    local rootkey3=""
    local output=""
    local sigver=""
    local keyhashver=""
    local soc="g12a"
    local argv=("$@")
    local i=0

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --soc)
                soc="${argv[$i]}" ;;
            --root-key-0)
                rootkey0="${argv[$i]}" ;;
            --root-key-1)
                rootkey1="${argv[$i]}" ;;
            --root-key-2)
                rootkey2="${argv[$i]}" ;;
            --root-key-3)
                rootkey3="${argv[$i]}" ;;
            --sig-ver)
                sigver="${argv[$i]}" ;;
            --key-hash-ver)
                keyhashver="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    set_soc "$soc"
    check_file rootkey0 "$rootkey0"
    check_file rootkey1 "$rootkey1"
    check_file rootkey2 "$rootkey2"
    check_file rootkey3 "$rootkey3"
    if [ -z "$output" ]; then echo Error: Missing output file option -o; exit 1; fi
    if [ -z "$sigver" ]; then
        sigver=1
    fi
    if [ -z "$keyhashver" ]; then
        keyhashver=$keyhashver_min
    fi
    if [ "$keyhashver" != "2" ] && [ "$keyhashver" != "3" ]; then
        echo Error: bad key hash ver; exit 1
    fi
    if [ "$keyhashver" -lt $keyhashver_min ]; then
        echo Error: bad key hash ver; exit 1
    fi

    local keylenbytes=$(get_pub_pem_key_len $rootkey0)
    local keylen=$(( $keylenbytes * 8 ))

    pub_to_pkcs1 $rootkey0 $TMP/rootkey0.pub
    pub_to_pkcs1 $rootkey1 $TMP/rootkey1.pub
    pub_to_pkcs1 $rootkey2 $TMP/rootkey2.pub
    pub_to_pkcs1 $rootkey3 $TMP/rootkey3.pub

    # Convert PEM key to rsa_public_key_t (precomputed RSA public key)
    if [ "$keyhashver" == "3" ]; then
        enable_n0inv128=True
    else
        enable_n0inv128=False
    fi
    pem_to_bin $TMP/rootkey0.pub $TMP/rootkey0.bin $enable_n0inv128
    pem_to_bin $TMP/rootkey1.pub $TMP/rootkey1.bin $enable_n0inv128
    pem_to_bin $TMP/rootkey2.pub $TMP/rootkey2.bin $enable_n0inv128
    pem_to_bin $TMP/rootkey3.pub $TMP/rootkey3.bin $enable_n0inv128

    # hash of keys
    hash_rsa_bin $keyhashver $TMP/rootkey0.bin $keylen $TMP/rootkey0.sha
    hash_rsa_bin $keyhashver $TMP/rootkey1.bin $keylen $TMP/rootkey1.sha
    hash_rsa_bin $keyhashver $TMP/rootkey2.bin $keylen $TMP/rootkey2.sha
    hash_rsa_bin $keyhashver $TMP/rootkey3.bin $keylen $TMP/rootkey3.sha

    cat $TMP/rootkey0.sha $TMP/rootkey1.sha $TMP/rootkey2.sha $TMP/rootkey3.sha > $TMP/rootkeys.sha
    openssl dgst -sha256 -binary $TMP/rootkeys.sha > $output
}

# Get key len in bytes of public PEM RSA key
# $1: PEM file
get_pub_pem_key_len() {
    local pem=$1
    local bits=0
    if [ ! -f "$1" ]; then
        echo "Argument error, \"$1\""
        exit 1
    fi
    bits=$(openssl rsa -in $pem -pubin -text -noout | \
        grep '^Public-Key: (' | \
        sed 's/Public-Key: (//' | \
        sed 's/ bit)//')
    if [ "$bits" -ne 1024 ] && [ "$bits" -ne 2048 ] &&
       [ "$bits" -ne 4096 ] && [ "$bits" -ne 8192]; then
       echo "Unexpected key size  $bits"
       exit 1
    fi
    echo $(( $bits / 8 ))
}

# Get key len in bytes of private PEM RSA key
# $1: PEM file
get_pem_key_len() {
    local pem=$1
    local bits=0
    if [ ! -f "$1" ]; then
        echo "Argument error, \"$1\""
        exit 1
    fi
    bits=$(openssl rsa -in $pem -text -noout | \
        grep 'Private-Key: (' | \
        sed -r 's/(RSA )?Private-Key: \(//'| \
        sed -r 's/ bit(,.?[0-9].?primes)?\)//')
    if [ "$bits" -ne 1024 ] && [ "$bits" -ne 2048 ] &&
       [ "$bits" -ne 4096 ] && [ "$bits" -ne 8192]; then
       echo "Unexpected key size  $bits"
       exit 1
    fi
    echo $(( $bits / 8 ))
}

# Calculate aligned file size
# $1: file
# $2: alignment requirement in bytes
aligned_size() {
    local file=$1
    local skip=$2
    local alignment=$3
    local alignedsize=0

    local filesize=$(wc -c < ${file})
    #echo "Input $file filesize $filesize"
    if [ $skip -ne 0 ]; then
        filesize=$(( $filesize - $skip ))
    fi
    local rem=$(( $filesize % $alignment ))
    if [ $rem -ne 0 ]; then
        #echo "Input $file not $alignment byte aligned"
        local padsize=$(( $alignment - $rem ))
        alignedsize=$(( $filesize + $padsize ))
    else
        alignedsize=$filesize
    fi
    #echo "Aligned size $alignedsize"
    echo $alignedsize
}

# Pad file to len by adding 0's to end of file
# $1: file
# $2: len
pad_file() {
    local file=$1
    local len=$2
    if [ ! -f "$1" ] || [ -z "$2" ]; then
        echo "Argument error, \"$1\", \"$2\" "
        exit 1
    fi
    local filesize=$(wc -c < ${file})
    local padlen=$(( $len - $filesize ))
    if [ $len -lt $filesize ]; then
        echo "File larger than expected.  $filesize, $len"
        exit 1
    fi
    dd if=/dev/zero of=$file oflag=append conv=notrunc bs=1 \
        count=$padlen >& /dev/null
}

# add header to bl3x
pack_bl3x() {
    local input=""
    local output=""
    local arb_cvn=""
    local argv=("$@")
    local i=0

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            -i)
                input="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            -v)
                arb_cvn="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    check_file "input" "$input"
    if [ -z "$output" ]; then echo Error: Missing output file option -o; exit 1; fi
    if [ -z "$arb_cvn" ]; then
        arb_cvn="0"
    fi

    # Check padding
    local imagesize=$(wc -c < ${input})
    local rem=$(( $imagesize % 16 ))
    if [ $rem -ne 0 ]; then
        #echo "Input $input not 16 byte aligned?"
        local topad=$(( 16 - $rem ))
        imagesize=$(( $imagesize + $topad ))
        cp $input $TMP/blpad.bin
        pad_file $TMP/blpad.bin $imagesize
        input=$TMP/blpad.bin
    fi

    # Hash payload
    openssl dgst -sha256 -binary $input > $TMP/bl-pl.sha

    # Add hash to header
    ${TOOL_PATH}/sign-boot-c1 --create-sbImage-header \
            --hash $TMP/bl-pl.sha \
            --arb-cvn $arb_cvn \
            -o $TMP/bl.hdr

    # Pad header
    # pad to (sizeof(sbImageHeader_t) - nonce[16] - checksum[32])
    pad_file $TMP/bl.hdr $(( 656 - 16 - 32))

    openssl dgst -sha256 -binary $TMP/bl.hdr > $TMP/bl.hdr.sha

    # Create nonce
    dd if=/dev/urandom of=$TMP/nonce.bin bs=16 count=1 >& /dev/null

    # Combine nonce + hdr + sha + payload
    cat $TMP/nonce.bin $TMP/bl.hdr $TMP/bl.hdr.sha $input > $TMP/bl.bin

    cp $TMP/bl.bin $output
}

# Convert RSA private PEM key to precomputed binary key file
# If input is already the precomputed binary key file, then it is simply copied
# to the output
# $1: input RSA private .PEM
# $2: output precomputed binary key file
# $3: enable N0INV128 if True
pem_to_bin() {
    local input=$1
    local output=$2
    local n0inv128_enable=$3
    if [ ! -f "$1" ] || [ -z "$2" ]; then
        echo "Argument error, \"$1\", \"$2\" "
        exit 1
    fi
    if [ -z "$n0inv128_enable" ]; then
      n0inv128_enable=False
    fi

    local insize=$(wc -c < $input)
    if [ $insize -eq 1052 ] || [ $insize -eq 1036 ]; then
        # input is already precomputed binary key file
        cp $input $output
    fi

    local pycmd="import sys; \
                 sys.path.append(\"${TOOL_PATH}\"); \
                 import pem_extract_pubkey; \
                 sys.stdout.write(pem_extract_pubkey.extract_pubkey( \
                    \"$input\", headerMode=False, n0inv128Enable=$n0inv128_enable));"
    /usr/bin/env python -c "$pycmd" > $output
}

# Create FIP
create_fip_unsigned() {
    local bl30=""
    local bl31=""
    local bl32=""
    local bl31_info=""
    local bl32_info=""
    local bl33=""
    local output=""
    local argv=("$@")
    local i=0
    local ddrfw1=""
    local ddrfw1_sha=""
    local ddrfw2=""
    local ddrfw2_sha=""
    local ddrfw3=""
    local ddrfw3_sha=""
    local ddrfw4=""
    local ddrfw4_sha=""
    local ddrfw5=""
    local ddrfw5_sha=""
    local ddrfw6=""
    local ddrfw6_sha=""
    local ddrfw7=""
    local ddrfw7_sha=""
    local ddrfw8=""
    local ddrfw8_sha=""
    local ddrfw9=""
    local ddrfw9_sha=""
    local ddrfw10=""
    local ddrfw10_sha=""
    local size=0
    local ddrfwalignment=16384

    touch $TMP/ddr.fw.bin

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --bl30)
                bl30="${argv[$i]}" ;;
            --bl31)
                bl31="${argv[$i]}" ;;
            --bl32)
                bl32="${argv[$i]}" ;;
            --bl31-info)
                bl31_info="${argv[$i]}" ;;
            --bl32-info)
                bl32_info="${argv[$i]}" ;;
            --bl33)
                bl33="${argv[$i]}" ;;
            --ddrfw1)
                ddrfw1="${argv[$i]}"
                if [ "$ddrfw1" != "" ]; then
                    check_file ddrfw1 "$ddrfw1"

                    # 16K align
                    size=$(aligned_size "$ddrfw1" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw1) $(wc -c < ${ddrfw1}) --> $size"

                    dd if=$ddrfw1 of=$TMP/ddrfw1.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw1 of=$TMP/ddrfw1.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw1.padded $size
                    dd if=$TMP/ddrfw1.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw1_sha=$TMP/ddrfw1.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw1.padded > $ddrfw1_sha

                    cat $TMP/ddrfw1.hdr $TMP/ddrfw1.padded > $TMP/ddrfw1.hdr.padded
                    ddrfw1=$TMP/ddrfw1.hdr.padded
                fi
                trace "DDR FW1 $ddrfw1 $ddrfw1_sha"
                ;;
            --ddrfw2)
                ddrfw2="${argv[$i]}"
                if [ "$ddrfw2" != "" ]; then
                    check_file ddrfw2 "$ddrfw2"

                    # 16K align
                    size=$(aligned_size "$ddrfw2" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw2) $(wc -c < ${ddrfw2}) --> $size"

                    dd if=$ddrfw2 of=$TMP/ddrfw2.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw2 of=$TMP/ddrfw2.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw2.padded $size
                    dd if=$TMP/ddrfw2.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw2_sha=$TMP/ddrfw2.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw2.padded> $ddrfw2_sha

                    cat $TMP/ddrfw2.hdr $TMP/ddrfw2.padded > $TMP/ddrfw2.hdr.padded
                    ddrfw2=$TMP/ddrfw2.hdr.padded
                fi
                trace "DDR FW2 $ddrfw2 $ddrfw2_sha"
                ;;
            --ddrfw3)
                ddrfw3="${argv[$i]}"
                if [ "$ddrfw3" != "" ]; then
                    check_file ddrfw3 "$ddrfw3"

                    # 16K align
                    size=$(aligned_size "$ddrfw3" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw3) $(wc -c < ${ddrfw3}) --> $size"

                    dd if=$ddrfw3 of=$TMP/ddrfw3.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw3 of=$TMP/ddrfw3.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw3.padded $size
                    dd if=$TMP/ddrfw3.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw3_sha=$TMP/ddrfw3.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw3.padded > $ddrfw3_sha

                    cat $TMP/ddrfw3.hdr $TMP/ddrfw3.padded > $TMP/ddrfw3.hdr.padded
                    ddrfw3=$TMP/ddrfw3.hdr.padded
                fi
                trace "DDR FW3 $ddrfw3 $ddrfw3_sha"
                ;;
            --ddrfw4)
                ddrfw4="${argv[$i]}"
                if [ "$ddrfw4" != "" ]; then
                    check_file ddrfw4 "$ddrfw4"

                    # 16K align
                    size=$(aligned_size "$ddrfw4" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw4) $(wc -c < ${ddrfw4}) --> $size"

                    dd if=$ddrfw4 of=$TMP/ddrfw4.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw4 of=$TMP/ddrfw4.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw4.padded $size
                    dd if=$TMP/ddrfw4.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw4_sha=$TMP/ddrfw4.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw4.padded > $ddrfw4_sha

                    cat $TMP/ddrfw4.hdr $TMP/ddrfw4.padded > $TMP/ddrfw4.hdr.padded
                    ddrfw4=$TMP/ddrfw4.hdr.padded
                fi
                trace "DDR FW4 $ddrfw4 $ddrfw4_sha"
                ;;
            --ddrfw5)
                ddrfw5="${argv[$i]}"
                if [ "$ddrfw5" != "" ]; then
                    check_file ddrfw5 "$ddrfw5"

                    # 16K align
                    size=$(aligned_size "$ddrfw5" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw5) $(wc -c < ${ddrfw5}) --> $size"

                    dd if=$ddrfw5 of=$TMP/ddrfw5.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw5 of=$TMP/ddrfw5.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw5.padded $size
                    dd if=$TMP/ddrfw5.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw5_sha=$TMP/ddrfw5.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw5.padded > $ddrfw5_sha

                    cat $TMP/ddrfw5.hdr $TMP/ddrfw5.padded > $TMP/ddrfw5.hdr.padded
                    ddrfw5=$TMP/ddrfw5.hdr.padded
                fi
                trace "DDR FW5 $ddrfw5 $ddrfw5_sha"
                ;;
            --ddrfw6)
                ddrfw6="${argv[$i]}"
                if [ "$ddrfw6" != "" ]; then
                    check_file ddrfw6 "$ddrfw6"

                    # 16K align
                    size=$(aligned_size "$ddrfw6" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw6) $(wc -c < ${ddrfw6}) --> $size"

                    dd if=$ddrfw6 of=$TMP/ddrfw6.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw6 of=$TMP/ddrfw6.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw6.padded $size
                    dd if=$TMP/ddrfw6.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw6_sha=$TMP/ddrfw6.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw6.padded > $ddrfw6_sha

                    cat $TMP/ddrfw6.hdr $TMP/ddrfw6.padded > $TMP/ddrfw6.hdr.padded
                    ddrfw6=$TMP/ddrfw6.hdr.padded
                fi
                trace "DDR FW6 $ddrfw6 $ddrfw6_sha"
                ;;
            --ddrfw7)
                ddrfw7="${argv[$i]}"
                if [ "$ddrfw7" != "" ]; then
                    check_file ddrfw7 "$ddrfw7"

                    # 16K align
                    size=$(aligned_size "$ddrfw7" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw7) $(wc -c < ${ddrfw7}) --> $size"

                    dd if=$ddrfw7 of=$TMP/ddrfw7.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw7 of=$TMP/ddrfw7.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw7.padded $size
                    dd if=$TMP/ddrfw7.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw7_sha=$TMP/ddrfw7.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw7.padded > $ddrfw7_sha

                    cat $TMP/ddrfw7.hdr $TMP/ddrfw7.padded > $TMP/ddrfw7.hdr.padded
                    ddrfw7=$TMP/ddrfw7.hdr.padded
                fi
                trace "DDR FW7 $ddrfw7 $ddrfw7_sha"
                ;;
            --ddrfw8)
                ddrfw8="${argv[$i]}"
                if [ "$ddrfw8" != "" ]; then
                    check_file ddrfw8 "$ddrfw8"

                    # 16K align
                    size=$(aligned_size "$ddrfw8" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw8) $(wc -c < ${ddrfw8}) --> $size"

                    dd if=$ddrfw8 of=$TMP/ddrfw8.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw8 of=$TMP/ddrfw8.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw8.padded $size
                    dd if=$TMP/ddrfw8.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw8_sha=$TMP/ddrfw8.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw8.padded > $ddrfw8_sha

                    cat $TMP/ddrfw8.hdr $TMP/ddrfw8.padded > $TMP/ddrfw8.hdr.padded
                    ddrfw8=$TMP/ddrfw8.hdr.padded
                fi
                trace "DDR FW8 $ddrfw8 $ddrfw8_sha"
                ;;
            --ddrfw9)
                ddrfw9="${argv[$i]}"
                if [ "$ddrfw9" != "" ]; then
                    check_file ddrfw9 "$ddrfw9"

                    # 16K align
                    size=$(aligned_size "$ddrfw9" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw9) $(wc -c < ${ddrfw9}) --> $size"

                    dd if=$ddrfw9 of=$TMP/ddrfw9.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw9 of=$TMP/ddrfw9.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw9.padded $size
                    dd if=$TMP/ddrfw9.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw9_sha=$TMP/ddrfw9.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw9.padded > $ddrfw9_sha

                    cat $TMP/ddrfw9.hdr $TMP/ddrfw9.padded > $TMP/ddrfw9.hdr.padded
                    ddrfw9=$TMP/ddrfw9.hdr.padded
                fi
                trace "DDR FW9 $ddrfw9 $ddrfw9_sha"
                ;;
            --ddrfw10)
                ddrfw10="${argv[$i]}"
                if [ "$ddrfw10" != "" ]; then
                    check_file ddrfw10 "$ddrfw10"

                    # 16K align
                    size=$(aligned_size "$ddrfw10" 96 $ddrfwalignment)
                    trace "sizeof($ddrfw10) $(wc -c < ${ddrfw10}) --> $size"

                    dd if=$ddrfw10 of=$TMP/ddrfw10.hdr bs=1 count=96 >& /dev/null
                    dd if=$ddrfw10 of=$TMP/ddrfw10.padded bs=1 skip=96 >& /dev/null
                    pad_file $TMP/ddrfw10.padded $size
                    dd if=$TMP/ddrfw10.padded of=$TMP/ddr.fw.bin oflag=append conv=notrunc >& /dev/null

                    ddrfw10_sha=$TMP/ddrfw10.padded.sha
                    openssl dgst -sha256 -binary $TMP/ddrfw10.padded > $ddrfw10_sha

                    cat $TMP/ddrfw10.hdr $TMP/ddrfw10.padded > $TMP/ddrfw10.hdr.padded
                    ddrfw10=$TMP/ddrfw10.hdr.padded
                fi
                trace "DDR FW10 $ddrfw10 $ddrfw10_sha"
                ;;
            -o)
                output="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1 ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    check_file bl30 "$bl30"
    check_file bl31 "$bl31"
    check_file bl32 "$bl32"
    check_file bl31_info "$bl31_info"
    check_file bl32_info "$bl32_info"
    check_file bl33 "$bl33"

    if [ -z "$output" ]; then echo Error: Missing output file option -o ; exit 1; fi

    #dummy rsa/key/iv because utility can't handle unsigned yet
    dd if=/dev/zero of=$TMP/zerorsakey bs=1 count=1036 >& /dev/null
    dd if=/dev/zero of=$TMP/zeroaeskey bs=1 count=32 >& /dev/null
    dd if=/dev/zero of=$TMP/zeroaesiv bs=1 count=16 >& /dev/null

    # Create header and add keys
    local argv=("$@")
    ${TOOL_PATH}/sign-boot-c1 --create-fip-header \
        --bl30 $bl30 --bl30-key $TMP/zerorsakey \
        --bl30-aes-key $TMP/zeroaeskey --bl30-aes-iv $TMP/zeroaesiv \
        --bl31 $bl31 --bl31-key $TMP/zerorsakey \
        --bl31-aes-key $TMP/zeroaeskey --bl31-aes-iv $TMP/zeroaesiv \
        --bl32 $bl32 --bl32-key $TMP/zerorsakey  \
        --bl32-aes-key $TMP/zeroaeskey --bl32-aes-iv $TMP/zeroaesiv \
        --bl33 $bl33 --bl33-key $TMP/zerorsakey  \
        --bl33-aes-key $TMP/zeroaeskey --bl33-aes-iv $TMP/zeroaesiv \
        --bl31-info $bl31_info --bl32-info $bl32_info \
        --kernel-key $TMP/zerorsakey    \
        --kernel-aes-key $TMP/zeroaeskey --kernel-aes-iv $TMP/zeroaesiv \
        --ddrfw1   "$ddrfw1" \
        --ddrfw1-sha   "$ddrfw1_sha" \
        --ddrfw2   "$ddrfw2" \
        --ddrfw2-sha   "$ddrfw2_sha" \
        --ddrfw3   "$ddrfw3" \
        --ddrfw3-sha   "$ddrfw3_sha" \
        --ddrfw4   "$ddrfw4" \
        --ddrfw4-sha   "$ddrfw4_sha" \
        --ddrfw5   "$ddrfw5" \
        --ddrfw5-sha   "$ddrfw5_sha" \
        --ddrfw6   "$ddrfw6" \
        --ddrfw6-sha   "$ddrfw6_sha" \
        --ddrfw7   "$ddrfw7" \
        --ddrfw7-sha   "$ddrfw7_sha" \
        --ddrfw8   "$ddrfw8" \
        --ddrfw8-sha   "$ddrfw8_sha" \
        --ddrfw9   "$ddrfw9" \
        --ddrfw9-sha   "$ddrfw9_sha" \
        --ddrfw10   "$ddrfw10" \
        --ddrfw10-sha   "$ddrfw10_sha" \
        -o $TMP/fip.hdr

    # Pad header to size
    # pad to (sizeof(fip hdr) - nonce[16] - checksum[32])
    pad_file $TMP/fip.hdr $(( (16*1024) - 16 - 32))

    openssl dgst -sha256 -binary $TMP/fip.hdr > $TMP/fip.hdr.sha

    # Create nonce
    dd if=/dev/urandom of=$TMP/nonce.bin bs=16 count=1 >& /dev/null

    # Combine nonce + hdr + sha
    cat $TMP/nonce.bin $TMP/fip.hdr $TMP/fip.hdr.sha > $TMP/fip.bin

    cp $TMP/fip.bin $output
}

# Check (bl31/bl32) input is .img
# 1: input
# returns True or False
is_img_format() {
    local input=$1
    if [ ! -f "$1" ]; then
        echo "Argument error, \"$1\""
        exit 1
    fi
    local insize=$(wc -c < $input)
    if [ $insize -le 512 ]; then
        # less than size of img header
        echo False
        return
    fi

    local inmagic=$(xxd -p -l 4 $input)
    if [ "$inmagic" == "65873412" ]; then
        # Input has FIP_TOC_ENTRY_EXT_FLAG, so it is in .img format.
        # Strip off 0x200 byte header.
        echo True
    else
        echo False
    fi
}

# Convert from .img to .bin.  If not .img format, copy to output.
# 1: input
# 2: output
convert_img_to_bin() {
    local input=$1
    local output=$2
    if [ ! -f "$1" ] || [ -z "$2" ]; then
        echo "Argument error, \"$1\", \"$2\" "
        exit 1
    fi
    local insize=$(wc -c < $input)
    if [ $insize -le 512 ]; then
        # less than size of img header
        cp $input $output
        return
    fi

    local inmagic=$(xxd -p -l 4 $input)
    if [ "$inmagic" == "65873412" ]; then
        # Input has FIP_TOC_ENTRY_EXT_FLAG, so it is in .img format.
        # Strip off 0x200 byte header.
        tail -c +513 $input > $output
    else
        cp $input $output
    fi
}

# input bl2, bl30/31/32/33kernel .bin
create_unsigned_bl() {
    local bl2=""
    local bl2size=65536
    local bl30=""
    local bl30size=58368
    local bl31=""
    local bl32=""
    local bl33=""
    local ddrfw1=""
    local ddrfw2=""
    local ddrfw3=""
    local ddrfw4=""
    local ddrfw5=""
    local ddrfw6=""
    local ddrfw7=""
    local ddrfw8=""
    local ddrfw9=""
    local ddrfw10=""
    local ddrfw_size=0
    local output=""
    local soc="g12a"
    local argv=("$@")
    local i=0

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --soc)
                soc="${argv[$i]}" ;;
            --bl2)
                bl2="${argv[$i]}" ;;
            --bl2-size)
                bl2size="${argv[$i]}" ;;
            --bl30)
                bl30="${argv[$i]}" ;;
            --bl30-size)
                bl30size="${argv[$i]}" ;;
            --bl31)
                bl31="${argv[$i]}" ;;
            --bl32)
                bl32="${argv[$i]}" ;;
            --bl33)
                bl33="${argv[$i]}" ;;
            --ddrfw1)
                ddrfw1="${argv[$i]}" ;;
            --ddrfw2)
                ddrfw2="${argv[$i]}" ;;
            --ddrfw3)
                ddrfw3="${argv[$i]}" ;;
            --ddrfw4)
                ddrfw4="${argv[$i]}" ;;
            --ddrfw5)
                ddrfw5="${argv[$i]}" ;;
            --ddrfw6)
                ddrfw6="${argv[$i]}" ;;
            --ddrfw7)
                ddrfw7="${argv[$i]}" ;;
            --ddrfw8)
                ddrfw8="${argv[$i]}" ;;
            --ddrfw9)
                ddrfw9="${argv[$i]}" ;;
            --ddrfw10)
                ddrfw10="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1 ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    set_soc "$soc"

    check_file bl2 "$bl2"
    if $bl30_required || [ "$bl30" != "" ]; then
        check_file bl30 "$bl30"
    fi
    check_file bl31 "$bl31"
    if [ "$bl32" != "" ]; then
        check_file bl32 "$bl32"
    fi
    check_file bl33 "$bl33"

    local bl2_payload_size=$(wc -c < ${bl2})
    trace "BL2 size specified $bl2size"
    trace "Input BL2 payload size $bl2_payload_size"
    if [ $bl2size -ne $(($bl2_payload_size + 4096)) ]; then
        echo Error: invalid bl2 input file size $bl2_payload_size
        exit 1
    fi

    if $bl30_required || [ "$bl30" != "" ]; then
        local bl30_payload_size=$(wc -c < ${bl30})
        trace "BL30 size specified $bl30size"
        trace "Input BL30 payload size $bl30_payload_size"
        if [ $bl30size -ne $(($bl30_payload_size + 4096)) ]; then
            echo Error: invalid bl30 payload size $bl30_payload_size
            exit 1
        fi
    fi

    if [ -z "$output" ]; then echo Error: Missing output file option -o ; exit 1; fi

    if [ "$(is_img_format $bl31)" != "True" ]; then
        echo Error. Expected .img format for \"$bl31\"
        exit 1
    fi
    if [ "$bl32" != "" ]; then
        if [ "$(is_img_format $bl32)" != "True" ]; then
            echo Error. Expected .img format for \"$bl32\"
            exit 1
        fi
    fi

    convert_img_to_bin $bl31 $TMP/bl31.bin
    if [ "$bl32" != "" ]; then
        convert_img_to_bin $bl32 $TMP/bl32.bin
    fi

    pack_bl2 -i $bl2 -o $TMP/bl2.bin.img -s $bl2size

    if [ "$bl30" == "" ]; then
        touch $TMP/bl30.bin.img
    else
        pack_bl2 -i $bl30 -o $TMP/bl30.bin.img -s $bl30size
    fi

    pack_bl3x -i $TMP/bl31.bin -o $TMP/bl31.bin.img
    if [ "$bl32" != "" ]; then
        pack_bl3x -i $TMP/bl32.bin -o $TMP/bl32.bin.img
    else
        rm -f $TMP/bl32.bin.img
        touch $TMP/bl32.bin.img
        # use bl31 as dummy bl32 img info
        cp $bl31 $TMP/bl32.img-info
        bl32=$TMP/bl32.img-info
    fi
    pack_bl3x -i $bl33 -o $TMP/bl33.bin.img

    create_fip_unsigned \
        --bl30     $TMP/bl30.bin.img \
        --bl31     $TMP/bl31.bin.img \
        --bl32     $TMP/bl32.bin.img \
        --bl31-info     $bl31 \
        --bl32-info     $bl32 \
        --bl33     $TMP/bl33.bin.img \
        --ddrfw1   "$ddrfw1" \
        --ddrfw2   "$ddrfw2" \
        --ddrfw3   "$ddrfw3" \
        --ddrfw4   "$ddrfw4" \
        --ddrfw5   "$ddrfw5" \
        --ddrfw6   "$ddrfw6" \
        --ddrfw7   "$ddrfw7" \
        --ddrfw8   "$ddrfw8" \
        --ddrfw9   "$ddrfw9" \
        --ddrfw10  "$ddrfw10" \
        -o $TMP/fip.hdr.out

    # TODO:
    # Call fixup script to create blxx_new.bin
    #package

    cat $TMP/bl2.bin.img $TMP/fip.hdr.out $TMP/ddr.fw.bin $TMP/bl30.bin.img $TMP/bl31.bin.img \
        $TMP/bl32.bin.img $TMP/bl33.bin.img > $output

    echo
    echo Created unsigned bootloader $output successfully
}

create_unsigned_bl40() {
    local bl40=""
    local bl40size=0
    local output=""
    local argv=("$@")
    local i=0

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --bl40)
                bl40="${argv[$i]}" ;;
            --bl40-size)
                bl40size="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1 ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    check_file bl40 "$bl40"

    local bl40_payload_size=$(wc -c < ${bl40})
    if [ $bl40_payload_size -gt 241664 ]; then
        echo Error: payload size overflow $bl40_payload_size
        exit 1
    fi
    if [ $bl40size -eq 0 ]; then
        bl40size=$(($bl40_payload_size + 4096))
    fi
    trace "BL40 size specified $bl40size"
    trace "Input BL40 payload size $bl40_payload_size"
    if [ $bl40size -ne $(($bl40_payload_size + 4096)) ]; then
        echo Error: invalid bl40 input file size $bl40_payload_size
        exit 1
    fi

    pack_bl2 -i $bl40 -o $TMP/bl40.bin.img -s $bl40size

    # TODO:
    # Call fixup script to create blxx_new.bin
    #package

    cat $TMP/bl40.bin.img > $output

    echo
    echo Created unsigned bl40 $output successfully
}

parse_main() {
    local i=0
    local argv=()
    for arg in "$@" ; do
        argv[$i]="$arg"
        i=$((i + 1))
    done

    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        case "$arg" in
            -h|--help)
                usage
                break ;;
            --create-unsigned-bl)
                create_unsigned_bl "${argv[@]:$((i + 1))}"
                break ;;
            --create-unsigned-bl40)
                create_unsigned_bl40 "${argv[@]:$((i + 1))}"
                break ;;
            --create-root-hash)
                create_root_hash "${argv[@]:$((i + 1))}"
                break ;;
            --create-root-hash-from-pub)
                create_root_hash_from_pub "${argv[@]:$((i + 1))}"
                break ;;
            *)
                echo "Unknown first option $1"; exit 1
                ;;
        esac
        i=$((i + 1))
    done
}

cleanup() {
    if [ $? -ne 0 ]; then
        echo
        echo "Failed to create unsigned bootloader!"
    fi

    if [ ! -d "$TMP" ]; then return; fi
    local tmpfiles="bl2.bin.img bl2.img bl2.img-noiv bl2.sha
    bl30.bin.img bl31.bin bl31.bin.img bl32.bin bl32.bin.img bl32.img-info
    bl33.bin.img bl.bin bl.hdr bl.hdr.sha blpad.bin
    bl40.bin.img
    bl-pl.sha chkdata fip.bin fip.hdr fip.hdr.out
    fip.hdr.sha kernel keydata mod modhex nonce.bin
    rootkey0.bin rootkey0.sha rootkey1.bin rootkey1.sha
    rootkey0.pub rootkey1.pub rootkey2.pub rootkey3.pub
    rootkey2.bin rootkey2.sha rootkey3.bin rootkey3.sha rootkeys.sha
    zeroaesiv zeroaeskey zerorsakey fip.hdr.bin fip.payload.bin bl3x.payload.bin bl3x.bin ddrfw*.padded* ddrfw*.hdr ddr.fw.bin"
    for i in $tmpfiles ; do
        rm -f $TMP/$i
    done
    rmdir $TMP || true
}

trap cleanup EXIT

cleanup
if [ ! -d "$TMP" ]; then mkdir "$TMP" ; fi
parse_main "$@"
