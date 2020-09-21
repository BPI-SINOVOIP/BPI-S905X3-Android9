#!/usr/bin/env python3.4
#
#   Copyright 2017 - Google
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

import queue
import statistics
import time

from acts import asserts
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.wifi.rtt import rtt_const as rconsts

# arbitrary timeout for events
EVENT_TIMEOUT = 10


def decorate_event(event_name, id):
  return '%s_%d' % (event_name, id)


def wait_for_event(ad, event_name, timeout=EVENT_TIMEOUT):
  """Wait for the specified event or timeout.

  Args:
    ad: The android device
    event_name: The event to wait on
    timeout: Number of seconds to wait
  Returns:
    The event (if available)
  """
  prefix = ''
  if hasattr(ad, 'pretty_name'):
    prefix = '[%s] ' % ad.pretty_name
  try:
    event = ad.ed.pop_event(event_name, timeout)
    ad.log.info('%s%s: %s', prefix, event_name, event['data'])
    return event
  except queue.Empty:
    ad.log.info('%sTimed out while waiting for %s', prefix, event_name)
    asserts.fail(event_name)

def fail_on_event(ad, event_name, timeout=EVENT_TIMEOUT):
  """Wait for a timeout period and looks for the specified event - fails if it
  is observed.

  Args:
    ad: The android device
    event_name: The event to wait for (and fail on its appearance)
  """
  prefix = ''
  if hasattr(ad, 'pretty_name'):
    prefix = '[%s] ' % ad.pretty_name
  try:
    event = ad.ed.pop_event(event_name, timeout)
    ad.log.info('%sReceived unwanted %s: %s', prefix, event_name, event['data'])
    asserts.fail(event_name, extras=event)
  except queue.Empty:
    ad.log.info('%s%s not seen (as expected)', prefix, event_name)
    return


def config_privilege_override(dut, override_to_no_privilege):
  """Configure the device to override the permission check and to disallow any
  privileged RTT operations, e.g. disallow one-sided RTT to Responders (APs)
  which do not support IEEE 802.11mc.

  Args:
    dut: Device to configure.
    override_to_no_privilege: True to indicate no privileged ops, False for
                              default (which will allow privileged ops).
  """
  dut.adb.shell("cmd wifirtt set override_assume_no_privilege %d" % (
    1 if override_to_no_privilege else 0))


def get_rtt_constrained_results(scanned_networks, support_rtt):
  """Filter the input list and only return those networks which either support
  or do not support RTT (IEEE 802.11mc.)

  Args:
    scanned_networks: A list of networks from scan results.
      support_rtt: True - only return those APs which support RTT, False - only
                   return those APs which do not support RTT.

  Returns: a sub-set of the scanned_networks per support_rtt constraint.
  """
  matching_networks = []
  for network in scanned_networks:
    if support_rtt:
      if (rconsts.SCAN_RESULT_KEY_RTT_RESPONDER in network and
          network[rconsts.SCAN_RESULT_KEY_RTT_RESPONDER]):
        matching_networks.append(network)
    else:
      if (rconsts.SCAN_RESULT_KEY_RTT_RESPONDER not in network or
            not network[rconsts.SCAN_RESULT_KEY_RTT_RESPONDER]):
        matching_networks.append(network)

  return matching_networks


def scan_networks(dut):
  """Perform a scan and return scan results.

  Args:
    dut: Device under test.

  Returns: an array of scan results.
  """
  wutils.start_wifi_connection_scan(dut)
  return dut.droid.wifiGetScanResults()


def scan_with_rtt_support_constraint(dut, support_rtt, repeat=0):
  """Perform a scan and return scan results of APs: only those that support or
  do not support RTT (IEEE 802.11mc) - per the support_rtt parameter.

  Args:
    dut: Device under test.
    support_rtt: True - only return those APs which support RTT, False - only
                 return those APs which do not support RTT.
    repeat: Re-scan this many times to find an RTT supporting network.

  Returns: an array of scan results.
  """
  for i in range(repeat + 1):
    scan_results = scan_networks(dut)
    aps = get_rtt_constrained_results(scan_results, support_rtt)
    if len(aps) != 0:
      return aps

  return []


