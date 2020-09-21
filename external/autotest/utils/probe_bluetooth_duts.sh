#!/bin/sh

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script probes the readiness of the DUTs and chameleon boards in
# the wifi-cells in the test lab.

BT_HOSTS="chromeos15-row1-rack4-host1
          chromeos15-row1-rack4-host2
          chromeos15-row1-rack4-host3
          chromeos15-row1-rack5-host1
          chromeos15-row1-rack5-host2
          chromeos15-row1-rack5-host3
          chromeos15-row1-rack5-host4"

SSH_OPTIONS="-q -i ~/.ssh/.test_rsa \
             -o UserKnownHostsFile=/dev/null \
             -o StrictHostKeyChecking=no"

PROBE_RESULT_DIR="/tmp/bt_probe_result"

SEP_LINE="------------------------------------------------------------------"


function disp_result {
  test "$1" -eq "0" && echo ok || echo "-"
}

function probe_dut {
  dut="$1.cros"

  # ping test
  ping -q -w 10 -c1 "${dut}" > /dev/null 2>&1
  ping_result="$(disp_result $?)"

  # Check board
  board=$(ssh ${SSH_OPTIONS} root@"$dut" cat /etc/lsb-release | \
          awk -F'=' '/CHROMEOS_RELEASE_BUILDER_PATH/ {print $2}')

  # Print the result
  printf "$1            %5s  %s\n" "${ping_result}" "${board}" \
    > "${PROBE_RESULT_DIR}/${dut}"
}

function probe_chameleon {
  RN42_MODULE="/usr/lib/python2.7/site-packages/chameleond-0.0.2-py2.7.egg/chameleond/utils/bluetooth_rn42.py"

  chameleon="$1-chameleon.cros"

  # ping test
  ping -q -w 10 -c1 "${chameleon}" > /dev/null 2>&1
  ping_result="$(disp_result $?)"

  # .usb_host_mode label test
  ssh $SSH_OPTIONS root@"$chameleon" "test -f /etc/default/.usb_host_mode"
  label_result="$(disp_result $?)"

  # Check if chameleond is running.
  test $(ssh ${SSH_OPTIONS} root@"$chameleon" \
         ps | awk '$5~"run_chameleond"' | wc -l) -gt "0"
  chameleond_result="$(disp_result $?)"

  # RN42 self-test status
  ssh ${SSH_OPTIONS} root@"$chameleon" \
      "python ${RN42_MODULE} > /dev/null 2>&1"
      # "find / -name *bluetooth_rn42.py | xargs python > /dev/null 2>&1"
  RN42_result="$(disp_result $?)"

  # TODO: add self test for bluefruit too.

  # Print the result
  printf "$1-chameleon  %5s %6s %10s %4s\n" "${ping_result}" \
      "${label_result}" "${chameleond_result}" "${RN42_result}" \
    > "${PROBE_RESULT_DIR}/${chameleon}"
}

function probe_duts {
  # Fork parallel processes to probe the DUTs.
  for host in $BT_HOSTS; do
    probe_dut $host &
    dut_pids="${dut_pids} $!"
  done
}

function probe_chameleons {
  # Fork parallel processes to probe the chameleon boards.
  for host in $BT_HOSTS; do
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

function print_dut_status {
  echo
  echo "Dut                                    ping                  board"
  echo "${SEP_LINE}"

  # Wait for all probing children processes to terminate.
  for pid in ${dut_pids}; do
    wait ${pid}
  done

  # Sort and print the results.
  cat "${PROBE_RESULT_DIR}"/*-host?.cros | sort
  echo; echo
}

function print_chameleon_status {
  echo "Chameleon                              ping  label chameleond rn42"
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
probe_duts
probe_chameleons
print_dut_status
print_chameleon_status
