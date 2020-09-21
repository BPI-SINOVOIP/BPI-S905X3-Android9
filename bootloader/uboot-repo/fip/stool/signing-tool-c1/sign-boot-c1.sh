#!/bin/bash -e

# Copyright (c) 2018 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.

#set -x

SCRIPT_PATH=${SCRIPT_PATH:-$(dirname $(readlink -f $0))}

# Path to sign-boot-c1 util
TOOL_PATH=${TOOL_PATH:-$(dirname $(readlink -f $0))}

USE_TOOL_SCRIPT_VERSION=false
if $USE_TOOL_SCRIPT_VERSION; then
    CREATE_HEADER_TOOL=${TOOL_PATH}/create-header.sh
else
    CREATE_HEADER_TOOL=${TOOL_PATH}/sign-boot-c1
fi

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

       All-in-one command to create a signed bootloader:

       $(basename $0) --create-signed-bl	\\
                [--soc      (g12a|g12b|sm1|a1|c1)] \\
                --bl2		bl2.bin		\\
                --bl30		bl30.bin	\\
                --bl30-key	bl30key.pem	\\
                [--bl30-aes-key	bl30aeskey	\\
                 --bl30-aes-iv	bl30aesiv]	\\
                --bl31		bl31.img	\\
                --bl31-key	bl31key.pem	\\
                [--bl31-aes-key	bl31aeskey	\\
                 --bl31-aes-iv	bl31aesiv]	\\
                [--bl32		bl32.img	\\
                 --bl32-key	bl32key.pem]	\\
                [--bl32-aes-key	bl32aeskey	\\
                 --bl32-aes-iv	bl32aesiv]	\\
                --bl33		bl33.bin	\\
                --bl33-key	bl33key.pem	\\
                [--bl33-aes-key	bl33aeskey	\\
                 --bl33-aes-iv	bl33aesiv]	\\
                [--kernel-key	kernelkey.pem	\\
                 --kernel-aes-key	kernelaeskey	\\
                 --kernel-aes-iv	kernelaesiv]	\\
                [--fip-key     fipkey.pem]     \\
                [--bl2-arb-cvn <version>]      \\
                [--fip-arb-cvn <version>]      \\
                [--bl30-arb-cvn        <version>]      \\
                [--bl31-arb-cvn        <version>]      \\
                [--bl32-arb-cvn        <version>]      \\
                [--bl33-arb-cvn        <version>]      \\
                [--e            none]          \\
                [--root-key-idx <index>]       \\
                [--marketid     <marketid>]    \\
                -o bl.bin

       Environment Variables:

       TMP:          path to a temporary directory. Defaults to <this script's path>/tmp
EOF
    exit 1
}

## Globals
readonly BL30_SIZE=58368
## /Globals

## Global SoC Config
current_soc=""
bl30_required=true
separate_fip_aes=false
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
            separate_fip_aes=true
            keyhashver_min=3
            ;;
        *)
            echo "Invalid soc $1"; exit 1 ;;
    esac
    readonly bl30_required separate_fip_aes
    readonly current_soc="$1"
}
## /Global SoC Config

check_file() {
    if [ ! -f "$2" ]; then echo Error: Unable to open $1: \""$2"\"; exit 1 ; fi
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

# Hash root/bl2 RSA keys'.  sha256(n[keylen] + e)
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

    openssl dgst -sha256 -out $output -binary $TMP/keydata
}

# Sign bl2 or bl30
sign_bl2() {
    local input=""
    local bl2hdr=""
    local bl2key=""
    local aesiv=""
    local output=""
    local sigver=""
    local keyhashver=""
    local argv=("$@")
    local i=0
    local bl2size=65536

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            -i)
                input="${argv[$i]}" ;;
            --bl2-hdr)
                bl2hdr="${argv[$i]}" ;;
            --bl2-key)
                bl2key="${argv[$i]}" ;;
            --bl2-size)
                bl2size="${argv[$i]}" ;;
            --iv)
                aesiv="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            --sig-ver)
                sigver="${argv[$i]}" ;;
            --key-hash-ver)
                keyhashver="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done

    # Verify args
    check_file bl2  "$input"
    check_file bl2-hdr  "$bl2hdr"
    check_file bl2key "$bl2key"
    if [ -z "$output" ]; then echo Error: Missing output file option -o; exit 1; fi
    if [ ! -z "$aesiv"  ]; then
        check_file "AES IV" "$aesiv"
    fi
    if [ -z "$sigver" ]; then
        sigver=1
    fi
    if [ "$keyhashver" != "2" ] && [ "$keyhashver" != "3" ]; then
        echo Error: bad key hash ver; exit 1
    fi

    local keylenbytes=$(get_pem_key_len "$bl2key")
    local keylen=$(( $keylenbytes * 8 ))

    local insize=$(wc -c < "$bl2hdr")
    if [ $insize -ne 4096 ]; then
        echo "bl2-hdr wrong size ($insize != 4096)"
        exit 1
    fi
    local bl2_payload_size=$(wc -c < ${input})
    trace "BL2 size specified $bl2size"
    trace "Input BL2 payload size $bl2_payload_size"
    if [ $bl2size -ne $(($bl2_payload_size + 4096)) ]; then
        echo Error: invalid bl2 input file size $bl2_payload_size
        exit 1
    fi

    # Add bl2-hdr + bl2
    cat "$bl2hdr" "$input" > "$TMP/bl2.img"

    # Truncate to correct length
    # TODO should check that end of output is all zeroes before truncating
    truncate -s $bl2size $TMP/bl2.img

    # Sign second part with bl2 key
    dd if=$TMP/bl2.img of=$TMP/sigdata bs=1 skip=2264 count=64 >& /dev/null
    chkstart=$(( 2264 + 64 + keylenbytes ))
    chksize=$(( $bl2_payload_size + 1832 - 64 - keylenbytes ))
    # - from nChkStart to end of bl2
    dd if=$TMP/bl2.img of=$TMP/sigdata bs=1 skip=$chkstart count=$chksize \
        oflag=append conv=notrunc >& /dev/null
    openssl dgst -sha256 -sign $bl2key -out $TMP/sigdata.sig $TMP/sigdata

    # Replace signature placeholder
    dd if=$TMP/sigdata.sig of=$TMP/bl2.img bs=1 seek=2328 count=$keylenbytes \
        conv=notrunc >& /dev/null

    # Replace nonce
    if [ ! -z "$aesiv" ]; then
        dd if=$aesiv of=$TMP/bl2.img bs=16 count=1 \
            conv=notrunc >& /dev/null
    else
        dd if=/dev/urandom of=$TMP/bl2.img bs=16 count=1 \
            conv=notrunc >& /dev/null
    fi

    cp $TMP/bl2.img $output
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
    if [ "$bits" == "" ]; then
       echo "Unexpected key size  $bits"
       exit 1
    fi
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

# $1: input
# $2: output
# $3: aes key file
# $4: aes iv file
internal_encrypt() {
    local input=$1
    local output=$2
    local keyfile=$3
    local ivfile=$4
    if [ ! -f "$1" ] || [ -z "$2" ] || [ -z "$3" ] || [ -z "$4" ]; then
        echo "Argument error"
        exit 1
    fi
    local key=$(xxd -p -c64 $keyfile)
    local iv=$(xxd -p -c64 $ivfile)
    local imagesize=$(wc -c < ${input})
    local rem=$(( $imagesize % 16 ))
    if [ $rem -ne 0 ]; then
        echo "Input $input not 16 byte aligned?"
        exit 1
    fi
    openssl enc -aes-256-cbc -K $key -iv $iv -e -in $input -out $output -nopad
}

# sign bl3x
sign_bl3x() {
    local input=""
    local key=""
    local output=""
    local arb_cvn=""
    local aeskey=""
    local aesiv=""
    local argv=("$@")
    local encrypt=false
    local stage=""
    local i=0

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            -i)
                input="${argv[$i]}" ;;
            -k)
                key="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            -a)
                aeskey="${argv[$i]}" ;;
            --iv)
                aesiv="${argv[$i]}" ;;
            --stage)
                stage="${argv[$i]}" ;;
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
    if [ ! -z "$key" ]; then
        check_file "RSA key" "$key"
    else
        echo Error missing key
        exit 1
    fi
    check_file "RSA key" "$key"
    if [ ! -z "$aeskey" ]; then
        if [ -z "$aesiv"  ]; then echo Error: missing AES IV; exit 1; fi
        check_file "AES key" "$aeskey"
        check_file "AES IV" "$aesiv"
        encrypt=true
    fi
    if [ "$stage" != "bl30" ] &&
            [ "$stage" != "bl31" ] &&
            [ "$stage" != "bl32" ] &&
            [ "$stage" != "bl33" ]; then
        echo Error invalid bl3x stage \"$stage\"
        exit 1
    fi

    # Pad so that hdr + payload is 512 byte aligned
    # because of bl2 driver limitation
    local imagesize=$(wc -c < ${input})
    local rem=$(( (656 + $imagesize) % 512 ))
    if [ $rem -ne 0 ]; then
        #echo "Input $input not 512 byte aligned?"
        local topad=$(( 512 - $rem ))
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
            --stage $stage \
            -o $TMP/bl.hdr

    # Pad header to push signature to end
    # pad to (sizeof(sbImageHeader_t) - signature[keylen] - nonce[16])
    local keylen=$(get_pem_key_len $key)
    pad_file $TMP/bl.hdr $(( 656 - $keylen - 16 ))

    # Sign header
    openssl dgst -sha256 -sign $key -out $TMP/bl.hdr.sig $TMP/bl.hdr

    # Create nonce
    dd if=/dev/urandom of=$TMP/nonce.bin bs=16 count=1 >& /dev/null

    # Combine nonce + hdr + sig + payload
    cat $TMP/nonce.bin $TMP/bl.hdr $TMP/bl.hdr.sig $input > $TMP/bl.bin

    if $encrypt; then
        internal_encrypt $TMP/bl.bin $output $aeskey $aesiv
        #external_encrypt $TMP/bl.bin $output
    else
        cp $TMP/bl.bin $output
    fi
}

