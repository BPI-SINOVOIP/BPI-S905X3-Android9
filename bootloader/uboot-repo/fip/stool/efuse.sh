#!/bin/bash

usage() {
    cat << EOF
Usage: $(basename $0) --help
       $(basename $0) --version
       $(basename $0) --generate-efuse-pattern \\
                      --soc [gxl | txlx | g12a | g12b | tl1 | tm2 | c1 ] \\
                      [--aml-key-path path-of-key]  \\
                      [--rsa-key-path path-of-rsa-key]  \\
                      [--enable-sb false]               \\
                      [--aes-key  aes-key]              \\
                      [--enable-aes false]              \\
                      [--password-hash password.hash]   \\
                      [--enable-jtag-password false]    \\
                      [--enable-usb-password false]     \\
                      [--enable-anti-rollback false]    \\
                      -o pattern.efuse

EOF
    exit 1
}

randomstr=`date +%Y%m%d%H%M%S`
amlkeypath=$randomstr
rsakeypath=$randomstr
kwrap=""
wrlock_kwrap="false"
roothash=""
passwordhash=$randomstr
scanpasswordhash=""
userefusefile=""
aeskey=$randomstr
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
disablebootsdcard="false"
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
soc="unknown"
socrev="a"
opt_raw_otp_pattern="false"


check_file() {
    if [ ! -f "$2" ]; then echo Error: $1 file : \""$2"\" not exist; exit 1 ; fi
}

generate_efuse_pattern() {
    local argv=("$@")
    local i=0
     # Parse args
    i=0
    while [ $i -lt $# ]; do
        arg="${argv[$i]}"
				#echo "i=$i argv[$i]=${argv[$i]}"
        i=$((i + 1))
        case "$arg" in
            --aml-key-path)
               amlkeypath="${argv[$i]}" ;;
            --rsa-key-path)
               rsakeypath="${argv[$i]}" ;;
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
            --disable-boot-sdcard)
                disablebootsdcard="${argv[$i]}" ;;
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
                i=$((i - 1));;
            --raw-otp-pattern)
                opt_raw_otp_pattern="${argv[$i]}" ;;
            --soc)
                soc="${argv[$i]}" ;;
            --soc-rev)
                socrev="${argv[$i]}" ;;
            *)
                echo "Unknown option $arg"; exit 1
                ;;
        esac
        i=$((i + 1))
    done

local tool_type=gxl
local hashver=2

#check soc first, only support gxl/txlx/g12a/g12b/tl1/tm2/c1
if [ ${soc} == "g12a" ] || [ ${soc} == "g12b" ]; then
	tool_type=g12a
	soc=g12a
elif [ ${soc} == "txlx" ] || [ ${soc} == "gxl" ] ; then
  tool_type=gxl
elif [ ${soc} == "tl1" ] || [ ${soc} == "tm2" ] ;  then
  tool_type=tl1
  soc=tl1
elif [ ${soc} == "c1" ] ;  then
  tool_type=c1
  soc=c1
  hashver=3
else
  echo invalid soc [$soc]
  exit 1
fi

if [ ${soc} == "gxl" ];then
hashver=1
fi

readonly tools_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
local sign_boot_tool_dev="${tools_dir}/signing-tool-${tool_type}-dev/sign-boot-${tool_type}-dev.sh"
local efuse_gen="${tools_dir}/signing-tool-${tool_type}-dev/efuse-gen.sh"

if [ "$amlkeypath" != "$randomstr" ]; then
	rsakeypath="$amlkeypath"
	aeskey="$amlkeypath"/bl2aeskey
	#echo rsakeypath=$rsakeypath
	#echo aeskey=$aeskey
fi

readonly rsa_root0=${rsakeypath}/root0.pem
readonly rsa_root1=${rsakeypath}/root1.pem
readonly rsa_root2=${rsakeypath}/root2.pem
readonly rsa_root3=${rsakeypath}/root3.pem

local secure_boot_option=""
local other_option=""

if [ "$passwordhash" != "$randomstr" ]; then
	if [ -f "$passwordhash" ]; then
	  local imagesize=$(wc -c < ${passwordhash})
	  #echo passwordhashsize=$imagesize
	  if [ $imagesize -ne 32 ]; then
	    echo "password hash $passwordhash size=$imagesize is illegal!"
	    exit 1
	  fi
		other_option="--password-hash ${passwordhash} "
	else
		check_file "password-hash"  "$passwordhash"
	fi
fi

other_option="${other_option} --enable-usb-password $enableusbpassword --enable-jtag-password $enablejtagpassword \
 --enable-anti-rollback $enableantirollback --raw-otp-pattern ${opt_raw_otp_pattern}"

#aes key process
if [ "$aeskey" != "$randomstr" ]; then
	if [ -f "$aeskey" ]; then
	  local imagesize=$(wc -c < ${aeskey})
	  #echo passwordhashsize=$imagesize
	  if [ $imagesize -ne 32 ]; then
	    echo "password hash $aeskey size=$imagesize is illegal!"
	    exit 1
	  fi
		secure_boot_option="${secure_boot_option} --aes-key ${aeskey} "
	else
		check_file "aes-key"  "$aeskey"
	fi
fi

#rsa key process
local rsa_root_hash=${rsakeypath}/rootkeys-hash.bin
if [  "$rsakeypath" != "$randomstr" ]; then
	if [ ! -d "$rsakeypath" ]; then
		echo illegal rsa key path:$rsakeypath
		exit 1
	else
		#key path does exist, rsa key process
		check_file rootkey0 "$rsa_root0"
		check_file rootkey1 "$rsa_root1"
		check_file rootkey2 "$rsa_root2"
		check_file rootkey3 "$rsa_root3"

		"$sign_boot_tool_dev" --create-root-hash \
		--key-hash-ver $hashver                  \
		--root-key-0 "$rsa_root0"                \
		--root-key-1 "$rsa_root1"                \
		--root-key-2 "$rsa_root2"                \
		--root-key-3 "$rsa_root3"                \
		-o "$rsa_root_hash"

		check_file rsarootsha   "$rsa_root_hash"

		secure_boot_option="${secure_boot_option} --root-hash $rsa_root_hash "
	fi
fi

secure_boot_option="${secure_boot_option} --enable-sb ${enablesb} --enable-aes ${enableaes} "

#echo other_option=$other_option
#echo secure_boot_option=$secure_boot_option

"$efuse_gen" --generate-efuse-pattern \
  --soc $soc                          \
  --key-hash-ver $hashver             \
  $secure_boot_option                 \
  $other_option                       \
  -o ${output}

#generate debug pattern for double check
other_option="${other_option} --raw-otp-pattern true"

"$efuse_gen" --generate-efuse-pattern \
  --soc $soc                          \
  --key-hash-ver $hashver             \
  $secure_boot_option                 \
  $other_option                       \
  -o ${output}.debug

rm -f $rsa_root_hash

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