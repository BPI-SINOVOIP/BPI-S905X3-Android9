#!/usr/bin/env python

import argparse

argparser = argparse.ArgumentParser(
    description="Get the highest reported board temperature (all sensors) in "
                "Celsius.")

group = argparser.add_mutually_exclusive_group()
group.add_argument("-m", "--maximum",
                   action="store_const",
                   const='Maximum',
                   dest="temperature_type",
                   help="Get the highest reported board temperature "
                        "from all sensors in Celsius.")
group.add_argument("-c", "--critical",
                   action="store_const",
                   const="Critical",
                   dest="temperature_type",
                   help="Get the critical temperature from all "
                        "sensors in Celsius.")
args = argparser.add_argument("-v", "--verbose",
                              action="store_true",
                              help="Show temperature type and value.")
argparser.set_defaults(temperature_type='all')
args = argparser.parse_args()

import common
from autotest_lib.client.bin import utils

TEMPERATURE_TYPE = {
    'Critical': utils.get_temperature_critical,
    'Maximum': utils.get_current_temperature_max,
}

def print_temperature(temperature_type):
    if args.verbose:
        print temperature_type,
    print TEMPERATURE_TYPE.get(temperature_type)()

if args.temperature_type == 'all':
    for temperature_type in TEMPERATURE_TYPE.keys():
        print_temperature(temperature_type)
else:
    print_temperature(args.temperature_type)
