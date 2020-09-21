#!/usr/bin/python

import copy
import datetime
import logging
import os
import pprint
import sys
import time

THIS_YEAR = datetime.datetime.now().year


def AnalyzeJavaLog(line):
    """Analyzes a log line printed by VTS Java harness.

    Args:
        line: string for a log line.

    Returns:
        boolean (True if it's a Java log line, False otherwise)
        string (module type)
        string (phase name)
        float (timestamp)
    """
    tokens = line.split()
    timestamp = 0
    if len(tokens) < 4:
        return False, None, None, 0
    # Example
    # 05-12 07:51:45 I/VtsMultiDeviceTest:
    date_time_str = "%s-%s %s" % (THIS_YEAR, tokens[0], tokens[1])
    try:
        timestamp = time.mktime(datetime.datetime.strptime(
            date_time_str, "%Y-%m-%d %H:%M:%S").timetuple())
    except ValueError as e:
        timestamp = -1

    if (len(tokens[2]) > 2 and tokens[2] == "D/ModuleDef:" and
            tokens[3][-1] == ":"):
        return True, tokens[3][:-1], tokens[4], timestamp
    return False, None, None, timestamp


def AnalyzePythonLog(line):
    """Analyzes a log line printed by VTS Python runner.

    Args:
        line: string for a log line.

    Returns:
        boolean (True if it's a Python log line, False otherwise)
        string (test module name)
        string (test case name)
        string ('begin' or 'end')
        float (timestamp)
    """
    tokens = line.split()
    timestamp = 0
    test_module_name = None
    if len(tokens) < 7:
        return False, test_module_name, None, None, timestamp
    # Example
    # [VtsKernelSelinuxFileApi] 05-12 07:51:32.916 INFO ...
    if len(tokens[0]) > 2 and tokens[0][0] == "[" and tokens[0][-1] == "]":
        test_module_name = tokens[0][1:-1]

        date_time_str = "%s-%s %s" % (THIS_YEAR, tokens[1], tokens[2])
        try:
            timestamp = time.mktime(datetime.datetime.strptime(
                date_time_str, "%Y-%m-%d %H:%M:%S.%f").timetuple())
        except ValueError as e:
            timestamp = -1
        if tokens[4] == "[Test" and tokens[5] == "Case]":
            test_case_name = tokens[6]
            if len(tokens) == 8 and tokens[7] in ["PASS", "FAIL", "SKIP", "ERROR"]:
                return True, test_module_name, test_case_name, "end", timestamp
            elif len(tokens) == 7:
                return True, test_module_name, test_case_name, "begin", timestamp
            else:
                assert False, "Error at '%s'" % line
        return True, test_module_name, None, None, timestamp
    return False, test_module_name, None, None, timestamp

# Java harness's execution stats.
java_exec_stats = {}
# Python runner's execution stats.
python_exec_stats = {}
flag_show_samples = False


def ProcessEvent(module_type, module_name, timestamp):
    """Processes a given Java log event."""
    if module_type in java_exec_stats:
        java_exec_stats[module_type]["sum"] += timestamp
        java_exec_stats[module_type]["count"] += 1
        if module_name in java_exec_stats[module_type]:
            java_exec_stats[module_type][module_name]["sum"] += timestamp
            java_exec_stats[module_type][module_name]["count"] += 1
            if flag_show_samples:
                java_exec_stats[module_type][module_name]["samples"].append(
                    timestamp)
        else:
            java_exec_stats[module_type][module_name] = {}
            java_exec_stats[module_type][module_name]["sum"] = timestamp
            java_exec_stats[module_type][module_name]["count"] = 1
            if flag_show_samples:
                java_exec_stats[module_type][module_name]["samples"] = [
                    timestamp
                ]
    else:
        java_exec_stats[module_type] = {}
        java_exec_stats[module_type]["sum"] = timestamp
        java_exec_stats[module_type]["count"] = 1


def FilterDict(input_dict, threashold):
    """Filters items in input_dict whose values are greater than threshold."""
    result_dict = {}
    org_dict = copy.copy(input_dict)
    for key, value in input_dict.iteritems():
        if value["value"] > threashold and value["state"] == "end":
            result_dict[key] = value["value"]
            del org_dict[key]
    return org_dict, result_dict


def main(log_file_path):
    """Analyzes the phases of an execution caught in the log.

    Args:
        log_file_path: string, log file path.
    """
    print "Log File:", log_file_path

    prev_java_module_type = None
    prev_java_module_name = None
    prev_timestamp = 0
    last_timestamp = 0
    python_exec_stats = {}

    with open(log_file_path, "r") as log_file:
        for line in log_file:
            (is_java_log, java_module_type, java_module_name,
             timestamp) = AnalyzeJavaLog(line)
            if is_java_log:
                last_timestamp = timestamp
                if prev_java_module_type:
                    ProcessEvent(prev_java_module_type, prev_java_module_name,
                                 timestamp - prev_timestamp)
                prev_java_module_type = java_module_type
                prev_java_module_name = java_module_name
                prev_timestamp = timestamp
            else:
                (is_python_log, test_module_name, test_case_name, event_type,
                 timestamp) = AnalyzePythonLog(line)
                if is_python_log:
                    last_timestamp = timestamp
                    if test_case_name:
                        if event_type == "begin":
                            if test_case_name not in python_exec_stats:
                                python_exec_stats[test_case_name] = {}
                                python_exec_stats[test_case_name][
                                    "value"] = timestamp
                                python_exec_stats[test_case_name][
                                    "state"] = "begin"
                            else:
                                for count in range(1, 1000):
                                    new_test_case_name = "%s_%s" % (
                                        test_case_name, str(count))
                                    if new_test_case_name not in python_exec_stats:
                                        python_exec_stats[
                                            new_test_case_name] = {}
                                        python_exec_stats[new_test_case_name][
                                            "value"] = timestamp
                                        python_exec_stats[new_test_case_name][
                                            "state"] = "begin"
                                        break
                                python_exec_stats[test_case_name] = {}
                                python_exec_stats[test_case_name][
                                    "value"] = timestamp
                                python_exec_stats[test_case_name][
                                    "state"] = "begin"
                        elif event_type == "end":
                            assert python_exec_stats[test_case_name][
                                "state"] == "begin"
                            python_exec_stats[test_case_name]["state"] = "end"
                            python_exec_stats[test_case_name]["value"] = (
                                timestamp -
                                python_exec_stats[test_case_name]["value"])
                            assert python_exec_stats[test_case_name] >= 0, (
                                "%s >= 0 ?" %
                                python_exec_stats[test_case_name])

    if prev_java_module_type:
        ProcessEvent(prev_java_module_type, prev_java_module_name,
                     last_timestamp - prev_timestamp)

    for threshold in [600, 300, 180, 120, 60, 30]:
        python_exec_stats, filtered_dict = FilterDict(python_exec_stats,
                                                      threshold)
        print "Python test cases took >%s seconds:" % threshold
        print filtered_dict.keys()
    print "Total Execution Time Breakdown:"
    pprint.pprint(java_exec_stats, width=1)


def usage():
    """Prints usage and exits."""
    print "Script to analyze the total execution of a VTS run."
    print "Usage: <this script> <VTS host log file path>"
    exit(-1)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        usage()
    main(sys.argv[1])