def select_best_scan_results(scans, select_count, lowest_rssi=-80):
  """Select the strongest 'select_count' scans in the input list based on
  highest RSSI. Exclude all very weak signals, even if results in a shorter
  list.

  Args:
    scans: List of scan results.
    select_count: An integer specifying how many scans to return at most.
    lowest_rssi: The lowest RSSI to accept into the output.
  Returns: a list of the strongest 'select_count' scan results from the scans
           list.
  """
  def takeRssi(element):
    return element['level']

  result = []
  scans.sort(key=takeRssi, reverse=True)
  for scan in scans:
    if len(result) == select_count:
      break
    if scan['level'] < lowest_rssi:
      break # rest are lower since we're sorted
    result.append(scan)

  return result


def validate_ap_result(scan_result, range_result):
  """Validate the range results:
  - Successful if AP (per scan result) support 802.11mc (allowed to fail
    otherwise)
  - MAC of result matches the BSSID

  Args:
    scan_result: Scan result for the AP
    range_result: Range result returned by the RTT API
  """
  asserts.assert_equal(scan_result[wutils.WifiEnums.BSSID_KEY], range_result[
    rconsts.EVENT_CB_RANGING_KEY_MAC_AS_STRING_BSSID], 'MAC/BSSID mismatch')
  if (rconsts.SCAN_RESULT_KEY_RTT_RESPONDER in scan_result and
      scan_result[rconsts.SCAN_RESULT_KEY_RTT_RESPONDER]):
    asserts.assert_true(range_result[rconsts.EVENT_CB_RANGING_KEY_STATUS] ==
                        rconsts.EVENT_CB_RANGING_STATUS_SUCCESS,
                        'Ranging failed for an AP which supports 802.11mc!')


def validate_ap_results(scan_results, range_results):
  """Validate an array of ranging results against the scan results used to
  trigger the range. The assumption is that the results are returned in the
  same order as the request (which were the scan results).

  Args:
    scan_results: Scans results used to trigger the range request
    range_results: Range results returned by the RTT API
  """
  asserts.assert_equal(
      len(scan_results),
      len(range_results),
      'Mismatch in length of scan results and range results')

  # sort first based on BSSID/MAC
  scan_results.sort(key=lambda x: x[wutils.WifiEnums.BSSID_KEY])
  range_results.sort(
      key=lambda x: x[rconsts.EVENT_CB_RANGING_KEY_MAC_AS_STRING_BSSID])

  for i in range(len(scan_results)):
    validate_ap_result(scan_results[i], range_results[i])


def validate_aware_mac_result(range_result, mac, description):
  """Validate the range result for an Aware peer specified with a MAC address:
  - Correct MAC address.

  The MAC addresses may contain ":" (which are ignored for the comparison) and
  may be in any case (which is ignored for the comparison).

  Args:
    range_result: Range result returned by the RTT API
    mac: MAC address of the peer
    description: Additional content to print on failure
  """
  mac1 = mac.replace(':', '').lower()
  mac2 = range_result[rconsts.EVENT_CB_RANGING_KEY_MAC_AS_STRING].replace(':',
                                                                  '').lower()
  asserts.assert_equal(mac1, mac2,
                       '%s: MAC mismatch' % description)

def validate_aware_peer_id_result(range_result, peer_id, description):
  """Validate the range result for An Aware peer specified with a Peer ID:
  - Correct Peer ID
  - MAC address information not available

  Args:
    range_result: Range result returned by the RTT API
    peer_id: Peer ID of the peer
    description: Additional content to print on failure
  """
  asserts.assert_equal(peer_id,
                       range_result[rconsts.EVENT_CB_RANGING_KEY_PEER_ID],
                       '%s: Peer Id mismatch' % description)
  asserts.assert_false(rconsts.EVENT_CB_RANGING_KEY_MAC in range_result,
                       '%s: MAC Address not empty!' % description)