append_uint32_le() {
    local input=$1
    local output=$2
    local v=
    local vrev=
    v=$(printf %08x $input)
    # 00010001
    vrev=${v:6:2}${v:4:2}${v:2:2}${v:0:2}

    echo $vrev | xxd -r -p >> $output
}

append_uint64_le() {
    local input=$1
    local output=$2
    local v=
    local vrev=
    v=$(printf %016x $input)

    vrev=${v:14:2}${v:12:2}${v:10:2}${v:8:2}${v:6:2}${v:4:2}${v:2:2}${v:0:2}

    echo $vrev | xxd -r -p >> $output
}

# Encrypt/sign kernel
#typedef struct {
#	uint32_t magic;
#	uint32_t version;
#	uint32_t flags;
#	uint32_t img_version;
#	uint32_t img_size;
#	uint32_t img_offset;
#	uint8_t img_hash[32];
#	uint8_t reserved[200];
#	uint8_t rsa_sig[256];
#} aml_boot_header_t;
#
#CASSERT(sizeof(aml_boot_header_t) == 512, assert_sizeof_aml_boot_header_t);
sign_kernel() {
    local input=""
    local key=""
    local output=""
    local arb_cvn=""
    local aeskey=""
    local aesiv=""
    local argv=("$@")
    local encrypt=false
    local stage="kernel"
    local i=0

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            -i)
                input="${argv[$i]}" ;;
            -k)
                key="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            -a)
                aeskey="${argv[$i]}" ;;
            --iv)
                aesiv="${argv[$i]}" ;;
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
    if [ ! -z "$key" ]; then
        check_file "RSA key" "$key"
    else
        echo Error missing key
        exit 1
    fi
    check_file "RSA key" "$key"
    if [ ! -z "$aeskey" ]; then
        if [ -z "$aesiv"  ]; then echo Error: missing AES IV; exit 1; fi
        check_file "AES key" "$aeskey"
        check_file "AES IV" "$aesiv"
        encrypt=true
    fi

    local imagesize=$(wc -c < ${input})
    local rem=$(( $imagesize % 512 ))
    if [ $rem -ne 0 ]; then
        #echo "Input $input not 512 byte aligned?"
        local topad=$(( 512 - $rem ))
        imagesize=$(( $imagesize + $topad ))
        cp $input $TMP/kernpad.bin
        pad_file $TMP/kernpad.bin $imagesize
        input=$TMP/kernpad.bin
    fi

    # Hash payload
    openssl dgst -sha256 -binary $input > $TMP/kern-pl.sha

    # Encrypt payload
    if $encrypt; then
        internal_encrypt $input $TMP/kern-pl.bin $aeskey $aesiv
    else
        cp $input $TMP/kern-pl.bin
    fi

    # Create header
    # magic, version, flags, img_version
    echo -n '@AML' > $TMP/kern.hdr
    append_uint32_le 4 $TMP/kern.hdr
    append_uint32_le 0 $TMP/kern.hdr
    append_uint32_le $arb_cvn $TMP/kern.hdr
    # img_size, img_offset, img_hash, reserved
    append_uint32_le $imagesize $TMP/kern.hdr
    append_uint32_le 512 $TMP/kern.hdr
    cat $TMP/kern-pl.sha >> $TMP/kern.hdr
    pad_file $TMP/kern.hdr 256

    # Sign header
    openssl dgst -sha256 -sign $key -out $TMP/kern.hdr.sig $TMP/kern.hdr

    # Combine header + signature
    cat $TMP/kern.hdr.sig >> $TMP/kern.hdr

    # Pad to 512 in case key/sig is smaller than maximum
    pad_file $TMP/kern.hdr 512

    # Combine hdr + payload
    cat $TMP/kern.hdr $TMP/kern-pl.bin > $output

    echo
    echo Created signed kernel $output successfully
}

# Encrypt/sign rtos/NBG
#define IH_MAGIC      0x56190527  /* Image Magic Number, 0x27051956*/
#define IH_ENC_MAGIC	0x43455341  /* Image encrypt flag*/
#define IH_NMLEN	32			/* Image Name Length		*/
#	typedef struct {
#		uint32_t	ih_magic;	/* Image Header Magic Number	*/
#		uint32_t	ih_hcrc;	/* Image Header CRC Checksum	*/
#		uint32_t	ih_time;	/* Image Creation Timestamp */
#		uint32_t	ih_size;	/* Image Data Size		*/
#		uint32_t	ih_load;	/* Data  Load  Address		*/
#		uint32_t	ih_ep;		/* Entry Point Address		*/
#		uint32_t	ih_dcrc;	/* Image Data CRC Checksum	*/
#		uint8_t 	ih_os;		/* Operating System 	*/
#		uint8_t 	ih_arch;	/* CPU architecture 	*/
#		uint8_t 	ih_type;	/* Image Type			*/
#		uint8_t 	ih_comp;	/* Compression Type 	*/
#		uint8_t 	ih_name[IH_NMLEN-4];	/* Image Name		*/
#		uint32_t    ih_encMagic; /*Image content encrypt flag*/
#	} image_header_chk_t;
#
sign_uimage() {
    local input=""
    local key=""
    local output=""
    local aeskey=""
    local aesiv=""
    local argv=("$@")
    local encrypt=false
    local i=0

    # Parse args
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            -i)
                input="${argv[$i]}" ;;
            -k)
                key="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            -a)
                aeskey="${argv[$i]}" ;;
            --iv)
                aesiv="${argv[$i]}" ;;
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

    if [ ! -z "$key" ]; then
        check_file "RSA key" "$key"
    else
        echo Error missing key
        exit 1
    fi
    check_file "RSA key" "$key"
    if [ ! -z "$aeskey" ]; then
        if [ -z "$aesiv"  ]; then echo Error: missing AES IV; exit 1; fi
        check_file "AES key" "$aeskey"
        check_file "AES IV" "$aesiv"
        encrypt=true
    fi

    local ihmagic=$(xxd -p -l 4 $input)
    local ihencmagic=$(xxd -p -seek 60 -l 4 $input)
    if [ "$ihmagic" == "27051956" ]; then
      if [ "$ihencmagic" == "41534543" ]; then
         echo uimage $input was signed, can not sign again!
        return
      fi

    fi

    local imagesize=$(wc -c < ${input})
    local rem=$((( $imagesize + 656 ) % 512 ))
    if [ $rem -ne 0 ]; then
        #echo "Input $input not 512 byte aligned?"
        local topad=$(( 512 - $rem ))
        imagesize=$(( $imagesize + $topad ))
        cp $input $TMP/uimagepad.bin
        pad_file $TMP/uimagepad.bin $imagesize
        input=$TMP/uimagepad.bin
    fi

    dd if="$input" of="$TMP/uimage.hdr" bs=1 count=64 >& /dev/null
    local tempsize=$(( $imagesize - 64 ))
    dd if="$input" of="$TMP/uimage.temp" bs=1 skip=64 count=$tempsize \
        oflag=append conv=notrunc >& /dev/null

    append_uint32_le 0x43455341 "$TMP/ihencmagic"
    dd if="$TMP/ihencmagic" of="$TMP/uimage.hdr" bs=1 seek=60 count=4 \
        conv=notrunc >& /dev/null

    local totalsize=$(( $imagesize + 656 - 64 ))
    printf %08x $totalsize | xxd -r -p > "$TMP/totalsize"
    dd if="$TMP/totalsize" of="$TMP/uimage.hdr" bs=1 seek=12 count=4 \
        conv=notrunc >& /dev/null

    # Hash payload
    openssl dgst -sha256 -binary $TMP/uimage.temp > $TMP/uimage.temp.sha

    # Encrypt payload
    dd if=/dev/urandom of=$TMP/uimage.aeskey bs=1 count=32 >& /dev/null
    dd if=/dev/urandom of=$TMP/uimage.aesiv bs=1 count=16 >& /dev/null
    if $encrypt; then
        internal_encrypt $TMP/uimage.temp $TMP/uimage.temp.bin $TMP/uimage.aeskey $TMP/uimage.aesiv
    else
        cp $TMP/uimage.temp $TMP/uimage.temp.bin
    fi

    # Create sbImageHeader
    # typedef struct{
    #	unsigned char  nonce[16];
    #	unsigned int   magic;
    #	unsigned int   version;
    #	unsigned int   flags;
    #	unsigned int   arb_cvn;
    #	uint64_t       img_size;
    #	uint64_t       img_offset;
    #	unsigned char  img_hash[32];
    #	unsigned char  aes_nonce[16];
    #	unsigned char  aes_key[32];
    #	unsigned char  aes_iv[16];
    #	unsigned char  signature[512];
    # } v3_aml_sbImageHeader_t; //16+16+16+96+512=656

    # Create nonce
    dd if=/dev/urandom of=$TMP/nonce.bin bs=16 count=1 >& /dev/null

    echo -n '@AML' > $TMP/uimage.sb.hdr
    append_uint32_le 1 $TMP/uimage.sb.hdr
    append_uint32_le 0 $TMP/uimage.sb.hdr
    append_uint32_le $arb_cvn $TMP/uimage.sb.hdr
    # img_size, img_offset, img_hash, reserved
    append_uint64_le $tempsize $TMP/uimage.sb.hdr
    append_uint64_le 656 $TMP/uimage.sb.hdr
    cat $TMP/uimage.temp.sha >> $TMP/uimage.sb.hdr
    dd if=/dev/urandom of=$TMP/uimage.sb.hdr oflag=append conv=notrunc bs=1 \
        count=16 >& /dev/null
    cat $TMP/uimage.aeskey >> $TMP/uimage.sb.hdr
    cat $TMP/uimage.aesiv >> $TMP/uimage.sb.hdr

    # Pad header to push signature to end
    # pad to (sizeof(sbImageHeader_t) - signature[keylen] - nonce[16])
    local keylen=$(get_pem_key_len $key)
    pad_file $TMP/uimage.sb.hdr $(( 656 - $keylen - 16 ))

    # Sign header
    openssl dgst -sha256 -sign $key -out $TMP/uimage.sb.hdr.sig $TMP/uimage.sb.hdr

    # Combine header + signature
    cat $TMP/uimage.sb.hdr.sig >> $TMP/uimage.sb.hdr

    dd if="$TMP/uimage.temp.bin" of="$TMP/uimage.temp.bin.first" bs=1 count=656 >& /dev/null
    local separatesize=$(( $tempsize - 656 ))
    dd if="$TMP/uimage.temp.bin" of="$TMP/uimage.temp.bin.second" bs=1 skip=656 count=$separatesize \
        oflag=append conv=notrunc >& /dev/null

    # Combine nonce + sb.hdr
    cat $TMP/nonce.bin $TMP/uimage.sb.hdr > $TMP/uimage.sb.hdr.temp

    if $encrypt; then
        internal_encrypt $TMP/uimage.sb.hdr.temp $TMP/uimage.sb.hdr.bin $aeskey $aesiv
    else
        cp $TMP/uimage.sb.hdr.temp $TMP/uimage.sb.hdr.bin
    fi

    # Combine image.hdr + sb.hdr + payload
    cat $TMP/uimage.hdr $TMP/uimage.sb.hdr.bin $TMP/uimage.temp.bin.second $TMP/uimage.temp.bin.first > $output
    echo
    echo Created signed uimage $output successfully
}

