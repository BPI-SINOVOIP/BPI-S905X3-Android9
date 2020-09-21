#!/bin/bash -u

# This is an example execution script.
# This script changes with the problem you are trying to fix.
# This particular script was used to triage a problem where a glibc
# compiled with a new compiler would expose a problem in piglit.
# Note it returns 0 only when the installation of the image succeeded
# (ie: the machine booted after installation)

source common/common.sh

#export BISECT_STAGE=TRIAGE
echo "BISECT_STAGE=${BISECT_STAGE}"

echo "State of sets"
wc -l ${bisect_dir}/*_SET

board=x86-alex
DUT=172.17.186.180

echo "Cleaning up"
{ sudo emerge -C cross-i686-pc-linux-gnu/glibc || exit 125; } &>> /tmp/glibc_triage.log

echo "Building"
{ sudo -E emerge cross-i686-pc-linux-gnu/glibc || exit 125; } &>> /tmp/glibc_triage.log

echo "Building image"
{ /home/llozano/trunk/src/scripts/build_image --board=${board} test || exit 125; } &>> /tmp/glibc_triage.log

echo "Installing image"
cros flash ${DUT} latest &> /tmp/tmp_cros_flash_result.log

cat /tmp/tmp_cros_flash_result.log >> /tmp/cros_flash_result.log

grep "Cros Flash completed successfully" /tmp/tmp_cros_flash_result.log || exit 125

echo "Trying piglit"

echo "export DISPLAY=:0.0; echo \$DISPLAY; /usr/local/piglit/lib/piglit/bin/glx-close-display -auto" > /tmp/repro.sh
SSH_OPTS="-oUserKnownHostsFile=/dev/null -oStrictHostKeyChecking=no -oServerAliveInterval=10 -i /var/cache/chromeos-cache/distfiles/target/./chrome-src/src/third_party/chromite/ssh_keys/testing_rsa"
scp ${SSH_OPTS} /tmp/repro.sh root@${DUT}:/tmp

# notice the bash -l here. Otherwise the DISPLAY cannot be set
( ssh ${SSH_OPTS} root@${DUT} -- /bin/bash -l /tmp/repro.sh ) > /tmp/result
grep pass /tmp/result || { echo "PIGLIT FAILED"; exit 1; }

echo "PIGLIT PASSED"

exit 0