def extract_stats(results, range_reference_mm, range_margin_mm, min_rssi,
    reference_lci=[], reference_lcr=[], summary_only=False):
  """Extract statistics from a list of RTT results. Returns a dictionary
   with results:
     - num_results (success or fails)
     - num_success_results
     - num_no_results (e.g. timeout)
     - num_failures
     - num_range_out_of_margin (only for successes)
     - num_invalid_rssi (only for successes)
     - distances: extracted list of distances
     - distance_std_devs: extracted list of distance standard-deviations
     - rssis: extracted list of RSSI
     - distance_mean
     - distance_std_dev (based on distance - ignoring the individual std-devs)
     - rssi_mean
     - rssi_std_dev
     - status_codes
     - lcis: extracted list of all of the individual LCI
     - lcrs: extracted list of all of the individual LCR
     - any_lci_mismatch: True/False - checks if all LCI results are identical to
                         the reference LCI.
     - any_lcr_mismatch: True/False - checks if all LCR results are identical to
                         the reference LCR.
     - num_attempted_measurements: extracted list of all of the individual
                                   number of attempted measurements.
     - num_successful_measurements: extracted list of all of the individual
                                    number of successful measurements.
     - invalid_num_attempted: True/False - checks if number of attempted
                              measurements is non-zero for successful results.
     - invalid_num_successful: True/False - checks if number of successful
                               measurements is non-zero for successful results.

  Args:
    results: List of RTT results.
    range_reference_mm: Reference value for the distance (in mm)
    range_margin_mm: Acceptable absolute margin for distance (in mm)
    min_rssi: Acceptable minimum RSSI value.
    reference_lci, reference_lcr: Reference values for LCI and LCR.
    summary_only: Only include summary keys (reduce size).

  Returns: A dictionary of stats.
  """
  stats = {}
  stats['num_results'] = 0
  stats['num_success_results'] = 0
  stats['num_no_results'] = 0
  stats['num_failures'] = 0
  stats['num_range_out_of_margin'] = 0
  stats['num_invalid_rssi'] = 0
  stats['any_lci_mismatch'] = False
  stats['any_lcr_mismatch'] = False
  stats['invalid_num_attempted'] = False
  stats['invalid_num_successful'] = False

  range_max_mm = range_reference_mm + range_margin_mm
  range_min_mm = range_reference_mm - range_margin_mm

  distances = []
  distance_std_devs = []
  rssis = []
  num_attempted_measurements = []
  num_successful_measurements = []
  status_codes = []
  lcis = []
  lcrs = []

  for i in range(len(results)):
    result = results[i]

    if result is None: # None -> timeout waiting for RTT result
      stats['num_no_results'] = stats['num_no_results'] + 1
      continue
    stats['num_results'] = stats['num_results'] + 1

    status_codes.append(result[rconsts.EVENT_CB_RANGING_KEY_STATUS])
    if status_codes[-1] != rconsts.EVENT_CB_RANGING_STATUS_SUCCESS:
      stats['num_failures'] = stats['num_failures'] + 1
      continue
    stats['num_success_results'] = stats['num_success_results'] + 1

    distance_mm = result[rconsts.EVENT_CB_RANGING_KEY_DISTANCE_MM]
    distances.append(distance_mm)
    if not range_min_mm <= distance_mm <= range_max_mm:
      stats['num_range_out_of_margin'] = stats['num_range_out_of_margin'] + 1
    distance_std_devs.append(
        result[rconsts.EVENT_CB_RANGING_KEY_DISTANCE_STD_DEV_MM])

    rssi = result[rconsts.EVENT_CB_RANGING_KEY_RSSI]
    rssis.append(rssi)
    if not min_rssi <= rssi <= 0:
      stats['num_invalid_rssi'] = stats['num_invalid_rssi'] + 1

    num_attempted = result[
      rconsts.EVENT_CB_RANGING_KEY_NUM_ATTEMPTED_MEASUREMENTS]
    num_attempted_measurements.append(num_attempted)
    if num_attempted == 0:
      stats['invalid_num_attempted'] = True

    num_successful = result[
      rconsts.EVENT_CB_RANGING_KEY_NUM_SUCCESSFUL_MEASUREMENTS]
    num_successful_measurements.append(num_successful)
    if num_successful == 0:
      stats['invalid_num_successful'] = True

    lcis.append(result[rconsts.EVENT_CB_RANGING_KEY_LCI])
    if (result[rconsts.EVENT_CB_RANGING_KEY_LCI] != reference_lci):
      stats['any_lci_mismatch'] = True
    lcrs.append(result[rconsts.EVENT_CB_RANGING_KEY_LCR])
    if (result[rconsts.EVENT_CB_RANGING_KEY_LCR] != reference_lcr):
      stats['any_lcr_mismatch'] = True

  if len(distances) > 0:
    stats['distance_mean'] = statistics.mean(distances)
  if len(distances) > 1:
    stats['distance_std_dev'] = statistics.stdev(distances)
  if len(rssis) > 0:
    stats['rssi_mean'] = statistics.mean(rssis)
  if len(rssis) > 1:
    stats['rssi_std_dev'] = statistics.stdev(rssis)
  if not summary_only:
    stats['distances'] = distances
    stats['distance_std_devs'] = distance_std_devs
    stats['rssis'] = rssis
    stats['num_attempted_measurements'] = num_attempted_measurements
    stats['num_successful_measurements'] = num_successful_measurements
    stats['status_codes'] = status_codes
    stats['lcis'] = lcis
    stats['lcrs'] = lcrs

  return stats