# Convert RSA private PEM key to public key
# $1: input RSA private .PEM
# $2: output public key file
pem_to_pub() {
    local input=$1
    local output=$2
    if [ ! -f "$1" ] || [ -z "$2" ]; then
        echo "Argument error, \"$1\", \"$2\" "
        exit 1
    fi

    openssl 2>/dev/null rsa -in $input -pubout -RSAPublicKey_out -out $output

    if [ $? -ne 0 ]; then
        echo "Error converting key"
        exit 1
    fi
}

# Convert RSA PEM key to precomputed binary key file
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

# Create and sign FIP
#./sign-boot-c1.sh --create-fip \
# ... \
#    -o fip.bin
create_fip() {
    local bl30=""
    local bl31=""
    local bl32=""
    local bl31_info=""
    local bl32_info=""
    local bl33=""
    local bl30key=""
    local bl31key=""
    local bl32key=""
    local bl33key=""
    local bl30aeskey=""
    local bl31aeskey=""
    local bl32aeskey=""
    local bl33aeskey=""
    local bl30aesiv=""
    local bl31aesiv=""
    local bl32aesiv=""
    local bl33aesiv=""
    local kernelkey=""
    local kernelaeskey=""
    local kernelaesiv=""
    local fipkey=""
    local fip_arb_cvn="0"
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
            --bl30-key)
                bl30key="${argv[$i]}" ;;
            --bl31-key)
                bl31key="${argv[$i]}" ;;
            --bl32-key)
                bl32key="${argv[$i]}" ;;
            --bl33-key)
                bl33key="${argv[$i]}" ;;
            --bl30-aes-key)
                bl30aeskey="${argv[$i]}" ;;
            --bl31-aes-key)
                bl31aeskey="${argv[$i]}" ;;
            --bl32-aes-key)
                bl32aeskey="${argv[$i]}" ;;
            --bl33-aes-key)
                bl33aeskey="${argv[$i]}" ;;
            --bl30-aes-iv)
                bl30aesiv="${argv[$i]}" ;;
            --bl31-aes-iv)
                bl31aesiv="${argv[$i]}" ;;
            --bl32-aes-iv)
                bl32aesiv="${argv[$i]}" ;;
            --bl33-aes-iv)
                bl33aesiv="${argv[$i]}" ;;
            --kernel-key)
                kernelkey="${argv[$i]}" ;;
            --kernel-aes-key)
                kernelaeskey="${argv[$i]}" ;;
            --kernel-aes-iv)
                kernelaesiv="${argv[$i]}" ;;
            --fip-key)
                fipkey="${argv[$i]}" ;;
            --fip-arb-cvn)
                fip_arb_cvn="${argv[$i]}" ;;
            --ddrfw1)
                ddrfw1="${argv[$i]}"
                if [ "$ddrfw1" != "" ]; then
                    ddrfw1_sha=$TMP/ddrfw1.padded.sha
                    check_file ddrfw1 "$ddrfw1"
                    check_file ddrfw1_sha "$ddrfw1_sha"
                    trace "DDR FW1 $ddrfw1 $ddrfw1_sha"
                fi
                ;;
            --ddrfw2)
                ddrfw2="${argv[$i]}"
                if [ "$ddrfw2" != "" ]; then
                    ddrfw2_sha=$TMP/ddrfw2.padded.sha
                    check_file ddrfw2 "$ddrfw2"
                    check_file ddrfw2_sha "$ddrfw2_sha"
                    trace "DDR FW2 $ddrfw2 $ddrfw2_sha"
                fi
                ;;
            --ddrfw3)
                ddrfw3="${argv[$i]}"
                if [ "$ddrfw3" != "" ]; then
                    ddrfw3_sha=$TMP/ddrfw3.padded.sha
                    check_file ddrfw3 "$ddrfw3"
                    check_file ddrfw3_sha "$ddrfw3_sha"
                    trace "DDR FW3 $ddrfw3 $ddrfw3_sha"
                fi
                ;;
            --ddrfw4)
                ddrfw4="${argv[$i]}"
                if [ "$ddrfw4" != "" ]; then
                    ddrfw4_sha=$TMP/ddrfw4.padded.sha
                    check_file ddrfw4 "$ddrfw4"
                    check_file ddrfw4_sha "$ddrfw4_sha"
                    trace "DDR FW4 $ddrfw4 $ddrfw4_sha"
                fi
                ;;
            --ddrfw5)
                ddrfw5="${argv[$i]}"
                if [ "$ddrfw5" != "" ]; then
                    ddrfw5_sha=$TMP/ddrfw5.padded.sha
                    check_file ddrfw5 "$ddrfw5"
                    check_file ddrfw5_sha "$ddrfw5_sha"
                    trace "DDR FW5 $ddrfw5 $ddrfw5_sha"
                fi
                ;;
            --ddrfw6)
                ddrfw6="${argv[$i]}"
                if [ "$ddrfw6" != "" ]; then
                    ddrfw6_sha=$TMP/ddrfw6.padded.sha
                    check_file ddrfw6 "$ddrfw6"
                    check_file ddrfw6_sha "$ddrfw6_sha"
                    trace "DDR FW6 $ddrfw6 $ddrfw6_sha"
                fi
                ;;
            --ddrfw7)
                ddrfw7="${argv[$i]}"
                if [ "$ddrfw7" != "" ]; then
                    ddrfw7_sha=$TMP/ddrfw7.padded.sha
                    check_file ddrfw7 "$ddrfw7"
                    check_file ddrfw7_sha "$ddrfw7_sha"
                    trace "DDR FW7 $ddrfw7 $ddrfw7_sha"
                fi
                ;;
            --ddrfw8)
                ddrfw8="${argv[$i]}"
                if [ "$ddrfw8" != "" ]; then
                    ddrfw8_sha=$TMP/ddrfw8.padded.sha
                    check_file ddrfw8 "$ddrfw8"
                    check_file ddrfw8_sha "$ddrfw8_sha"
                    trace "DDR FW8 $ddrfw8 $ddrfw8_sha"
                fi
                ;;
            --ddrfw9)
                ddrfw9="${argv[$i]}"
                if [ "$ddrfw9" != "" ]; then
                    ddrfw9_sha=$TMP/ddrfw9.padded.sha
                    check_file ddrfw9 "$ddrfw9"
                    check_file ddrfw9_sha "$ddrfw9_sha"
                    trace "DDR FW9 $ddrfw9 $ddrfw9_sha"
                fi
                ;;
            --ddrfw10)
                ddrfw10="${argv[$i]}"
                if [ "$ddrfw10" != "" ]; then
                    ddrfw10_sha=$TMP/ddrfw10.padded.sha
                    check_file ddrfw10 "$ddrfw10"
                    check_file ddrfw10_sha "$ddrfw10_sha"
                    trace "DDR FW10 $ddrfw10 $ddrfw10_sha"
                fi
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
    if [ -n "$bl30key" ]; then
        check_file bl30key "$bl30key"
    fi
    check_file bl31key "$bl31key"
    check_file bl32key "$bl32key"
    check_file bl33key "$bl33key"
    check_file bl30aeskey "$bl30aeskey"
    check_file bl31aeskey "$bl31aeskey"
    check_file bl32aeskey "$bl32aeskey"
    check_file bl33aeskey "$bl33aeskey"
    check_file bl30aesiv "$bl30aesiv"
    check_file bl31aesiv "$bl31aesiv"
    check_file bl32aesiv "$bl32aesiv"
    check_file bl33aesiv "$bl33aesiv"
    if [ ! -z "$kernelkey" ]; then
        check_file kernelkey "$kernelkey"
    fi
    if [ ! -z "$kernelaeskey" ]; then
        check_file kernelaeskey "$kernelaeskey"
        check_file kernelaesiv "$kernelaesiv"
    fi
    check_file fipkey "$fipkey"

    if [ -z "$output" ]; then echo Error: Missing output file option -o ; exit 1; fi

    if [ -n "$bl30key" ]; then
        pem_to_pub $bl30key $TMP/bl30key.pub
    fi
    pem_to_pub $bl31key $TMP/bl31key.pub
    pem_to_pub $bl32key $TMP/bl32key.pub
    pem_to_pub $bl33key $TMP/bl33key.pub

    # Convert PEM key to rsa_public_key_t (precomputed RSA public key)
    if [ -n "$bl30key" ]; then
        pem_to_bin $TMP/bl30key.pub $TMP/bl30key.bin
    else
        head -c 1036 /dev/zero > "$TMP/bl30key.bin"
    fi
    pem_to_bin $TMP/bl31key.pub $TMP/bl31key.bin
    pem_to_bin $TMP/bl32key.pub $TMP/bl32key.bin
    pem_to_bin $TMP/bl33key.pub $TMP/bl33key.bin

    # dummy key/iv placeholders
    dd if=/dev/zero of="$TMP/zeroaeskey" bs=1 count=32 >& /dev/null
    dd if=/dev/zero of="$TMP/zeroaesiv" bs=1 count=16 >& /dev/null

    if [ ! -z "$kernelkey" ]; then
        pem_to_pub $kernelkey $TMP/kernelkey.pub
        pem_to_bin $TMP/kernelkey.pub $TMP/kernelkey.bin
    else
        # dummy kernelkey placeholder
        dd if=/dev/zero of=$TMP/kernelkey.bin bs=1036 count=1 &> /dev/null
    fi
    if [ -z "$kernelaeskey" ]; then
        # dummy key/iv placeholders
        kernelaeskey=$TMP/zeroaeskey
        kernelaesiv=$TMP/zeroaesiv
    fi

    # Overencrypt bl3x keys

    # Create header (without nonce) and add keys
    local argv=("$@")
    ${TOOL_PATH}/sign-boot-c1 --create-fip-header \
        --bl30 $bl30 --bl30-key $TMP/bl30key.bin \
        --bl30-aes-key $bl30aeskey --bl30-aes-iv $bl30aesiv \
        --bl31 $bl31 --bl31-key $TMP/bl31key.bin \
        --bl31-aes-key $bl31aeskey --bl31-aes-iv $bl31aesiv \
        --bl32 $bl32 --bl32-key $TMP/bl32key.bin \
        --bl32-aes-key $bl32aeskey --bl32-aes-iv $bl32aesiv \
        --bl33 $bl33 --bl33-key $TMP/bl33key.bin \
        --bl33-aes-key $bl33aeskey --bl33-aes-iv $bl33aesiv \
        --bl31-info $bl31_info --bl32-info $bl32_info \
        --kernel-key $TMP/kernelkey.bin \
        --kernel-aes-key $kernelaeskey --kernel-aes-iv $kernelaesiv \
        --arb-cvn $fip_arb_cvn \
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
    # pad to (sizeof(fip hdr) - signature[keylen] - nonce[16])
    local keylen=$(get_pem_key_len $fipkey)
    pad_file $TMP/fip.hdr $(( (16*1024) - $keylen - 16 ))

    # Sign header
    openssl dgst -sha256 -sign $fipkey -out $TMP/fip.hdr.sig $TMP/fip.hdr

    # Create nonce
    dd if=/dev/urandom of=$TMP/nonce.bin bs=16 count=1 >& /dev/null

    # Combine nonce + hdr + sig
    cat $TMP/nonce.bin $TMP/fip.hdr $TMP/fip.hdr.sig > $output
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

