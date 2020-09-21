#!/bin/bash
#
# Copyright 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -euo pipefail

function set_route {
  local mode="${1}"
  local ip="${2}"
  local host="${3}"
  local gw="${4}"

  if [ "${mode}" = "ethernet" ]; then
    ${ip} route del "${host}" via "${gw}"
  else
    # FIXME: This *should* work for IPv6, but it returns EINVAL.
    ${ip} route add "${host}" via "${gw}"
    dbus-send --system --dest=org.chromium.flimflam --print-reply / \
      org.chromium.flimflam.Manager.SetServiceOrder \
      string:"vpn,wifi,ethernet,wimax,cellular"
  fi
}

function find_route {
  local mode="${1}"
  local host="${2}"
  local ip="ip -4"

  if [[ "${host}" = *:* ]]; then
    ip="ip -6"
  fi

  set_route "${mode}" "${ip}" "${host}" 192.168.231.254
  exit 0
}

function parse_netstat {
  local mode="${1}"

  while read -r proto recv_q send_q local foreign state; do
    if [[ "${proto}" = tcp* && \
          ("${local}" = *:22 || "${local}" = *:2222) && \
          "${state}" == ESTABLISHED ]]; then
      find_route "${mode}" "${foreign%:*}"
      exit 0
    fi
  done

  echo "Could not find ssh connection in netstat"
  exit 1
}

mode="${1:-}"
if [ "${mode}" != "wifi" -a "${mode}" != "ethernet" ]; then
  echo "Tells shill to prioritize ethernet or wifi, and adds a route"
  echo "back to the ssh/adb host so that the device can still be controlled"
  echo "remotely."
  echo ""
  echo "usage: ${0} { ethernet | wifi }"
  exit 1
fi

if [ "${mode}" = "ethernet" ]; then
  # Switch the service order first, because the IP lookup might fail.
  dbus-send --system --dest=org.chromium.flimflam --print-reply / \
    org.chromium.flimflam.Manager.SetServiceOrder \
    string:"vpn,ethernet,wifi,wimax,cellular"
fi

# Find the first connection to our local port 22 (ssh), then use it to
# set a static route via eth0.
# This should ideally use $SSH_CLIENT instead, but that will require enabling
# transparent mode in sslh because $SSH_CLIENT currently points to
# 127.0.0.1.
netstat --tcp --numeric --wide | parse_netstat "${mode}"
exit 0

