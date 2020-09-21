#!/bin/sh

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script updates chameleon boards and cle in the audioboxes
# and atlantis labs. This script takes about 15-30s per board.

HOSTS="chromeos9-audiobox1-host2
       chromeos9-audiobox2-host1"

# NOTE: May need to update based on where test_rsa is located.
SSH_OPTIONS="-q -i ~/.ssh/.test_rsa \
             -o UserKnownHostsFile=/dev/null \
             -o StrictHostKeyChecking=no"

PROBE_RESULT_DIR="/tmp/chameleon_update_result"

SEP_LINE="--------------------------------------------------------------------------------"


function disp_result {
  test "$1" -eq "0" && echo ok || echo "-"
}

function probe_chameleon {
  chameleon="$1-chameleon.cros"

  # ping test
  ping -q -w 10 -c1 "${chameleon}" > /dev/null 2>&1
  ping_result="$(disp_result $?)"

  # Check if chameleond is running.
  test $(ssh ${SSH_OPTIONS} root@"$chameleon" \
         ps | awk '$5~"run_chameleond"' | wc -l) -gt "0"
  chameleond_result="$(disp_result $?)"

  # clear /dev/root space
  ssh $SSH_OPTIONS root@"$chameleon" "echo "" > /www/logs/lighttpd.error.log"
  root_result="$(disp_result $?)"

  # run update command
  ssh $SSH_OPTIONS root@"$chameleon" "/etc/init.d/chameleon-updater start >/dev/null 2>&1"
  update_result="$(disp_result $?)"

  # Print the result
  printf "$1-chameleon  %5s %10s %10s      %s\n" "${ping_result}" \
      "${chameleond_result}" "${root_result}" "${update_result}" \
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
  dut_pids=""
  chameleon_pids=""

  mkdir -p "${PROBE_RESULT_DIR}"
  rm -fr "${PROBE_RESULT_DIR}"/*
}

function print_chameleon_status {
  echo "Chameleon                              ping   chameleond    root   update"
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