sign_enc_ddrfw() {
    local name="$1"
    local ddrfw="$2"
    local ddrfwalignment=16384
    local encryptFIPbl3x="$3"
    local aeskey="$4"
    local ddrfwsha="$5"
    local ddrfwout="$6"
    local ddrfwcombined="$7"

    check_file $name "$ddrfw"

    # 16K align
    size=$(aligned_size "$ddrfw" 96 $ddrfwalignment)
    trace "sizeof($ddrfw) $(wc -c < ${ddrfw}) --> $size"

    dd if=$ddrfw of=$TMP/ddrfw.hdr bs=1 count=96 >& /dev/null
    dd if=$ddrfw of=$TMP/ddrfw.padded bs=1 skip=96 >& /dev/null
    pad_file $TMP/ddrfw.padded $size

    openssl dgst -sha256 -binary $TMP/ddrfw.padded > $ddrfwsha

    if $encryptFIPbl3x; then
        dd if=/dev/zero of=$TMP/zeroiv bs=16 count=1 >& /dev/null
        internal_encrypt $TMP/ddrfw.padded $TMP/ddrfw.padded.enc $aeskey $TMP/zeroiv
        mv $TMP/ddrfw.padded.enc $TMP/ddrfw.padded
    fi

    dd if=$TMP/ddrfw.padded of=$ddrfwcombined oflag=append conv=notrunc >& /dev/null

    cat $TMP/ddrfw.hdr $TMP/ddrfw.padded > $ddrfwout
}

