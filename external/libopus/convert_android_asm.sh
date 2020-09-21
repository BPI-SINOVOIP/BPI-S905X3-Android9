#!/bin/bash

set -e
ASM_CONVERTER="./celt/arm/arm2gnu.pl"

if [[ ! -x "${ASM_CONVERTER}" ]]; then
  echo "This script should be run from external/libopus."
  exit
fi

while read file; do
  # This check is required because the ASM conversion script doesn't seem to be
  # idempotent.
  if [[ ! "${file}" =~ .*_gnu\.s$ ]]; then
    gnu_file="${file%.s}_gnu.s"
    ${ASM_CONVERTER} "${file}" > "${gnu_file}"
    # The ASM conversion script replaces includes with *_gnu.S. So, replace
    # occurences of "*-gnu.S" with "*_gnu.s".
    sed -i "s/-gnu\.S/_gnu\.s/g" "${gnu_file}"
    rm -f "${file}"
  fi
done < <(find . -iname '*.s')

# Generate armopts.s from armopts.s.in
sed \
  -e "s/@OPUS_ARM_MAY_HAVE_EDSP@/1/g" \
  -e "s/@OPUS_ARM_MAY_HAVE_MEDIA@/1/g" \
  -e "s/@OPUS_ARM_MAY_HAVE_NEON@/1/g" \
  -e "s/@OPUS_ARM_MAY_HAVE_NEON_INTR@/1/g" \
	celt/arm/armopts.s.in > celt/arm/armopts.s.temp
${ASM_CONVERTER} "celt/arm/armopts.s.temp" > "celt/arm/armopts_gnu.s"
rm "celt/arm/armopts.s.temp"
echo "Converted all ASM files and generated armopts.s successfully."
