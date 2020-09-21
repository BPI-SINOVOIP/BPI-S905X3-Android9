#!/usr/bin/python3.4
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import json
import pprint
import queue
import threading
import time

from acts import asserts
from acts.test_utils.net import connectivity_const as cconsts
from acts.test_utils.wifi.aware import aware_const as aconsts
from acts.test_utils.wifi.aware import aware_test_utils as autils
from acts.test_utils.wifi.aware.AwareBaseTest import AwareBaseTest


class ThroughputTest(AwareBaseTest):
  """Set of tests for Wi-Fi Aware to measure latency of Aware operations."""

  SERVICE_NAME = "GoogleTestServiceXYZ"

  PASSPHRASE = "This is some random passphrase - very very secure!!"
  PASSPHRASE2 = "This is some random passphrase - very very secure - but diff!!"

  def __init__(self, controllers):
    super(ThroughputTest, self).__init__(controllers)

  def request_network(self, dut, ns):
    """Request a Wi-Fi Aware network.

    Args:
      dut: Device
      ns: Network specifier
    Returns: the request key
    """
    network_req = {"TransportType": 5, "NetworkSpecifier": ns}
    return dut.droid.connectivityRequestWifiAwareNetwork(network_req)

  def run_iperf_single_ndp_aware_only(self, use_ib, results):
    """Measure iperf performance on a single NDP, with Aware enabled and no
    infrastructure connection - i.e. device is not associated to an AP.

    Args:
      use_ib: True to use in-band discovery, False to use out-of-band discovery.
      results: Dictionary into which to place test results.
    """
    init_dut = self.android_devices[0]
    resp_dut = self.android_devices[1]

    if use_ib:
      # note: Publisher = Responder, Subscribe = Initiator
      (resp_req_key, init_req_key, resp_aware_if,
       init_aware_if, resp_ipv6, init_ipv6) = autils.create_ib_ndp(
           resp_dut, init_dut,
           autils.create_discovery_config(self.SERVICE_NAME,
                                          aconsts.PUBLISH_TYPE_UNSOLICITED),
           autils.create_discovery_config(self.SERVICE_NAME,
                                          aconsts.SUBSCRIBE_TYPE_PASSIVE),
           self.device_startup_offset)
    else:
      (init_req_key, resp_req_key, init_aware_if, resp_aware_if, init_ipv6,
      resp_ipv6) = autils.create_oob_ndp(init_dut, resp_dut)
    self.log.info("Interface names: I=%s, R=%s", init_aware_if, resp_aware_if)
    self.log.info("Interface addresses (IPv6): I=%s, R=%s", init_ipv6,
                  resp_ipv6)

    # Run iperf3
    result, data = init_dut.run_iperf_server("-D")
    asserts.assert_true(result, "Can't start iperf3 server")

    result, data = resp_dut.run_iperf_client(
        "%s%%%s" % (init_ipv6, resp_aware_if), "-6 -J")
    self.log.debug(data)
    asserts.assert_true(result,
                        "Failure starting/running iperf3 in client mode")
    self.log.debug(pprint.pformat(data))

    # clean-up
    resp_dut.droid.connectivityUnregisterNetworkCallback(resp_req_key)
    init_dut.droid.connectivityUnregisterNetworkCallback(init_req_key)

    # Collect results
    data_json = json.loads("".join(data))
    if "error" in data_json:
      asserts.fail(
          "iperf run failed: %s" % data_json["error"], extras=data_json)
    results["tx_rate"] = data_json["end"]["sum_sent"]["bits_per_second"]
    results["rx_rate"] = data_json["end"]["sum_received"]["bits_per_second"]
    self.log.info("iPerf3: Sent = %d bps Received = %d bps", results["tx_rate"],
                  results["rx_rate"])

  def run_iperf(self, q, dut, peer_dut, peer_aware_if, dut_ipv6, port):
    """Runs iperf and places results in the queue.

    Args:
      q: The queue into which to place the results
      dut: The DUT on which to run the iperf server command.
      peer_dut: The DUT on which to run the iperf client command.
      peer_aware_if: The interface on the DUT.
      dut_ipv6: The IPv6 address of the server.
      port: The port to use for the server and client.
    """
    result, data = dut.run_iperf_server("-D -p %d" % port)
    asserts.assert_true(result, "Can't start iperf3 server")

    result, data = peer_dut.run_iperf_client(
        "%s%%%s" % (dut_ipv6, peer_aware_if), "-6 -J -p %d" % port)
    self.log.debug(data)
    q.put((result, data))

  def run_iperf_max_ndp_aware_only(self, results):
    """Measure iperf performance on the max number of concurrent OOB NDPs, with
    Aware enabled and no infrastructure connection - i.e. device is not
    associated to an AP.

    Note: the test requires MAX_NDP + 1 devices to be validated. If these are
    not available the test will fail.

    Args:
      results: Dictionary into which to place test results.
    """
    dut = self.android_devices[0]

    # get max NDP: using first available device (assumes all devices are the
    # same)
    max_ndp = dut.aware_capabilities[aconsts.CAP_MAX_NDP_SESSIONS]
    asserts.assert_true(len(self.android_devices) > max_ndp,
                        'Needed %d devices to run the test, have %d' %
                        (max_ndp + 1, len(self.android_devices)))

    # create all NDPs
    dut_aware_if = None
    dut_ipv6 = None
    peers_aware_ifs = []
    peers_ipv6s = []
    dut_requests = []
    peers_requests = []
    for i in range(max_ndp):
      (init_req_key, resp_req_key, init_aware_if, resp_aware_if, init_ipv6,
       resp_ipv6) = autils.create_oob_ndp(dut, self.android_devices[i + 1])
      self.log.info("Interface names: I=%s, R=%s", init_aware_if, resp_aware_if)
      self.log.info("Interface addresses (IPv6): I=%s, R=%s", init_ipv6,
                    resp_ipv6)

      dut_requests.append(init_req_key)
      peers_requests.append(resp_req_key)
      if dut_aware_if is None:
        dut_aware_if = init_aware_if
      else:
        asserts.assert_equal(
            dut_aware_if, init_aware_if,
            "DUT (Initiator) interface changed on subsequent NDPs!?")
      if dut_ipv6 is None:
        dut_ipv6 = init_ipv6
      else:
        asserts.assert_equal(
            dut_ipv6, init_ipv6,
            "DUT (Initiator) IPv6 changed on subsequent NDPs!?")
      peers_aware_ifs.append(resp_aware_if)
      peers_ipv6s.append(resp_ipv6)

    # create threads, start them, and wait for all to finish
    base_port = 5000
    q = queue.Queue()
    threads = []
    for i in range(max_ndp):
      threads.append(
          threading.Thread(
              target=self.run_iperf,
              args=(q, dut, self.android_devices[i + 1], peers_aware_ifs[i],
                    dut_ipv6, base_port + i)))

    for thread in threads:
      thread.start()

    for thread in threads:
      thread.join()

    # cleanup
    for i in range(max_ndp):
      dut.droid.connectivityUnregisterNetworkCallback(dut_requests[i])
      self.android_devices[i + 1].droid.connectivityUnregisterNetworkCallback(
          peers_requests[i])

    # collect data
    for i in range(max_ndp):
      results[i] = {}
      result, data = q.get()
      asserts.assert_true(result,
                          "Failure starting/running iperf3 in client mode")
      self.log.debug(pprint.pformat(data))
      data_json = json.loads("".join(data))
      if "error" in data_json:
        asserts.fail(
            "iperf run failed: %s" % data_json["error"], extras=data_json)
      results[i]["tx_rate"] = data_json["end"]["sum_sent"]["bits_per_second"]
      results[i]["rx_rate"] = data_json["end"]["sum_received"][
          "bits_per_second"]
      self.log.info("iPerf3: Sent = %d bps Received = %d bps",
                    results[i]["tx_rate"], results[i]["rx_rate"])

  ########################################################################

  def test_iperf_single_ndp_aware_only_ib(self):
    """Measure throughput using iperf on a single NDP, with Aware enabled and
    no infrastructure connection. Use in-band discovery."""
    results = {}
    self.run_iperf_single_ndp_aware_only(use_ib=True, results=results)
    asserts.explicit_pass(
        "test_iperf_single_ndp_aware_only_ib passes", extras=results)

  def test_iperf_single_ndp_aware_only_oob(self):
    """Measure throughput using iperf on a single NDP, with Aware enabled and
    no infrastructure connection. Use out-of-band discovery."""
    results = {}
    self.run_iperf_single_ndp_aware_only(use_ib=False, results=results)
    asserts.explicit_pass(
        "test_iperf_single_ndp_aware_only_oob passes", extras=results)

  def test_iperf_max_ndp_aware_only_oob(self):
    """Measure throughput using iperf on all possible concurrent NDPs, with
    Aware enabled and no infrastructure connection. Use out-of-band discovery.
    """
    results = {}
    self.run_iperf_max_ndp_aware_only(results=results)
    asserts.explicit_pass(
        "test_iperf_max_ndp_aware_only_oob passes", extras=results)

  ########################################################################

  def run_iperf_max_ndi_aware_only(self, sec_configs, results):
    """Measure iperf performance on multiple NDPs between 2 devices using
    different security configurations (and hence different NDIs). Test with
    Aware enabled and no infrastructure connection - i.e. device is not
    associated to an AP.

    The security configuration can be:
    - None: open
    - String: passphrase
    - otherwise: PMK (byte array)

    Args:
      sec_configs: list of security configurations
      results: Dictionary into which to place test results.
    """
    init_dut = self.android_devices[0]
    init_dut.pretty_name = "Initiator"
    resp_dut = self.android_devices[1]
    resp_dut.pretty_name = "Responder"

    asserts.skip_if(init_dut.aware_capabilities[aconsts.CAP_MAX_NDI_INTERFACES]
                    < len(sec_configs) or
                    resp_dut.aware_capabilities[aconsts.CAP_MAX_NDI_INTERFACES]
                    < len(sec_configs),
                    "Initiator or Responder do not support multiple NDIs")


    init_id, init_mac = autils.attach_with_identity(init_dut)
    resp_id, resp_mac = autils.attach_with_identity(resp_dut)

    # wait for for devices to synchronize with each other - there are no other
    # mechanisms to make sure this happens for OOB discovery (except retrying
    # to execute the data-path request)
    time.sleep(autils.WAIT_FOR_CLUSTER)

    resp_req_keys = []
    init_req_keys = []
    resp_aware_ifs = []
    init_aware_ifs = []
    resp_aware_ipv6s = []
    init_aware_ipv6s = []

    for sec in sec_configs:
      # Responder: request network
      resp_req_key = autils.request_network(resp_dut,
                                            autils.get_network_specifier(
                                                resp_dut, resp_id,
                                                aconsts.DATA_PATH_RESPONDER,
                                                init_mac, sec))
      resp_req_keys.append(resp_req_key)

      # Initiator: request network
      init_req_key = autils.request_network(init_dut,
                                            autils.get_network_specifier(
                                                init_dut, init_id,
                                                aconsts.DATA_PATH_INITIATOR,
                                                resp_mac, sec))
      init_req_keys.append(init_req_key)

      # Wait for network
      init_net_event = autils.wait_for_event_with_keys(
          init_dut, cconsts.EVENT_NETWORK_CALLBACK, autils.EVENT_NDP_TIMEOUT,
          (cconsts.NETWORK_CB_KEY_EVENT,
           cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED),
          (cconsts.NETWORK_CB_KEY_ID, init_req_key))
      resp_net_event = autils.wait_for_event_with_keys(
          resp_dut, cconsts.EVENT_NETWORK_CALLBACK, autils.EVENT_NDP_TIMEOUT,
          (cconsts.NETWORK_CB_KEY_EVENT,
           cconsts.NETWORK_CB_LINK_PROPERTIES_CHANGED),
          (cconsts.NETWORK_CB_KEY_ID, resp_req_key))

      resp_aware_ifs.append(
          resp_net_event["data"][cconsts.NETWORK_CB_KEY_INTERFACE_NAME])
      init_aware_ifs.append(
          init_net_event["data"][cconsts.NETWORK_CB_KEY_INTERFACE_NAME])

      resp_aware_ipv6s.append(
          autils.get_ipv6_addr(resp_dut, resp_aware_ifs[-1]))
      init_aware_ipv6s.append(
          autils.get_ipv6_addr(init_dut, init_aware_ifs[-1]))

    self.log.info("Initiator interfaces/ipv6: %s / %s", init_aware_ifs,
                  init_aware_ipv6s)
    self.log.info("Responder interfaces/ipv6: %s / %s", resp_aware_ifs,
                  resp_aware_ipv6s)

    # create threads, start them, and wait for all to finish
    base_port = 5000
    q = queue.Queue()
    threads = []
    for i in range(len(sec_configs)):
      threads.append(
          threading.Thread(
              target=self.run_iperf,
              args=(q, init_dut, resp_dut, resp_aware_ifs[i], init_aware_ipv6s[
                  i], base_port + i)))

    for thread in threads:
      thread.start()

    for thread in threads:
      thread.join()

    # release requests
    for resp_req_key in resp_req_keys:
      resp_dut.droid.connectivityUnregisterNetworkCallback(resp_req_key)
    for init_req_key in init_req_keys:
      init_dut.droid.connectivityUnregisterNetworkCallback(init_req_key)


    # collect data
    for i in range(len(sec_configs)):
      results[i] = {}
      result, data = q.get()
      asserts.assert_true(result,
                          "Failure starting/running iperf3 in client mode")
      self.log.debug(pprint.pformat(data))
      data_json = json.loads("".join(data))
      if "error" in data_json:
        asserts.fail(
            "iperf run failed: %s" % data_json["error"], extras=data_json)
      results[i]["tx_rate"] = data_json["end"]["sum_sent"]["bits_per_second"]
      results[i]["rx_rate"] = data_json["end"]["sum_received"][
        "bits_per_second"]
      self.log.info("iPerf3: Sent = %d bps Received = %d bps",
                    results[i]["tx_rate"], results[i]["rx_rate"])

  def test_iperf_max_ndi_aware_only_passphrases(self):
    """Test throughput for multiple NDIs configured with different passphrases.
    """
    results = {}
    self.run_iperf_max_ndi_aware_only(
        [self.PASSPHRASE, self.PASSPHRASE2], results=results)
    asserts.explicit_pass(
        "test_iperf_max_ndi_aware_only_passphrases passes", extras=results)
