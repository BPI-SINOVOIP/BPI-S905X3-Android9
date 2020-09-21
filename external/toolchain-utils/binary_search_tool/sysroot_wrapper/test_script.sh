#!/bin/bash -u

# This is an example execution script.
# This script changes with the problem you are trying to fix.
# This particular script was used to triage a problem where the kernel
# would not boot while migrating to GCC 4.9.
# Note it returns 0 only when the installation of the image succeeded
# (ie: the machine booted after installation)

source common/common.sh

export BISECT_STAGE=TRIAGE
echo "BISECT_STAGE=${BISECT_STAGE}"

echo "State of sets"
wc -l ${bisect_dir}/*_SET

echo "Cleaning up"
{ /usr/bin/sudo rm -rf /build/falco/var/cache/portage/sys-kernel && emerge-falco -C sys-kernel/chromeos-kernel-3_8-3.8.11-r96 || exit 125; } &>> /tmp/kernel_triage.log

echo "Building"
{ /usr/local/bin/emerge-falco =sys-kernel/chromeos-kernel-3_8-3.8.11-r96 || exit 125; } &>> /tmp/kernel_triage.log

echo "Building image"
{ /home/llozano/trunk/src/scripts/build_image --board=falco test || exit 125; } &>> /tmp/kernel_triage.log

echo "Installing image"
cros flash 172.17.187.150 latest &> /tmp/tmp_cros_flash_result.log

cat /tmp/tmp_cros_flash_result.log >> /tmp/cros_flash_result.log

grep "Cros Flash completed successfully" /tmp/tmp_cros_flash_result.log || exit 1

exit 0
