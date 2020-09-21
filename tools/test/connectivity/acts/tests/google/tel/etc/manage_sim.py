#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
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
"""
    Basic script for managing a JSON "database" file of SIM cards.
    It will look at the list of attached devices, and add their SIMs to a
    database.
    We expect to add much more functionality in the future.
"""

import argparse
import json
import acts.controllers.android_device as android_device
import acts.test_utils.tel.tel_defines as tel_defines
import acts.test_utils.tel.tel_lookup_tables as tel_lookup_tables
import acts.test_utils.tel.tel_test_utils as tel_test_utils


def get_active_sim_list(verbose_warnings=False):
    """ Get a dictionary of active sims across phones

    Args: verbose_warnings - print warnings as issues are encountered if True

    Returns:
        A dictionary with keys equivalent to the ICCIDs of each SIM containing
        information about that SIM
    """
    active_list = {}
    droid_list = android_device.get_all_instances()
    for droid_device in droid_list:
        droid = droid_device.get_droid(False)

        sub_info_list = droid.subscriptionGetActiveSubInfoList()
        if not sub_info_list:
            if verbose_warnings:
                print('No Valid Sim in {} {}! SimState = {}'.format(
                    droid_device.model, droid_device.serial, droid.telephonyGetSimState()))
            continue

        for sub_info in sub_info_list:
            print(sub_info)
            iccid = sub_info['iccId']
            if not sub_info['iccId']:
                continue

            active_list[iccid] = {}
            current = active_list[iccid]
            current['droid_serial'] = droid_device.serial

            sub_id = sub_info['subscriptionId']

            try:
                plmn_id = droid.telephonyGetSimOperatorForSubscription(sub_id)
                current[
                    'operator'] = tel_lookup_tables.operator_name_from_plmn_id(
                        plmn_id)
            except KeyError:
                if vebose_warnings:
                    print('Unknown Operator {}'.format(
                        droid.telephonyGetSimOperator()))
                current['operator'] = ''

            # TODO: add actual capability determination to replace the defaults
            current['capability'] = ['voice', 'ims', 'volte', 'vt', 'sms',
                                     'tethering', 'data']

            phone_num = droid.telephonyGetLine1NumberForSubscription(sub_id)
            if not phone_num:
                if verbose_warnings:
                    print('Please manually add a phone number for {}\n'.format(
                        iccid))
                current['phone_num'] = ''
            else:
                current['phone_num'] = tel_test_utils.phone_number_formatter(
                    phone_num, tel_defines.PHONE_NUMBER_STRING_FORMAT_11_DIGIT)
    return active_list


def add_sims(sim_card_file=None):
    if not sim_card_file:
        print('Error: file name is None.')
        return False
    try:
        f = open(sim_card_file, 'r')
        simconf = json.load(f)
        f.close()
    except FileNotFoundError:
        simconf = {}

    active_sims = get_active_sim_list(True)

    if not active_sims:
        print('No active SIMs, exiting')
        return False

    file_write_required = False

    for iccid in active_sims.keys():
        # Add new entry if not exist
        if iccid in simconf:
            print('Declining to add a duplicate entry: {}'.format(iccid))
            #TODO: Add support for "refreshing" an entry
            continue

        simconf[iccid] = {}
        current = simconf[iccid]
        file_write_required = True

        current['operator'] = active_sims[iccid]['operator']
        current['capability'] = active_sims[iccid]['capability']
        current['phone_num'] = active_sims[iccid]['phone_num']

    if file_write_required:
        f = open(sim_card_file, 'w')
        json.dump(simconf, f, indent=4, sort_keys=True)
        f.close()
    return True


def prune_sims(sim_card_file=None):
    try:
        f = open(sim_card_file, 'r')
        simconf = json.load(f)
        f.close()
    except FileNotFoundError:
        print('File {} not found.'.format(sim_card_file if sim_card_file else
                                          '<UNSPECIFIED>'))
        return False

    simconf_list = list(simconf.keys())
    active_list = get_active_sim_list().keys()
    delete_list = list(set(simconf_list).difference(set(active_list)))

    print('active phones: {}'.format(active_list))

    file_write_required = False

    if len(delete_list) > 0:
        for sim in delete_list:
            # prune
            print('Deleting the SIM entry: ', sim)
            del simconf[sim]
            file_write_required = True
    else:
        print('No entries to prune')

    if file_write_required:
        f = open(sim_card_file, 'w')
        json.dump(simconf, f, indent=4, sort_keys=True)
        f.close()
    return True


def dump_sims():
    active_list = get_active_sim_list()
    output_format = '{:32.32}{:20.20}{:12.12}{:10.10}'
    if not active_list:
        print('No active devices with sims!')
        return False

    print(output_format.format('ICCID', 'Android SN', 'Phone #', 'Carrier'))
    for iccid in active_list.keys():
        print(
            output_format.format(
                str(iccid), str(active_list[iccid]['droid_serial']), str(
                    active_list[iccid]['phone_num']), str(active_list[iccid][
                        'operator'])))


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description=(
        'Script to generate, augment and prune'
        ' SIM list'))
    parser.add_argument(
        '-f',
        '--file',
        default='./simcard_list.json',
        help='json file path',
        type=str)
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        '-a',
        '--append',
        help='(default) Append to the list of SIM entries',
        action='store_true')
    group.add_argument(
        '-p',
        '--prune',
        help='Prune the list of SIM entries',
        action='store_true')
    group.add_argument(
        '-d',
        '--dump',
        help='Dump a list of SIMs from devices',
        action='store_true')

    args = parser.parse_args()

    if args.prune:
        prune_sims(args.file)
    elif args.dump:
        dump_sims()
    # implies either no arguments or a && !p
    elif not args.prune and not args.dump:
        add_sims(args.file)
"""
Usage Examples

----------------------------------------------------------------
p3 manage_sim.py -h
usage: manage_sim.py [-h] [-f F] [-a | -p]

Script to generate, augment and prune SIM list

optional arguments:
  -h, --help      show this help message and exit
  -f F, --file F  name for json file
  -a              append to the list of SIM entries
  -d              dump a list of SIMs from all devices
  -p              prune the list of SIM entries

----------------------------------------------------------------
p3 manage_sim.py -f ./simcard_list.json -p
        OR
p3 manage_sim.py -p

Namespace(a=False, f='./simcard_list.json', p=True)
add_sims: 8901260222780922759
Please manually add a phone number for 8901260222780922759

active phones: 1
 ['8901260222780922759']
Deleting the SIM entry:  89148000001280331488
:
:
Deleting the SIM entry:  89014103277559059196

----------------------------------------------------------------
p3 manage_sim.py -f ./simcard_list.json -a
        OR
p3 manage_sim.py -a

Namespace(a=True, f='./simcard_list.json', p=False)
add_sims: 8901260222780922759
Please manually add a phone number for 8901260222780922759

----------------------------------------------------------------
"""