# Sign bl2 or bl30 header with root key
sign_bl2_hdr() {
    local rootkey=""
    local rootkeyidx="0"
    local rootkey0=""
    local rootkey1=""
    local rootkey2=""
    local rootkey3=""
    local bl2key0=""
    local bl2key1=""
    local bl2key2=""
    local bl2key3=""
    local bl2key=""
    local output=""
    local arb_cvn=""
    local marketid="0"
    local sigver=""
    local keyhashver=""
    local argv=("$@")
    local i=0
    local bl2size=65536
    local soc="g12a"

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --soc)
                soc="${argv[$i]}" ;;
            --root-key)
                rootkey="${argv[$i]}" ;;
            --root-key-idx)
                rootkeyidx="${argv[$i]}" ;;
            --root-key-0)
                rootkey0="${argv[$i]}" ;;
            --root-key-1)
                rootkey1="${argv[$i]}" ;;
            --root-key-2)
                rootkey2="${argv[$i]}" ;;
            --root-key-3)
                rootkey3="${argv[$i]}" ;;
            --bl2-key-0)
                bl2key0="${argv[$i]}" ;;
            --bl2-key-1)
                bl2key1="${argv[$i]}" ;;
            --bl2-key-2)
                bl2key2="${argv[$i]}" ;;
            --bl2-key-3)
                bl2key3="${argv[$i]}" ;;
            --bl2-key)
                bl2key="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            --bl2-size)
                bl2size="${argv[$i]}" ;;
            --arb-cvn)
                arb_cvn="${argv[$i]}" ;;
            --marketid)
                marketid="${argv[$i]}" ;;
            --sig-ver)
                sigver="${argv[$i]}" ;;
            --key-hash-ver)
                keyhashver="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    set_soc "$soc"

    check_file rootkey "$rootkey"
    check_file rootkey0 "$rootkey0"
    check_file rootkey1 "$rootkey1"
    check_file rootkey2 "$rootkey2"
    check_file rootkey3 "$rootkey3"
    check_file bl2key0 "$bl2key0"
    check_file bl2key1 "$bl2key1"
    check_file bl2key2 "$bl2key2"
    check_file bl2key3 "$bl2key3"
    check_file bl2key "$bl2key"
    if [ -z "$output" ]; then echo Error: Missing output file option -o; exit 1; fi
    if [ -z "$arb_cvn" ]; then
        arb_cvn="0"
    else
        if [ $(( $arb_cvn )) -lt 0 ] || [ $(( $arb_cvn )) -gt 255 ]; then
            echo Error: invalid bl2 arb cvn \"$arb_cvn\"
            exit 1
        fi
    fi
    if [ -z "$sigver" ]; then
        sigver=1
    fi
    if [ -z "$keyhashver" ]; then
        keyhashver=$keyhashver_min
    fi
    if [ "$keyhashver" -lt $keyhashver_min ]; then
        echo Error: bad key hash ver; exit 1
    fi
    if [ "$keyhashver" != "2" ] && [ "$keyhashver" != "3" ]; then
        echo Error: bad key hash ver; exit 1
    fi
    trace "BL2 size specified $bl2size"

    local keylenbytes=$(get_pem_key_len "$rootkey")
    local keylen=$(( $keylenbytes * 8 ))

    # Convert to public PEM using openssl in case of password protected keys
    pem_to_pub "$rootkey"  "$TMP/rootkey.pub"
    cp "$rootkey0" "$TMP/rootkey0.pub"
    cp "$rootkey1" "$TMP/rootkey1.pub"
    cp "$rootkey2" "$TMP/rootkey2.pub"
    cp "$rootkey3" "$TMP/rootkey3.pub"
    cp "$bl2key0" "$TMP/bl2key0.pub"
    cp "$bl2key1" "$TMP/bl2key1.pub"
    cp "$bl2key2" "$TMP/bl2key2.pub"
    cp "$bl2key3" "$TMP/bl2key3.pub"
    pem_to_pub "$bl2key"  "$TMP/bl2key.pub"

    # Convert PEM key to rsa_public_key_t (precomputed RSA public key)
    if [ "$keyhashver" == "3" ]; then
        enable_n0inv128=True
    else
        enable_n0inv128=False
    fi

    pem_to_bin "$TMP/rootkey.pub" "$TMP/rootkey.bin" $enable_n0inv128
    pem_to_bin "$TMP/rootkey0.pub" "$TMP/rootkey0.bin" $enable_n0inv128
    pem_to_bin "$TMP/rootkey1.pub" "$TMP/rootkey1.bin" $enable_n0inv128
    pem_to_bin "$TMP/rootkey2.pub" "$TMP/rootkey2.bin" $enable_n0inv128
    pem_to_bin "$TMP/rootkey3.pub" "$TMP/rootkey3.bin" $enable_n0inv128
    pem_to_bin "$TMP/bl2key0.pub" "$TMP/bl2key0.bin" $enable_n0inv128
    pem_to_bin "$TMP/bl2key1.pub" "$TMP/bl2key1.bin" $enable_n0inv128
    pem_to_bin "$TMP/bl2key2.pub" "$TMP/bl2key2.bin" $enable_n0inv128
    pem_to_bin "$TMP/bl2key3.pub" "$TMP/bl2key3.bin" $enable_n0inv128
    pem_to_bin "$TMP/bl2key.pub" "$TMP/bl2key.bin" $enable_n0inv128

    # hash of keys
    hash_rsa_bin $keyhashver "$TMP/rootkey.bin" $keylen "$TMP/rootkey.sha"
    hash_rsa_bin $keyhashver "$TMP/rootkey0.bin" $keylen "$TMP/rootkey0.sha"
    hash_rsa_bin $keyhashver "$TMP/rootkey1.bin" $keylen "$TMP/rootkey1.sha"
    hash_rsa_bin $keyhashver "$TMP/rootkey2.bin" $keylen "$TMP/rootkey2.sha"
    hash_rsa_bin $keyhashver "$TMP/rootkey3.bin" $keylen "$TMP/rootkey3.sha"
    hash_rsa_bin $keyhashver "$TMP/bl2key0.bin" $keylen "$TMP/bl2key0.sha"
    hash_rsa_bin $keyhashver "$TMP/bl2key1.bin" $keylen "$TMP/bl2key1.sha"
    hash_rsa_bin $keyhashver "$TMP/bl2key2.bin" $keylen "$TMP/bl2key2.sha"
    hash_rsa_bin $keyhashver "$TMP/bl2key3.bin" $keylen "$TMP/bl2key3.sha"
    hash_rsa_bin $keyhashver "$TMP/bl2key.bin" $keylen "$TMP/bl2key.sha"

    # Create bl2 header
    ${CREATE_HEADER_TOOL} --create-bl2-header \
            -r $keylen \
            --root-key "$TMP/rootkey.bin" \
            --root-key-idx $rootkeyidx \
            --root-key-sha-0 "$TMP/rootkey0.sha" \
            --root-key-sha-1 "$TMP/rootkey1.sha" \
            --root-key-sha-2 "$TMP/rootkey2.sha" \
            --root-key-sha-3 "$TMP/rootkey3.sha" \
            --bl2-key-sha-0 "$TMP/bl2key0.sha" \
            --bl2-key-sha-1 "$TMP/bl2key1.sha" \
            --bl2-key-sha-2 "$TMP/bl2key2.sha" \
            --bl2-key-sha-3 "$TMP/bl2key3.sha" \
            --bl2-key-idx 0 \
            --bl2-key "$TMP/bl2key.bin" \
            --marketid $marketid \
            --bl2-size $bl2size \
            -o "$TMP/bl2.hdr"

    # Add arb cvn
    if [ ! -z "$arb_cvn" ]; then
        printf %02x $arb_cvn | xxd -r -p > "$TMP/bl2cvn"
        local filesize=$(wc -c < "${TMP}/bl2cvn")
        if [ $filesize -ne 1 ]; then echo Error: cvn error ; exit 1; fi
        # to byteoffset 16+12 - bl2VerMajor
        dd if="$TMP/bl2cvn" of="$TMP/bl2.hdr" bs=1 seek=28 count=1 \
            conv=notrunc >& /dev/null
        # to 2nd aml block header, 0x8d8 + 12
        dd if="$TMP/bl2cvn" of="$TMP/bl2.hdr" bs=1 seek=2276 count=1 \
            conv=notrunc >& /dev/null
    fi

    # Sign first part with root key
    # aml block header - first 64:
    dd if="$TMP/bl2.hdr" of="$TMP/sigdata" bs=1 skip=16 count=64 >& /dev/null
    local chkstart=$(( 16 + 64 + keylenbytes ))
    local chksize=$(( 2248 - 64 - keylenbytes ))
    # - from nChkStart to end of bl2_key_array
    dd if="$TMP/bl2.hdr" of="$TMP/sigdata" bs=1 skip=$chkstart count=$chksize \
        oflag=append conv=notrunc >& /dev/null
    openssl dgst -sha256 -sign "$rootkey" -out "$TMP/sigdata.sig" "$TMP/sigdata"

    # Replace signature placeholder
    dd if="$TMP/sigdata.sig" of="$TMP/bl2.hdr" bs=1 seek=80 count=$keylenbytes \
        conv=notrunc >& /dev/null

    mv "$TMP/bl2.hdr" "$output"
}

