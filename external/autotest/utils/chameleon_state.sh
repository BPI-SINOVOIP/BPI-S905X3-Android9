#!/bin/sh

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script probes the readiness of chameleon boards in
# the audioboxes and atlantis labs. This script takes about 2 seconds per board.
# The total time for all hosts listed is 5-6 minutes.

#TODO (rjahagir): Add command line for a status check on only a few hosts.
HOSTS="chromeos2-row10-rack10-host1
       chromeos2-row10-rack10-host11
       chromeos2-row10-rack10-host13
       chromeos2-row10-rack10-host15
       chromeos2-row10-rack10-host17
       chromeos2-row10-rack10-host19
       chromeos2-row10-rack10-host3
       chromeos2-row10-rack10-host5
       chromeos2-row10-rack10-host7
       chromeos2-row10-rack10-host9
       chromeos2-row10-rack5-host11
       chromeos2-row10-rack5-host13
       chromeos2-row10-rack5-host15
       chromeos2-row10-rack5-host17
       chromeos2-row10-rack5-host19
       chromeos2-row10-rack5-host21
       chromeos2-row10-rack6-host1
       chromeos2-row10-rack6-host11
       chromeos2-row10-rack6-host13
       chromeos2-row10-rack6-host15
       chromeos2-row10-rack6-host3
       chromeos2-row10-rack6-host5
       chromeos2-row10-rack6-host7
       chromeos2-row10-rack6-host9
       chromeos2-row10-rack7-host1
       chromeos2-row10-rack7-host11
       chromeos2-row10-rack7-host13
       chromeos2-row10-rack7-host15
       chromeos2-row10-rack7-host17
       chromeos2-row10-rack7-host3
       chromeos2-row10-rack7-host5
       chromeos2-row10-rack7-host7
       chromeos2-row10-rack7-host9
       chromeos2-row10-rack8-host1
       chromeos2-row10-rack8-host13
       chromeos2-row10-rack8-host15
       chromeos2-row10-rack8-host17
       chromeos2-row10-rack8-host19
       chromeos2-row10-rack8-host21
       chromeos2-row10-rack8-host3
       chromeos2-row10-rack8-host5
       chromeos2-row10-rack8-host7
       chromeos2-row10-rack8-host9
       chromeos2-row10-rack9-host11
       chromeos2-row10-rack9-host13
       chromeos2-row10-rack9-host15
       chromeos2-row10-rack9-host17
       chromeos2-row10-rack9-host19
       chromeos2-row10-rack9-host21
       chromeos2-row10-rack9-host3
       chromeos2-row10-rack9-host5
       chromeos2-row10-rack9-host7
       chromeos2-row10-rack9-host9
       chromeos9-audiobox1-host1
       chromeos9-audiobox1-host2
       chromeos9-audiobox2-host1
       chromeos9-audiobox2-host2
       chromeos9-audiobox3-host1
       chromeos9-audiobox3-host2
       chromeos9-audiobox4-host1
       chromeos9-audiobox4-host2
       chromeos9-audiobox5-host1
       chromeos9-audiobox5-host2
       chromeos9-audiobox6-host1
       chromeos9-audiobox6-host2
       chromeos9-audiobox7-host1
       chromeos9-audiobox7-host2
       chromeos1-row5-rack1-host2
       chromeos1-row5-rack2-host2
       chromeos1-row2-rack3-host4
       chromeos1-row2-rack4-host4"

# NOTE: May need to update based on where test_rsa is located.
SSH_OPTIONS="-q -i ~/.ssh/.test_rsa \
             -o UserKnownHostsFile=/dev/null \
             -o StrictHostKeyChecking=no"

PROBE_RESULT_DIR="/tmp/chameleon_probe_result"

SEP_LINE="--------------------------------------------------------------------------------------"


function disp_result {
  test "$1" -eq "0" && echo ok || echo "-"
}

function probe_chameleon {
  chameleon="$1-chameleon.cros"

  # ping test
  ping -q -w 10 -c1 "${chameleon}" > /dev/null 2>&1
  ping_result="$(disp_result $?)"

  # checking /dev/root space
  devroot_space=$(ssh ${SSH_OPTIONS} root@"$chameleon" \
     df -h | awk -F' ' 'FNR == 2 {print $5}')\

  # Check if chameleond is running.
  test $(ssh ${SSH_OPTIONS} root@"$chameleon" \
         ps | awk '$5~"run_chameleond"' | wc -l) -gt "0"
  chameleond_result="$(disp_result $?)"

  # Check chameleond version
  chameleond_version=$(ssh ${SSH_OPTIONS} root@"$chameleon" \
     cat /etc/default/chameleond | \
     awk -F'=' '/BUNDLE_VERSION/ {print $2}')\

  # Print the result
  printf "$1-chameleon  %5s %10s %10s      %s\n" "${ping_result}" \
      "${devroot_space}" "${chameleond_result}" "${chameleond_version}" \
    > "${PROBE_RESULT_DIR}/${chameleon}"
}

function probe_chameleons {
  # Fork parallel processes to probe the chameleon boards.
  for host in $HOSTS; do
    probe_chameleon $host &
    chameleon_pids="${chameleon_pids} $!"
  done
}

function create_ping_result_dir {
  chameleon_pids=""

  mkdir -p "${PROBE_RESULT_DIR}"
  rm -fr "${PROBE_RESULT_DIR}"/*
}

function print_chameleon_status {
  echo "Chameleon                               ping    /dev/root   chameleond     version"
  echo "${SEP_LINE}"

  # Wait for all probing children processes to terminate.
  for pid in ${chameleon_pids}; do
    wait ${pid}
  done

  # Sort and print the results.
  cat "${PROBE_RESULT_DIR}"/*-chameleon.cros | sort
  echo; echo
}

create_ping_result_dir
probe_chameleons
print_chameleon_status