def run_ranging(dut, aps, iter_count, time_between_iterations,
    target_run_time_sec=0):
  """Executing ranging to the set of APs.

  Will execute a minimum of 'iter_count' iterations. Will continue to run
  until execution time (just) exceeds 'target_run_time_sec'.

  Args:
    dut: Device under test
    aps: A list of APs (Access Points) to range to.
    iter_count: (Minimum) Number of measurements to perform.
    time_between_iterations: Number of seconds to wait between iterations.
    target_run_time_sec: The target run time in seconds.

  Returns: a list of the events containing the RTT results (or None for a
  failed measurement).
  """
  max_peers = dut.droid.wifiRttMaxPeersInRequest()

  asserts.assert_true(len(aps) > 0, "Need at least one AP!")
  if len(aps) > max_peers:
    aps = aps[0:max_peers]

  events = {} # need to keep track per BSSID!
  for ap in aps:
    events[ap["BSSID"]] = []

  start_clock = time.time()
  iterations_done = 0
  run_time = 0
  while iterations_done < iter_count or (
      target_run_time_sec != 0 and run_time < target_run_time_sec):
    if iterations_done != 0 and time_between_iterations != 0:
      time.sleep(time_between_iterations)

    id = dut.droid.wifiRttStartRangingToAccessPoints(aps)
    try:
      event = dut.ed.pop_event(
        decorate_event(rconsts.EVENT_CB_RANGING_ON_RESULT, id), EVENT_TIMEOUT)
      range_results = event["data"][rconsts.EVENT_CB_RANGING_KEY_RESULTS]
      asserts.assert_equal(
          len(aps),
          len(range_results),
          'Mismatch in length of scan results and range results')
      for result in range_results:
        bssid = result[rconsts.EVENT_CB_RANGING_KEY_MAC_AS_STRING]
        asserts.assert_true(bssid in events,
                            "Result BSSID %s not in requested AP!?" % bssid)
        asserts.assert_equal(len(events[bssid]), iterations_done,
                             "Duplicate results for BSSID %s!?" % bssid)
        events[bssid].append(result)
    except queue.Empty:
      for ap in aps:
        events[ap["BSSID"]].append(None)

    iterations_done = iterations_done + 1
    run_time = time.time() - start_clock

  return events


def analyze_results(all_aps_events, rtt_reference_distance_mm,
    distance_margin_mm, min_expected_rssi, lci_reference, lcr_reference,
    summary_only=False):
  """Verifies the results of the RTT experiment.

  Args:
    all_aps_events: Dictionary of APs, each a list of RTT result events.
    rtt_reference_distance_mm: Expected distance to the AP (source of truth).
    distance_margin_mm: Accepted error marging in distance measurement.
    min_expected_rssi: Minimum acceptable RSSI value
    lci_reference, lcr_reference: Expected LCI/LCR values (arrays of bytes).
    summary_only: Only include summary keys (reduce size).
  """
  all_stats = {}
  for bssid, events in all_aps_events.items():
    stats = extract_stats(events, rtt_reference_distance_mm,
                          distance_margin_mm, min_expected_rssi,
                          lci_reference, lcr_reference, summary_only)
    all_stats[bssid] = stats
  return all_stats