# input bl2, bl30/31/32/33/kernel {.bin, rsa key, aes key, aes iv}
create_signed_bl() {
    local bl2=""
    local bl2hdr=""
    local bl30hdr=""
    local bl30=""
    local bl31=""
    local bl32=""
    local bl33=""
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
    local rootkey=""
    local rootkeyidx="0"
    local rootkey0=""
    local rootkey1=""
    local rootkey2=""
    local rootkey3=""
    local bl2key0=""
    local bl2key1=""
    local bl2key2=""
    local bl2key3=""
    local bl2key=""
    local bl30key=""
    local bl31key=""
    local bl32key=""
    local bl33key=""
    local bl2aeskey=""
    local bl30aeskey=""
    local bl31aeskey=""
    local bl32aeskey=""
    local bl33aeskey=""
    local bl2aesiv=""
    local bl30aesiv=""
    local bl31aesiv=""
    local bl32aesiv=""
    local bl33aesiv=""
    local kernel=""
    local kernelkey=""
    local kernelaeskey=""
    local kernelaesiv=""
    local fipkey=""
    local fipaeskey=""
    local fipaesiv=""
    local encryptbl2=true
    local encryptFIPbl3x=false
    local bl2_arb_cvn="0"
    local fip_arb_cvn="0"
    local bl30_arb_cvn="0"
    local bl31_arb_cvn="0"
    local bl32_arb_cvn="0"
    local bl33_arb_cvn="0"
    local output=""
    local sigver=""
    local keyhashver=""
    local marketid="0"
    local argv=("$@")
    local i=0
    local size=0
    local ddrfwalignment=16384
    local soc="g12a"

    touch $TMP/ddr.fw.bin

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
            --bl30)
                bl30="${argv[$i]}" ;;
            --bl31)
                bl31="${argv[$i]}" ;;
            --bl32)
                bl32="${argv[$i]}" ;;
            --bl33)
                bl33="${argv[$i]}" ;;
            --bl2-hdr)
                bl2hdr="${argv[$i]}" ;;
            --bl30-hdr)
                bl30hdr="${argv[$i]}" ;;
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
            --root-key)
                rootkey="${argv[$i]}" ;;
            --root-key-idx)
                rootkeyidx="${argv[$i]}" ;;
            --root-key-0)
                rootkey0="${argv[$i]}" ;;
            --root-key-1)
                rootkey1="${argv[$i]}" ;;
            --root-key-2)
                rootkey2="${argv[$i]}" ;;
            --root-key-3)
                rootkey3="${argv[$i]}" ;;
            --bl2-key-0)
                bl2key0="${argv[$i]}" ;;
            --bl2-key-1)
                bl2key1="${argv[$i]}" ;;
            --bl2-key-2)
                bl2key2="${argv[$i]}" ;;
            --bl2-key-3)
                bl2key3="${argv[$i]}" ;;
            --bl2-key)
                bl2key="${argv[$i]}" ;;
            --bl30-key)
                bl30key="${argv[$i]}" ;;
            --bl31-key)
                bl31key="${argv[$i]}" ;;
            --bl32-key)
                bl32key="${argv[$i]}" ;;
            --bl33-key)
                bl33key="${argv[$i]}" ;;
            --bl2-aes-key)
                bl2aeskey="${argv[$i]}" ;;
            --bl30-aes-key)
                bl30aeskey="${argv[$i]}" ;;
            --bl31-aes-key)
                bl31aeskey="${argv[$i]}" ;;
            --bl32-aes-key)
                bl32aeskey="${argv[$i]}" ;;
            --bl33-aes-key)
                bl33aeskey="${argv[$i]}" ;;
            --bl2-aes-iv)
                bl2aesiv="${argv[$i]}" ;;
            --bl30-aes-iv)
                bl30aesiv="${argv[$i]}" ;;
            --bl31-aes-iv)
                bl31aesiv="${argv[$i]}" ;;
            --bl32-aes-iv)
                bl32aesiv="${argv[$i]}" ;;
            --bl33-aes-iv)
                bl33aesiv="${argv[$i]}" ;;
            --kernel-key)
                kernelkey="${argv[$i]}" ;;
            --kernel-aes-key)
                kernelaeskey="${argv[$i]}" ;;
            --kernel-aes-iv)
                kernelaesiv="${argv[$i]}" ;;
            --fip-key)
                fipkey="${argv[$i]}" ;;
            --fip-aes-key)
                fipaeskey="${argv[$i]}" ;;
            --fip-aes-iv)
                fipaesiv="${argv[$i]}" ;;
            -e)
                if [ "${argv[$i]}" == "FIPbl3x" ]; then
                    # encrypted bl2, FIP, bl3x
                    encryptFIPbl3x=true
                fi
                if [ "${argv[$i]}" == "FIPbl3xonly" ]; then
                    # unencrypted bl2.  encrypted FIP, bl3x
                    encryptFIPbl3x=true
                    encryptbl2=false
                fi
                if [ "${argv[$i]}" == "none" ]; then
                    # unencrypted bl2, FIP, bl3x
                    encryptFIPbl3x=false
                    encryptbl2=false
                fi
                ;;
            --bl2-arb-cvn)
                bl2_arb_cvn="${argv[$i]}" ;;
            --fip-arb-cvn)
                fip_arb_cvn="${argv[$i]}" ;;
            --bl30-arb-cvn)
                bl30_arb_cvn="${argv[$i]}" ;;
            --bl31-arb-cvn)
                bl31_arb_cvn="${argv[$i]}" ;;
            --bl32-arb-cvn)
                bl32_arb_cvn="${argv[$i]}" ;;
            --bl33-arb-cvn)
                bl33_arb_cvn="${argv[$i]}" ;;
            --sig-ver)
                sigver="${argv[$i]}" ;;
            --key-hash-ver)
                keyhashver="${argv[$i]}" ;;
            --marketid)
                marketid="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1 ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    set_soc "$soc"

    if [ -z "$keyhashver" ]; then
        keyhashver=$keyhashver_min
    fi
    if [ "$keyhashver" -lt $keyhashver_min ]; then
        echo Error: bad key hash ver; exit 1
    fi

    check_file bl2 "$bl2"
    if $bl30_required || [ "$bl30" != "" ]; then
        check_file bl30 "$bl30"
    fi
    check_file bl31 "$bl31"
    if [ "$bl32" != "" ]; then
        check_file bl32 "$bl32"
    fi
    check_file bl33 "$bl33"

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
    if [ "$bl2hdr" != "" ]; then
        check_file bl2hdr "$bl2hdr"
        if $bl30_required || [ "$bl30hdr" != "" ]; then
            check_file bl30hdr "$bl30hdr"
        fi

        if [ "$rootkey" != "" ]; then
            echo Presigned bl2 header given. Root-key not necessary
            exit 1;
        fi
    else
        check_file rootkey "$rootkey"
        if [ "$rootkeyidx" -lt 0 ] || [ "$rootkeyidx" -gt 3 ]; then
            echo "Error invalid root-key-idx: $rootkeyidx.";
            exit 1;
        fi
        check_file rootkey0 "$rootkey0"
        check_file rootkey1 "$rootkey1"
        check_file rootkey2 "$rootkey2"
        check_file rootkey3 "$rootkey3"
    fi
    check_file bl2key "$bl2key"
    if [ "$bl2key0" == "" ]; then
        bl2key0=$bl2key
    fi
    if [ "$bl2key1" == "" ]; then
        bl2key1=$bl2key
    fi
    if [ "$bl2key2" == "" ]; then
        bl2key2=$bl2key
    fi
    if [ "$bl2key3" == "" ]; then
        bl2key3=$bl2key
    fi
    check_file bl2key0 "$bl2key0"
    check_file bl2key1 "$bl2key1"
    check_file bl2key2 "$bl2key2"
    check_file bl2key3 "$bl2key3"
    if $bl30_required || [ "$bl30key" != "" ]; then
        check_file bl30key "$bl30key"
    fi
    check_file bl31key "$bl31key"
    check_file bl32key "$bl32key"
    check_file bl33key "$bl33key"

    if $encryptbl2; then
        check_file bl2aeskey "$bl2aeskey"
    fi
    if $encryptFIPbl3x; then
        if ! $separate_fip_aes; then
            if [ -n "$fipaeskey" ] && [ "$fipaeskey" != "$bl2aeskey" ]; then
                echo "Error: Separate fip-aes-key not supported by SoC $soc"
                exit 1
            fi
        fi
        if [ -z "$bl2aeskey" ] && [ -z "$fipaeskey" ]; then
            echo Error: bl2-aes-key or fip-aes-key must be given
            exit 1
        fi
        check_opt_file bl2-aes-key 32 "$bl2aeskey"
    fi
    if $encryptbl2 || $encryptFIPbl3x; then
        if [ -n "$bl2aesiv" ]; then
            check_file bl2aesiv "$bl2aesiv"
        else
            dd if=/dev/urandom of=$TMP/bl2aesiv bs=16 count=1 \
                conv=notrunc >& /dev/null
            bl2aesiv=$TMP/bl2aesiv
        fi
    fi
    if $encryptFIPbl3x; then
        if [ -z "$fipaeskey" ]; then
            if [ -n "$fipaesiv" ]; then
                echo Error: Mix of bl2-aes-key and fip-aes-iv for FIP encryption is not supported by this tool
                exit 1
            fi
            fipaeskey="$bl2aeskey"
            fipaesiv="$bl2aesiv"
        else
            check_file fip-aes-key "$fipaeskey"
            check_file fip-aes-iv "$fipaeskey"
        fi

        if $bl30_required || [ "$bl30aeskey" != "" ]; then
            check_file bl30aeskey "$bl30aeskey"
            check_file bl30aesiv "$bl30aesiv"
        fi
        check_file bl31aeskey "$bl31aeskey"
        check_file bl32aeskey "$bl32aeskey"
        check_file bl33aeskey "$bl33aeskey"
        check_file bl31aesiv "$bl31aesiv"
        check_file bl32aesiv "$bl32aesiv"
        check_file bl33aesiv "$bl33aesiv"
        check_file kernelaeskey "$kernelaeskey"
        check_file kernelaesiv "$kernelaesiv"
    fi
    if [ ! -z "$kernelkey" ]; then
        check_file kernelkey "$kernelkey"
    fi
    if [ ! -z "$kernelaeskey" ]; then
        check_file kernelaeskey "$kernelaeskey"
        check_file kernelaesiv "$kernelaesiv"
    fi

    check_file fipkey "$fipkey"
    if [ -z "$bl2_arb_cvn" ]; then
        bl2_arb_cvn="0"
    fi
    if [ -z "$sigver" ]; then
        sigver=1
    fi
    if [ "$keyhashver" != "2" ] && [ "$keyhashver" != "3" ]; then
        echo Error: bad key hash ver; exit 1
    fi

    if [ -z "$output" ]; then echo Error: Missing output file option -o ; exit 1; fi

    local bl2hdr_i="$bl2hdr"
    if [ "$bl2hdr" == "" ]; then
        bl2hdr_i="$TMP/bl2.hdri"
        # No pre-signed header given. Create one.
        sign_bl2_hdr -o "$bl2hdr_i" \
            --soc "$soc" \
            --root-key "$rootkey" --root-key-idx "$rootkeyidx" \
            --root-key-0 "$rootkey0" --root-key-1 "$rootkey1" \
            --root-key-2 "$rootkey2" --root-key-3 "$rootkey3" \
            --bl2-key-0 "$bl2key0" --bl2-key-1 "$bl2key1" \
            --bl2-key-2 "$bl2key2" --bl2-key-3 "$bl2key3" \
            --bl2-key "$bl2key" \
            --bl2-size 65536 \
            --marketid $marketid \
            --arb-cvn $bl2_arb_cvn \
            --sig-ver $sigver --key-hash-ver $keyhashver
    fi

    # Sign/encrypt BL2 payload
    sign_bl2 -i "$bl2" -o "$TMP/bl2.bin.out" \
        --bl2-hdr "$bl2hdr_i" \
        --bl2-key "$bl2key" \
        --bl2-size 65536 \
        $( [ -n "$bl2aesiv" ] && echo -n "--iv $bl2aesiv" ) \
        --sig-ver $sigver --key-hash-ver $keyhashver

    if $encryptbl2; then
        dd if=/dev/zero of=$TMP/zeroiv bs=16 count=1 >& /dev/null
        internal_encrypt $TMP/bl2.bin.out $TMP/bl2.bin.sig $bl2aeskey $TMP/zeroiv
    else
        cp $TMP/bl2.bin.out $TMP/bl2.bin.sig
    fi

    # Sign/encrypt BL30 payload using bl2 functions
    if ! $bl30_required && [ "$bl30" == "" ]; then
        touch "$TMP/bl30.bin.sig"
    else
        local bl30hdr_i="$bl30hdr"
        if [ "$bl30hdr" == "" ]; then
            bl30hdr_i="$TMP/bl30.hdri"
            # No pre-signed header given. Create one.
            sign_bl2_hdr -o "$bl30hdr_i" \
                --soc "$soc" \
                --root-key "$rootkey" --root-key-idx "$rootkeyidx" \
                --root-key-0 "$rootkey0" --root-key-1 "$rootkey1" \
                --root-key-2 "$rootkey2" --root-key-3 "$rootkey3" \
                --bl2-key-0 "$bl30key" --bl2-key-1 "$bl30key" \
                --bl2-key-2 "$bl30key" --bl2-key-3 "$bl30key" \
                --bl2-key "$bl30key" \
                --bl2-size 58368 \
                --arb-cvn $bl30_arb_cvn \
                --sig-ver $sigver --key-hash-ver $keyhashver
        fi
        sign_bl2 -i $bl30 -o $TMP/bl30.bin.out \
            --bl2-hdr "$bl30hdr_i" \
            --bl2-key $bl30key \
            --bl2-size 58368 \
            $( [ -n "$bl30aesiv" ] && echo -n "--iv $bl30aesiv" ) \
            --sig-ver $sigver --key-hash-ver $keyhashver

        if $encryptbl2; then
            # bl30 encryption enable is based on a fuse bit, so use the 'encryptbl2' flag
            dd if=/dev/zero of=$TMP/zeroiv bs=16 count=1 >& /dev/null
            #internal_encrypt $TMP/bl30.bin.out $TMP/bl30.bin.sig $bl2aeskey $TMP/zeroiv
            # scp needs to decrypt in 2KB chunks:
            if [ -f $TMP/bl30.bin.sig ]; then rm -f $TMP/bl30.bin.sig ; fi
            for i in $(seq 0 $(( 58368 / 2048 )) ); do
                dd if=$TMP/bl30.bin.out of=$TMP/pt skip=$i bs=2048 count=1 >& /dev/null
                internal_encrypt $TMP/pt $TMP/ct $bl2aeskey $TMP/zeroiv
                cat $TMP/ct >> $TMP/bl30.bin.sig
            done
            if [ $(wc -c < $TMP/bl30.bin.sig) -ne 58368 ]; then
                echo Internal error, bl30.bin.sig size
                exit 1;
            fi
        else
            cp $TMP/bl30.bin.out $TMP/bl30.bin.sig
        fi
    fi

    if [ "$ddrfw1" != "" ]; then
        ddrfw1_sha=$TMP/ddrfw1.padded.sha
        ddrfw1_out=$TMP/ddrfw1.hdr.padded
        sign_enc_ddrfw ddrfw1 "$ddrfw1" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw1_sha" "$ddrfw1_out" "$TMP/ddr.fw.bin"
        ddrfw1=$ddrfw1_out
    fi
    if [ "$ddrfw2" != "" ]; then
        ddrfw2_sha=$TMP/ddrfw2.padded.sha
        ddrfw2_out=$TMP/ddrfw2.hdr.padded
        sign_enc_ddrfw ddrfw2 "$ddrfw2" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw2_sha" "$ddrfw2_out" "$TMP/ddr.fw.bin"
        ddrfw2=$ddrfw2_out
    fi
    if [ "$ddrfw3" != "" ]; then
        ddrfw3_sha=$TMP/ddrfw3.padded.sha
        ddrfw3_out=$TMP/ddrfw3.hdr.padded
        sign_enc_ddrfw ddrfw3 "$ddrfw3" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw3_sha" "$ddrfw3_out" "$TMP/ddr.fw.bin"
        ddrfw3=$ddrfw3_out
    fi
    if [ "$ddrfw4" != "" ]; then
        ddrfw4_sha=$TMP/ddrfw4.padded.sha
        ddrfw4_out=$TMP/ddrfw4.hdr.padded
        sign_enc_ddrfw ddrfw4 "$ddrfw4" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw4_sha" "$ddrfw4_out" "$TMP/ddr.fw.bin"
        ddrfw4=$ddrfw4_out
    fi
    if [ "$ddrfw5" != "" ]; then
        ddrfw5_sha=$TMP/ddrfw5.padded.sha
        ddrfw5_out=$TMP/ddrfw5.hdr.padded
        sign_enc_ddrfw ddrfw5 "$ddrfw5" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw5_sha" "$ddrfw5_out" "$TMP/ddr.fw.bin"
        ddrfw5=$ddrfw5_out
    fi
    if [ "$ddrfw6" != "" ]; then
        ddrfw6_sha=$TMP/ddrfw6.padded.sha
        ddrfw6_out=$TMP/ddrfw6.hdr.padded
        sign_enc_ddrfw ddrfw6 "$ddrfw6" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw6_sha" "$ddrfw6_out" "$TMP/ddr.fw.bin"
        ddrfw6=$ddrfw6_out
    fi
    if [ "$ddrfw7" != "" ]; then
        ddrfw7_sha=$TMP/ddrfw7.padded.sha
        ddrfw7_out=$TMP/ddrfw7.hdr.padded
        sign_enc_ddrfw ddrfw7 "$ddrfw7" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw7_sha" "$ddrfw7_out" "$TMP/ddr.fw.bin"
        ddrfw7=$ddrfw7_out
    fi
    if [ "$ddrfw8" != "" ]; then
        ddrfw8_sha=$TMP/ddrfw8.padded.sha
        ddrfw8_out=$TMP/ddrfw8.hdr.padded
        sign_enc_ddrfw ddrfw8 "$ddrfw8" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw8_sha" "$ddrfw8_out" "$TMP/ddr.fw.bin"
        ddrfw8=$ddrfw8_out
    fi
    if [ "$ddrfw9" != "" ]; then
        ddrfw9_sha=$TMP/ddrfw9.padded.sha
        ddrfw9_out=$TMP/ddrfw9.hdr.padded
        sign_enc_ddrfw ddrfw9 "$ddrfw9" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw9_sha" "$ddrfw9_out" "$TMP/ddr.fw.bin"
        ddrfw9=$ddrfw9_out
    fi
    if [ "$ddrfw10" != "" ]; then
        ddrfw10_sha=$TMP/ddrfw10.padded.sha
        ddrfw10_out=$TMP/ddrfw10.hdr.padded
        sign_enc_ddrfw ddrfw10 "$ddrfw10" "$encryptFIPbl3x" "$fipaeskey" "$ddrfw10_sha" "$ddrfw10_out" "$TMP/ddr.fw.bin"
        ddrfw10=$ddrfw10_out
    fi

    convert_img_to_bin $bl31 $TMP/bl31.bin
    if [ "$bl32" != "" ]; then
        convert_img_to_bin $bl32 $TMP/bl32.bin
    fi

    sign_bl3x -i $TMP/bl31.bin -o $TMP/bl31.bin.sig -k $bl31key --stage bl31 -v $bl31_arb_cvn \
        $( $encryptFIPbl3x && echo -n "-a $bl31aeskey --iv $bl31aesiv" )
    if [ "$bl32" != "" ]; then
        sign_bl3x -i $TMP/bl32.bin -o $TMP/bl32.bin.sig -k $bl32key --stage bl32 -v $bl32_arb_cvn \
            $( $encryptFIPbl3x && echo -n "-a $bl32aeskey --iv $bl32aesiv" )
    fi
    sign_bl3x -i $bl33         -o $TMP/bl33.bin.sig -k $bl33key --stage bl33 -v $bl33_arb_cvn \
        $( $encryptFIPbl3x && echo -n "-a $bl33aeskey --iv $bl33aesiv" )

    if ! $encryptFIPbl3x || (! $bl30_required && [ "$bl30" == "" ]); then
        dd if=/dev/zero of=$TMP/zerokey bs=32 count=1 >& /dev/null
        dd if=/dev/zero of=$TMP/zeroiv bs=16 count=1 >& /dev/null
    fi
    if ! $encryptFIPbl3x; then
        # create_fip currently requires bl3x aes key/iv to be inserted into fip
        # header. Create dummy data for it
        bl30aeskey=$TMP/zerokey
        bl31aeskey=$TMP/zerokey
        bl32aeskey=$TMP/zerokey
        bl33aeskey=$TMP/zerokey
        kernelaeskey=$TMP/zerokey
        bl30aesiv=$TMP/zeroiv
        bl31aesiv=$TMP/zeroiv
        bl32aesiv=$TMP/zeroiv
        bl33aesiv=$TMP/zeroiv
        kernelaesiv=$TMP/zeroiv
    fi

    bl30_key_arg="--bl30-key     $bl30key"
    if ! $bl30_required; then
        if [ "$bl30aeskey" == "" ]; then
            bl30aeskey=$TMP/zerokey
            bl30aesiv=$TMP/zeroiv
        fi
        if [ "$bl30key" == "" ]; then
            bl30_key_arg=
        fi
    fi

    if [ "$bl32" == "" ]; then
        rm -f $TMP/bl32.bin.sig
        touch $TMP/bl32.bin.sig
        # use bl31 as dummy bl32 img info
        cp $bl31 $TMP/bl32.img-info
        bl32info=$TMP/bl32.img-info
    else
        bl32info=$bl32
    fi

    create_fip \
        --bl30     $TMP/bl30.bin.sig \
        $bl30_key_arg \
        --bl30-aes-key $bl30aeskey \
        --bl30-aes-iv  $bl30aesiv \
        --bl31     $TMP/bl31.bin.sig \
        --bl31-info $bl31 \
        --bl31-key     $bl31key \
        --bl31-aes-key $bl31aeskey \
        --bl31-aes-iv  $bl31aesiv \
        --bl32     $TMP/bl32.bin.sig \
        --bl32-info $bl32info \
        --bl32-key     $bl32key \
        --bl32-aes-key $bl32aeskey \
        --bl32-aes-iv  $bl32aesiv \
        --bl33     $TMP/bl33.bin.sig \
        --bl33-key     $bl33key \
        --bl33-aes-key $bl33aeskey \
        --bl33-aes-iv  $bl33aesiv \
        $( [ ! -z "$kernelkey" ] && \
            echo -n "--kernel-key $kernelkey " ) \
        $( [ ! -z "$kernelaeskey" ] && \
            echo -n "--kernel-aes-key $kernelaeskey \
                     --kernel-aes-iv  $kernelaesiv " ) \
        --fip-key $fipkey \
        --fip-arb-cvn $fip_arb_cvn \
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

    if $encryptFIPbl3x; then
        internal_encrypt $TMP/fip.hdr.out $TMP/fip.hdr.sig $fipaeskey $fipaesiv
    else
        cp $TMP/fip.hdr.out $TMP/fip.hdr.sig
    fi

    # TODO:
    # Call fixup script to create blxx_new.bin
    #package

    cat $TMP/bl2.bin.sig $TMP/fip.hdr.sig $TMP/ddr.fw.bin $TMP/bl30.bin.sig $TMP/bl31.bin.sig \
        $TMP/bl32.bin.sig $TMP/bl33.bin.sig > $output

    echo
    echo Created signed bootloader $output successfully
}

