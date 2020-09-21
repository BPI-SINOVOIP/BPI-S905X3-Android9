#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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

import argparse
import json
import logging
import socket
import time
import threading
import sys

from host_controller import console
from host_controller import tfc_host_controller
from host_controller.build import build_provider_pab
from host_controller.tfc import tfc_client
from host_controller.vti_interface import vti_endpoint_client
from host_controller.tradefed import remote_client
from vts.utils.python.os import env_utils

_ANDROID_BUILD_TOP = "ANDROID_BUILD_TOP"
_SECONDS_PER_UNIT = {
    "m": 60,
    "h": 60 * 60,
    "d": 60 * 60 * 24
}


def _ParseInterval(interval_str):
    """Parses string to time interval.

    Args:
        interval_str: string, a floating-point number followed by time unit.

    Returns:
        float, the interval in seconds.

    Raises:
        ValueError if the argument format is wrong.
    """
    if not interval_str:
        raise ValueError("Argument is empty.")

    unit = interval_str[-1]
    if unit not in _SECONDS_PER_UNIT:
        raise ValueError("Unknown unit: %s" % unit)

    interval = float(interval_str[:-1])
    if interval < 0:
        raise ValueError("Invalid time interval: %s" % interval)

    return interval * _SECONDS_PER_UNIT[unit]


def _ScriptLoop(hc_console, script_path, loop_interval):
    """Runs a console script repeatedly.

    Args:
        hc_console: the host controller console.
        script_path: string, the path to the script.
        loop_interval: float or integer, the interval in seconds.
    """
    next_start_time = time.time()
    while hc_console.ProcessScript(script_path):
        if loop_interval == 0:
            continue
        current_time = time.time()
        skip_cnt = (current_time - next_start_time) // loop_interval
        if skip_cnt >= 1:
            logging.warning("Script execution time is longer than loop "
                            "interval. Skip %d iteration(s).", skip_cnt)
        next_start_time += (skip_cnt + 1) * loop_interval
        if next_start_time - current_time >= 0:
            time.sleep(next_start_time - current_time)
        else:
            logging.error("Unexpected timestamps: current=%f, next=%f",
                          current_time, next_start_time)


def main():
    """Parses arguments and starts console."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--config-file",
                        default=None,
                        type=argparse.FileType('r'),
                        help="The configuration file in JSON format")
    parser.add_argument("--poll", action="store_true",
                        help="Disable console and start host controller "
                             "threads polling TFC.")
    parser.add_argument("--use-tfc", action="store_true",
                        help="Enable TFC (TradeFed Cluster).")
    parser.add_argument("--vti",
                        default=None,
                        help="The base address of VTI endpoint APIs")
    parser.add_argument("--script",
                        default=None,
                        help="The path to a script file in .py format")
    parser.add_argument("--serial",
                        default=None,
                        help="The default serial numbers for flashing and "
                             "testing in the console. Multiple serial numbers "
                             "are separated by comma.")
    parser.add_argument("--loop",
                        default=None,
                        metavar="INTERVAL",
                        type=_ParseInterval,
                        help="The interval of repeating the script. "
                             "The format is a float followed by unit which is "
                             "one of 'm' (minute), 'h' (hour), and 'd' (day). "
                             "If this option is unspecified, the script will "
                             "be processed once.")
    parser.add_argument("--console", action="store_true",
                        help="Whether to start a console after processing "
                             "a script.")
    args = parser.parse_args()
    if args.config_file:
        config_json = json.load(args.config_file)
    else:
        config_json = {}
        config_json["log_level"] = "DEBUG"
        config_json["hosts"] = []
        host_config = {}
        host_config["cluster_ids"] = ["local-cluster-1",
                                      "local-cluster-2"]
        host_config["lease_interval_sec"] = 30
        config_json["hosts"].append(host_config)

    env_vars = env_utils.SaveAndClearEnvVars([_ANDROID_BUILD_TOP])

    root_logger = logging.getLogger()
    root_logger.setLevel(getattr(logging, config_json["log_level"]))

    if args.vti:
        vti_endpoint = vti_endpoint_client.VtiEndpointClient(args.vti)
    else:
        vti_endpoint = None

    tfc = None
    if args.use_tfc:
        if args.config_file:
            tfc = tfc_client.CreateTfcClient(
                    config_json["tfc_api_root"],
                    config_json["service_key_json_path"],
                    api_name=config_json["tfc_api_name"],
                    api_version=config_json["tfc_api_version"],
                    scopes=config_json["tfc_scopes"])
        else:
            print("WARN: If --use_tfc is set, --config_file argument value "
                  "must be provided. Starting without TFC.")

    pab = build_provider_pab.BuildProviderPAB()

    hosts = []
    for host_config in config_json["hosts"]:
        cluster_ids = host_config["cluster_ids"]
        # If host name is not specified, use local host.
        hostname = host_config.get("hostname", socket.gethostname())
        port = host_config.get("port", remote_client.DEFAULT_PORT)
        cluster_ids = host_config["cluster_ids"]
        remote = remote_client.RemoteClient(hostname, port)
        host = tfc_host_controller.HostController(remote, tfc, hostname,
                                                  cluster_ids)
        hosts.append(host)
        if args.poll:
            lease_interval_sec = host_config["lease_interval_sec"]
            host_thread = threading.Thread(target=host.Run,
                                           args=(lease_interval_sec,))
            host_thread.daemon = True
            host_thread.start()

    if args.poll:
        while True:
            sys.stdin.readline()
    else:
        main_console = console.Console(vti_endpoint, tfc, pab, hosts,
                                       vti_address=args.vti)
        main_console.StartJobThreadAndProcessPool()
        try:
            if args.serial:
                main_console.SetSerials(args.serial.split(","))
            if args.script:
                if args.loop is None:
                    main_console.ProcessScript(args.script)
                else:
                    _ScriptLoop(main_console, args.script, args.loop)

                if args.console:
                    main_console.cmdloop()
            else:  # if not script, the default is console mode.
                main_console.cmdloop()
        finally:
            main_console.TearDown()

    env_utils.RestoreEnvVars(env_vars)


if __name__ == "__main__":
    main()