create_signed_bl40() {
    local bl40hdr=""
    local bl40=""
    local rootkey=""
    local rootkeyidx="0"
    local rootkey0=""
    local rootkey1=""
    local bl40key=""
    local bl40aeskey=""
    local bl40aesiv=""
    local encryptFIPbl3x=false
    local bl40_arb_cvn="0"
    local output=""
    local sigver=""
    local keyhashver=""
    local marketid="0"
    local argv=("$@")
    local i=0
    local size=0
    local soc="g12a"

    # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --soc)
                soc="${argv[$i]}" ;;
            --root-key)
                rootkey="${argv[$i]}" ;;
            --root-key-idx)
                rootkeyidx="${argv[$i]}" ;;
            --root-key-0)
                rootkey0="${argv[$i]}" ;;
            --root-key-1)
                rootkey1="${argv[$i]}" ;;
            --bl40)
                bl40="${argv[$i]}" ;;
            --bl40-hdr)
                bl40hdr="${argv[$i]}" ;;
            --bl40-key)
                bl40key="${argv[$i]}" ;;
            --bl40-aes-key)
                bl40aeskey="${argv[$i]}" ;;
            --bl40-aes-iv)
                bl40aesiv="${argv[$i]}" ;;
            -e)
                if [ "${argv[$i]}" == "FIPbl3x" ]; then
                    # encrypted bl2, FIP, bl3x
                    encryptFIPbl3x=true
                fi
                if [ "${argv[$i]}" == "none" ]; then
                    # unencrypted bl2, FIP, bl3x
                    encryptFIPbl3x=false
                fi
                ;;
            --bl40-arb-cvn)
                bl40_arb_cvn="${argv[$i]}" ;;
            --sig-ver)
                sigver="${argv[$i]}" ;;
            --key-hash-ver)
                keyhashver="${argv[$i]}" ;;
            --marketid)
                marketid="${argv[$i]}" ;;
            -o)
                output="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1 ;;
        esac
        i=$((i + 1))
    done
    # Verify args
    set_soc "$soc"

    if [ -z "$keyhashver" ]; then
        keyhashver=$keyhashver_min
    fi
    if [ "$keyhashver" -lt $keyhashver_min ]; then
        echo Error: bad key hash ver; exit 1
    fi

    check_file bl40 "$bl40"

    check_file bl40key "$bl40key"
    if $encryptFIPbl3x; then
        if [ "$bl40aeskey" != "" ]; then
            check_file bl40aeskey "$bl40aeskey"
            check_file bl40aesiv "$bl40aesiv"
        fi
    fi
    if [ -z "$sigver" ]; then
        sigver=1
    fi
    if [ "$keyhashver" != "2" ] && [ "$keyhashver" != "3" ]; then
        echo Error: bad key hash ver; exit 1
    fi

    if [ -z "$output" ]; then echo Error: Missing output file option -o ; exit 1; fi

    bl40_align_size=$(aligned_size "$bl40" 0 16)
    cp $bl40 $TMP/bl40.bin.aligned
    pad_file $TMP/bl40.bin.aligned $bl40_align_size
    bl40_aligned=$TMP/bl40.bin.aligned
    bl40_size=$(($bl40_align_size + 4096))
    # Sign/encrypt BL30 payload using bl2 functions
    local bl40hdr_i="$bl40hdr"
    if [ "$bl40hdr" == "" ]; then
        check_file rootkey "$rootkey"
        if [ "$rootkeyidx" -lt 0 ] || [ "$rootkeyidx" -gt 2 ]; then
            echo "Error invalid root-key-idx: $rootkeyidx.";
            exit 1;
        fi
        check_file rootkey0 "$rootkey0"
        check_file rootkey1 "$rootkey1"

        bl40hdr_i="$TMP/bl40.hdri"
        # No pre-signed header given. Create one.
        sign_bl2_hdr -o "$bl40hdr_i" \
            --soc "$soc" \
            --root-key "$rootkey" --root-key-idx "$rootkeyidx" \
            --root-key-0 "$rootkey0" --root-key-1 "$rootkey1" \
            --root-key-2 "$rootkey0" --root-key-3 "$rootkey1" \
            --bl2-key-0 "$bl40key" --bl2-key-1 "$bl40key" \
            --bl2-key-2 "$bl40key" --bl2-key-3 "$bl40key" \
            --bl2-key "$bl40key" \
            --bl2-size $bl40_size \
            --arb-cvn $bl40_arb_cvn \
            --sig-ver $sigver --key-hash-ver $keyhashver
    fi
    sign_bl2 -i $bl40_aligned -o $TMP/bl40.bin.out \
        --bl2-hdr "$bl40hdr_i" \
        --bl2-key $bl40key \
        --bl2-size $bl40_size \
        $( [ -n "$bl40aesiv" ] && echo -n "--iv $bl40aesiv" ) \
        --sig-ver $sigver --key-hash-ver $keyhashver

    if $encryptFIPbl3x; then
        # bl40 encryption enable is based on a fuse bit, so use the 'encryptFIPbl3x' flag
        dd if=/dev/zero of=$TMP/zeroiv bs=16 count=1 >& /dev/null
        #internal_encrypt $TMP/bl40.bin.out $TMP/bl40.bin.sig $bl2aeskey $TMP/zeroiv
        # scp needs to decrypt in 2KB chunks:
        if [ -f $TMP/bl40.bin.sig ]; then rm -f $TMP/bl40.bin.sig ; fi
        openssl dgst -sha256 -out $TMP/realbl40key -binary $bl40aeskey
        for i in $(seq 0 $(( $bl40_size / 2048 )) ); do
            dd if=$TMP/bl40.bin.out of=$TMP/pt skip=$i bs=2048 count=1 >& /dev/null
            internal_encrypt $TMP/pt $TMP/ct $TMP/realbl40key $TMP/zeroiv
            cat $TMP/ct >> $TMP/bl40.bin.sig
        done
        if [ $(wc -c < $TMP/bl40.bin.sig) -ne $bl40_size ]; then
            echo Internal error, bl40.bin.sig size
            exit 1;
        fi
    else
        cp $TMP/bl40.bin.out $TMP/bl40.bin.sig
    fi

    cat $TMP/bl40.bin.sig > $output
    echo
    echo Created signed bootloader $output successfully
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
            --sign-bl2-hdr)
                sign_bl2_hdr "${argv[@]:$((i + 1))}"
                break ;;
            --create-signed-bl)
                create_signed_bl "${argv[@]:$((i + 1))}"
                break ;;
            --create-signed-bl40)
                create_signed_bl40 "${argv[@]:$((i + 1))}"
                break ;;
            --sign-kernel)
                sign_kernel "${argv[@]:$((i + 1))}"
                break ;;
            --sign-uimage)
                sign_uimage "${argv[@]:$((i + 1))}"
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
        echo "Failed to sign bootloader!"
    fi
    if [ ! -d "$TMP" ]; then return; fi
    local tmpfiles="bl2.bin.out bl2.bin.sig bl2cvn bl2.hdr bl2.img bl2key0.bin
    bl2key0.sha bl2key1.bin bl2key1.sha bl2key2.bin bl2key2.sha bl2key3.bin
    bl2key3.sha bl2key.bin bl2key.sha bl30.bin.out bl30.bin.img bl30.bin.sig bl30key.bin
    bl31.bin bl31.bin.img bl31.bin.sig bl31key.bin bl32.bin bl32.bin.img
    bl32.bin.sig bl32key.bin bl33.bin.img bl33.bin.sig bl33key.bin
    bl2aesiv
    bl.bin bl.hdr bl.hdr.sig blpad.bin bl-pl.sha bl2.hdri bl30.hdri
    bl32.img-info
    fip.hdr fip.hdr.out fip.hdr.sig kernelkey.bin keydata
    nonce.bin rootkey0.bin rootkey0.sha rootkey1.bin rootkey1.sha
    rootkey2.bin rootkey2.sha rootkey3.bin rootkey3.sha rootkey.bin
    rootkey.sha sigdata sigdata.sig zeroiv
    kernel zeroaesiv zeroaeskey zerokey
    bl2key0.pub bl2key1.pub  bl2key2.pub  bl2key3.pub  bl2key.pub  mod  modhex
    bl30key.pub  bl31key.pub  bl32key.pub  bl33key.pub  kernelkey.pub
    rootkey0.pub  rootkey1.pub  rootkey2.pub  rootkey3.pub  rootkey.pub
    fip.hdr.bin fip.payload.bin bl3x.payload.bin bl3x.bin ddrfw*.padded* ddrfw*.hdr ddr.fw.bin
    kern.hdr  kern.hdr.sig  kern-pl.bin  kern-pl.sha pt ct
    bl40.bin.out  bl40.bin.sig  bl40.hdri  realbl40key bl40.bin.aligned
    ihencmagic  uimage.aesiv  uimage.hdr  uimage.sb.hdr  uimage.temp.bin
    totalsize  uimage.aeskey  uimagepad.bin  uimage.temp  uimage.temp.sha uimage.sb.hdr.temp
    uimage.sb.hdr.bin  uimage.sb.hdr.sig  uimage.temp.bin.first  uimage.temp.bin.second"
    for i in $tmpfiles ; do
        rm -f $TMP/$i
    done
    rm -fr $TMP
}

trap cleanup EXIT

cleanup
if [ ! -d "$TMP" ]; then mkdir "$TMP" ; fi
parse_main "$@"
