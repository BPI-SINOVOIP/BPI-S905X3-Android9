# Copyright (c) 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module includes all moblab-related RPCs. These RPCs can only be run
on moblab.
"""

import ConfigParser
import common
import logging
import os
import re
import sys
import shutil
import socket
import StringIO
import subprocess
import time
import multiprocessing
import ctypes

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.frontend.afe import models
from autotest_lib.frontend.afe import rpc_utils
from autotest_lib.server import frontend
from autotest_lib.server.hosts import moblab_host
from chromite.lib import gs

_CONFIG = global_config.global_config
MOBLAB_BOTO_LOCATION = '/home/moblab/.boto'
CROS_CACHEDIR = '/mnt/moblab/cros_cache_apache'

# Google Cloud Storage bucket url regex pattern. The pattern is used to extract
# the bucket name from the bucket URL. For example, "gs://image_bucket/google"
# should result in a bucket name "image_bucket".
GOOGLE_STORAGE_BUCKET_URL_PATTERN = re.compile(
        r'gs://(?P<bucket>[a-zA-Z][a-zA-Z0-9-_]*)/?.*')

# Contants used in Json RPC field names.
_IMAGE_STORAGE_SERVER = 'image_storage_server'
_GS_ACCESS_KEY_ID = 'gs_access_key_id'
_GS_SECRET_ACCESS_KEY = 'gs_secret_access_key'
_RESULT_STORAGE_SERVER = 'results_storage_server'
_USE_EXISTING_BOTO_FILE = 'use_existing_boto_file'
_CLOUD_NOTIFICATION_ENABLED = 'cloud_notification_enabled'
_WIFI_AP_NAME = 'wifi_dut_ap_name'
_WIFI_AP_PASS = 'wifi_dut_ap_pass'

# Location where dhcp leases are stored.
_DHCPD_LEASES = '/var/lib/dhcp/dhcpd.leases'

# File where information about the current device is stored.
_ETC_LSB_RELEASE = '/etc/lsb-release'

# ChromeOS update engine client binary location
_UPDATE_ENGINE_CLIENT = '/usr/bin/update_engine_client'

# Full path to the correct gsutil command to run.
class GsUtil:
    """Helper class to find correct gsutil command."""
    _GSUTIL_CMD = None

    @classmethod
    def get_gsutil_cmd(cls):
      if not cls._GSUTIL_CMD:
         cls._GSUTIL_CMD = gs.GSContext.GetDefaultGSUtilBin(
           cache_dir=CROS_CACHEDIR)

      return cls._GSUTIL_CMD


class BucketPerformanceTestException(Exception):
  """Exception thrown when the command to test the bucket performance fails."""
  pass

@rpc_utils.moblab_only
def get_config_values():
    """Returns all config values parsed from global and shadow configs.

    Config values are grouped by sections, and each section is composed of
    a list of name value pairs.
    """
    sections =_CONFIG.get_sections()
    config_values = {}
    for section in sections:
        config_values[section] = _CONFIG.config.items(section)
    return rpc_utils.prepare_for_serialization(config_values)


def _write_config_file(config_file, config_values, overwrite=False):
    """Writes out a configuration file.

    @param config_file: The name of the configuration file.
    @param config_values: The ConfigParser object.
    @param ovewrite: Flag on if overwriting is allowed.
    """
    if not config_file:
        raise error.RPCException('Empty config file name.')
    if not overwrite and os.path.exists(config_file):
        raise error.RPCException('Config file already exists.')

    if config_values:
        with open(config_file, 'w') as config_file:
            config_values.write(config_file)


def _read_original_config():
    """Reads the orginal configuratino without shadow.

    @return: A configuration object, see global_config_class.
    """
    original_config = global_config.global_config_class()
    original_config.set_config_files(shadow_file='')
    return original_config


def _read_raw_config(config_file):
    """Reads the raw configuration from a configuration file.

    @param: config_file: The path of the configuration file.

    @return: A ConfigParser object.
    """
    shadow_config = ConfigParser.RawConfigParser()
    shadow_config.read(config_file)
    return shadow_config


def _get_shadow_config_from_partial_update(config_values):
    """Finds out the new shadow configuration based on a partial update.

    Since the input is only a partial config, we should not lose the config
    data inside the existing shadow config file. We also need to distinguish
    if the input config info overrides with a new value or reverts back to
    an original value.

    @param config_values: See get_moblab_settings().

    @return: The new shadow configuration as ConfigParser object.
    """
    original_config = _read_original_config()
    existing_shadow = _read_raw_config(_CONFIG.shadow_file)
    for section, config_value_list in config_values.iteritems():
        for key, value in config_value_list:
            if original_config.get_config_value(section, key,
                                                default='',
                                                allow_blank=True) != value:
                if not existing_shadow.has_section(section):
                    existing_shadow.add_section(section)
                existing_shadow.set(section, key, value)
            elif existing_shadow.has_option(section, key):
                existing_shadow.remove_option(section, key)
    return existing_shadow


def _update_partial_config(config_values):
    """Updates the shadow configuration file with a partial config udpate.

    @param config_values: See get_moblab_settings().
    """
    existing_config = _get_shadow_config_from_partial_update(config_values)
    _write_config_file(_CONFIG.shadow_file, existing_config, True)


@rpc_utils.moblab_only
def update_config_handler(config_values):
    """Update config values and override shadow config.

    @param config_values: See get_moblab_settings().
    """
    original_config = _read_original_config()
    new_shadow = ConfigParser.RawConfigParser()
    for section, config_value_list in config_values.iteritems():
        for key, value in config_value_list:
            if original_config.get_config_value(section, key,
                                                default='',
                                                allow_blank=True) != value:
                if not new_shadow.has_section(section):
                    new_shadow.add_section(section)
                new_shadow.set(section, key, value)

    if not _CONFIG.shadow_file or not os.path.exists(_CONFIG.shadow_file):
        raise error.RPCException('Shadow config file does not exist.')
    _write_config_file(_CONFIG.shadow_file, new_shadow, True)

    # TODO (sbasi) crbug.com/403916 - Remove the reboot command and
    # instead restart the services that rely on the config values.
    os.system('sudo reboot')


@rpc_utils.moblab_only
def reset_config_settings():
    """Reset moblab shadow config."""
    with open(_CONFIG.shadow_file, 'w') as config_file:
        pass
    os.system('sudo reboot')


@rpc_utils.moblab_only
def reboot_moblab():
    """Simply reboot the device."""
    os.system('sudo reboot')


@rpc_utils.moblab_only
def set_boto_key(boto_key):
    """Update the boto_key file.

    @param boto_key: File name of boto_key uploaded through handle_file_upload.
    """
    if not os.path.exists(boto_key):
        raise error.RPCException('Boto key: %s does not exist!' % boto_key)
    shutil.copyfile(boto_key, moblab_host.MOBLAB_BOTO_LOCATION)


@rpc_utils.moblab_only
def set_service_account_credential(service_account_filename):
    """Update the service account credential file.

    @param service_account_filename: Name of uploaded file through
            handle_file_upload.
    """
    if not os.path.exists(service_account_filename):
        raise error.RPCException(
                'Service account file: %s does not exist!' %
                service_account_filename)
    shutil.copyfile(
            service_account_filename,
            moblab_host.MOBLAB_SERVICE_ACCOUNT_LOCATION)


@rpc_utils.moblab_only
def set_launch_control_key(launch_control_key):
    """Update the launch_control_key file.

    @param launch_control_key: File name of launch_control_key uploaded through
            handle_file_upload.
    """
    if not os.path.exists(launch_control_key):
        raise error.RPCException('Launch Control key: %s does not exist!' %
                                 launch_control_key)
    shutil.copyfile(launch_control_key,
                    moblab_host.MOBLAB_LAUNCH_CONTROL_KEY_LOCATION)
    # Restart the devserver service.
    os.system('sudo restart moblab-devserver-init')


###########Moblab Config Wizard RPCs #######################
def _get_public_ip_address(socket_handle):
    """Gets the public IP address.

    Connects to Google DNS server using a socket and gets the preferred IP
    address from the connection.

    @param: socket_handle: a unix socket.

    @return: public ip address as string.
    """
    try:
        socket_handle.settimeout(1)
        socket_handle.connect(('8.8.8.8', 53))
        socket_name = socket_handle.getsockname()
        if socket_name is not None:
            logging.info('Got socket name from UDP socket.')
            return socket_name[0]
        logging.warn('Created UDP socket but with no socket_name.')
    except socket.error:
        logging.warn('Could not get socket name from UDP socket.')
    return None


def _get_network_info():
    """Gets the network information.

    TCP socket is used to test the connectivity. If there is no connectivity,
    try to get the public IP with UDP socket.

    @return: a tuple as (public_ip_address, connected_to_internet).
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ip = _get_public_ip_address(s)
    if ip is not None:
        logging.info('Established TCP connection with well known server.')
        return (ip, True)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return (_get_public_ip_address(s), False)


@rpc_utils.moblab_only
def get_network_info():
    """Returns the server ip addresses, and if the server connectivity.

    The server ip addresses as an array of strings, and the connectivity as a
    flag.
    """
    network_info = {}
    info = _get_network_info()
    if info[0] is not None:
        network_info['server_ips'] = [info[0]]
    network_info['is_connected'] = info[1]

    return rpc_utils.prepare_for_serialization(network_info)


# Gets the boto configuration.
def _get_boto_config():
    """Reads the boto configuration from the boto file.

    @return: Boto configuration as ConfigParser object.
    """
    boto_config = ConfigParser.ConfigParser()
    boto_config.read(MOBLAB_BOTO_LOCATION)
    return boto_config


@rpc_utils.moblab_only
def get_cloud_storage_info():
    """RPC handler to get the cloud storage access information.
    """
    cloud_storage_info = {}
    value =_CONFIG.get_config_value('CROS', _IMAGE_STORAGE_SERVER)
    if value is not None:
        cloud_storage_info[_IMAGE_STORAGE_SERVER] = value
    value = _CONFIG.get_config_value('CROS', _RESULT_STORAGE_SERVER,
            default=None)
    if value is not None:
        cloud_storage_info[_RESULT_STORAGE_SERVER] = value

    boto_config = _get_boto_config()
    sections = boto_config.sections()

    if sections:
        cloud_storage_info[_USE_EXISTING_BOTO_FILE] = True
    else:
        cloud_storage_info[_USE_EXISTING_BOTO_FILE] = False
    if 'Credentials' in sections:
        options = boto_config.options('Credentials')
        if _GS_ACCESS_KEY_ID in options:
            value = boto_config.get('Credentials', _GS_ACCESS_KEY_ID)
            cloud_storage_info[_GS_ACCESS_KEY_ID] = value
        if _GS_SECRET_ACCESS_KEY in options:
            value = boto_config.get('Credentials', _GS_SECRET_ACCESS_KEY)
            cloud_storage_info[_GS_SECRET_ACCESS_KEY] = value

    return rpc_utils.prepare_for_serialization(cloud_storage_info)


def _get_bucket_name_from_url(bucket_url):
    """Gets the bucket name from a bucket url.

    @param: bucket_url: the bucket url string.
    """
    if bucket_url:
        match = GOOGLE_STORAGE_BUCKET_URL_PATTERN.match(bucket_url)
        if match:
            return match.group('bucket')
    return None


def _is_valid_boto_key(key_id, key_secret, directory):
  try:
      _run_bucket_performance_test(key_id, key_secret, directory)
  except BucketPerformanceTestException as e:
       return(False, str(e))
  return(True, None)


def _validate_cloud_storage_info(cloud_storage_info):
    """Checks if the cloud storage information is valid.

    @param: cloud_storage_info: The JSON RPC object for cloud storage info.

    @return: A tuple as (valid_boolean, details_string).
    """
    valid = True
    details = None
    if not cloud_storage_info[_USE_EXISTING_BOTO_FILE]:
        key_id = cloud_storage_info[_GS_ACCESS_KEY_ID]
        key_secret = cloud_storage_info[_GS_SECRET_ACCESS_KEY]
        valid, details = _is_valid_boto_key(
            key_id, key_secret, cloud_storage_info[_IMAGE_STORAGE_SERVER])
    return (valid, details)


def _create_operation_status_response(is_ok, details):
    """Helper method to create a operation status reponse.

    @param: is_ok: Boolean for if the operation is ok.
    @param: details: A detailed string.

    @return: A serialized JSON RPC object.
    """
    status_response = {'status_ok': is_ok}
    if details:
        status_response['status_details'] = details
    return rpc_utils.prepare_for_serialization(status_response)


@rpc_utils.moblab_only
def validate_cloud_storage_info(cloud_storage_info):
    """RPC handler to check if the cloud storage info is valid.

    @param cloud_storage_info: The JSON RPC object for cloud storage info.
    """
    valid, details = _validate_cloud_storage_info(cloud_storage_info)
    return _create_operation_status_response(valid, details)


@rpc_utils.moblab_only
def submit_wizard_config_info(cloud_storage_info, wifi_info):
    """RPC handler to submit the cloud storage info.

    @param cloud_storage_info: The JSON RPC object for cloud storage info.
    @param wifi_info: The JSON RPC object for DUT wifi info.
    """
    config_update = {}
    config_update['CROS'] = [
        (_IMAGE_STORAGE_SERVER, cloud_storage_info[_IMAGE_STORAGE_SERVER]),
        (_RESULT_STORAGE_SERVER, cloud_storage_info[_RESULT_STORAGE_SERVER])
    ]
    config_update['MOBLAB'] = [
        (_WIFI_AP_NAME, wifi_info.get(_WIFI_AP_NAME) or ''),
        (_WIFI_AP_PASS, wifi_info.get(_WIFI_AP_PASS) or '')
    ]
    _update_partial_config(config_update)

    if not cloud_storage_info[_USE_EXISTING_BOTO_FILE]:
        boto_config = ConfigParser.RawConfigParser()
        boto_config.add_section('Credentials')
        boto_config.set('Credentials', _GS_ACCESS_KEY_ID,
                        cloud_storage_info[_GS_ACCESS_KEY_ID])
        boto_config.set('Credentials', _GS_SECRET_ACCESS_KEY,
                        cloud_storage_info[_GS_SECRET_ACCESS_KEY])
        _write_config_file(MOBLAB_BOTO_LOCATION, boto_config, True)

    _CONFIG.parse_config_file()
    _enable_notification_using_credentials_in_bucket()
    services = ['moblab-devserver-init',
    'moblab-devserver-cleanup-init', 'moblab-gsoffloader_s-init',
    'moblab-scheduler-init', 'moblab-gsoffloader-init']
    cmd = 'export ATEST_RESULTS_DIR=/usr/local/autotest/results;'
    cmd += 'sudo stop ' + ';sudo stop '.join(services)
    cmd += ';sudo start ' + ';sudo start '.join(services)
    cmd += ';sudo apache2 -k graceful'
    logging.info(cmd)
    try:
        utils.run(cmd)
    except error.CmdError as e:
        logging.error(e)
        # if all else fails reboot the device.
        utils.run('sudo reboot')

    return _create_operation_status_response(True, None)


@rpc_utils.moblab_only
def get_version_info():
    """ RPC handler to get informaiton about the version of the moblab.

    @return: A serialized JSON RPC object.
    """
    lines = open(_ETC_LSB_RELEASE).readlines()
    version_response = {
        x.split('=')[0]: x.split('=')[1] for x in lines if '=' in x}
    version_response['MOBLAB_ID'] = utils.get_moblab_id();
    version_response['MOBLAB_SERIAL_NUMBER'] = (
        utils.get_moblab_serial_number())
    _check_for_system_update()
    update_status = _get_system_update_status()
    version_response['MOBLAB_UPDATE_VERSION'] = update_status['NEW_VERSION']
    version_response['MOBLAB_UPDATE_STATUS'] = update_status['CURRENT_OP']
    version_response['MOBLAB_UPDATE_PROGRESS'] = update_status['PROGRESS']
    return rpc_utils.prepare_for_serialization(version_response)


@rpc_utils.moblab_only
def update_moblab():
    """ RPC call to update and reboot moblab """
    _install_system_update()


def _check_for_system_update():
    """ Run the ChromeOS update client to check update server for an
    update. If an update exists, the update client begins downloading it
    in the background
    """
    # sudo is required to run the update client
    subprocess.call(['sudo', _UPDATE_ENGINE_CLIENT, '--check_for_update'])
    # wait for update engine to finish checking
    tries = 0
    while ('CHECKING_FOR_UPDATE' in _get_system_update_status()['CURRENT_OP']
            and tries < 10):
        time.sleep(.1)
        tries = tries + 1

def _get_system_update_status():
    """ Run the ChromeOS update client to check status on a
    pending/downloading update

    @return: A dictionary containing {
        PROGRESS: str containing percent progress of an update download
        CURRENT_OP: str current status of the update engine,
            ex UPDATE_STATUS_UPDATED_NEED_REBOOT
        NEW_SIZE: str size of the update
        NEW_VERSION: str version number for the update
        LAST_CHECKED_TIME: str unix time stamp of the last update check
    }
    """
    # sudo is required to run the update client
    cmd_out = subprocess.check_output(
        ['sudo' ,_UPDATE_ENGINE_CLIENT, '--status'])
    split_lines = [x.split('=') for x in cmd_out.strip().split('\n')]
    status = dict((key, val) for [key, val] in split_lines)
    return status


def _install_system_update():
    """ Installs a ChromeOS update, will cause the system to reboot
    """
    # sudo is required to run the update client
    # first run a blocking command to check, fetch, prepare an update
    # then check if a reboot is needed
    try:
        subprocess.check_call(['sudo', _UPDATE_ENGINE_CLIENT, '--update'])
        # --is_reboot_needed returns 0 if a reboot is required
        subprocess.check_call(
            ['sudo', _UPDATE_ENGINE_CLIENT, '--is_reboot_needed'])
        subprocess.call(['sudo', _UPDATE_ENGINE_CLIENT, '--reboot'])

    except subprocess.CalledProcessError as e:
        update_error = subprocess.check_output(
            ['sudo', _UPDATE_ENGINE_CLIENT, '--last_attempt_error'])
        raise error.RPCException(update_error)


@rpc_utils.moblab_only
def get_connected_dut_info():
    """ RPC handler to get informaiton about the DUTs connected to the moblab.

    @return: A serialized JSON RPC object.
    """
    # Make a list of the connected DUT's
    leases = _get_dhcp_dut_leases()


    connected_duts = _test_all_dut_connections(leases)

    # Get a list of the AFE configured DUT's
    hosts = list(rpc_utils.get_host_query((), False, True, {}))
    models.Host.objects.populate_relationships(hosts, models.Label,
                                               'label_list')
    configured_duts = {}
    for host in hosts:
        labels = [label.name for label in host.label_list]
        labels.sort()
        for host_attribute in host.hostattribute_set.all():
              labels.append("ATTR:(%s=%s)" % (host_attribute.attribute,
                                              host_attribute.value))
        configured_duts[host.hostname] = ', '.join(labels)

    return rpc_utils.prepare_for_serialization(
            {'configured_duts': configured_duts,
             'connected_duts': connected_duts})


def _get_dhcp_dut_leases():
     """ Extract information about connected duts from the dhcp server.

     @return: A dict of ipaddress to mac address for each device connected.
     """
     lease_info = open(_DHCPD_LEASES).read()

     leases = {}
     for lease in lease_info.split('lease'):
         if lease.find('binding state active;') != -1:
             ipaddress = lease.split('\n')[0].strip(' {')
             last_octet = int(ipaddress.split('.')[-1].strip())
             if last_octet > 150:
                 continue
             mac_address_search = re.search('hardware ethernet (.*);', lease)
             if mac_address_search:
                 leases[ipaddress] = mac_address_search.group(1)
     return leases

def _test_all_dut_connections(leases):
    """ Test ssh connection of all connected DUTs in parallel

    @param leases: dict containing key value pairs of ip and mac address

    @return: dict containing {
        ip: {mac_address:[string], ssh_connection_ok:[boolean]}
    }
    """
    # target function for parallel process
    def _test_dut(ip, result):
        result.value = _test_dut_ssh_connection(ip)

    processes = []
    for ip in leases:
        # use a shared variable to get the ssh test result from child process
        ssh_test_result = multiprocessing.Value(ctypes.c_bool)
        # create a subprocess to test each DUT
        process = multiprocessing.Process(
            target=_test_dut, args=(ip, ssh_test_result))
        process.start()

        processes.append({
            'ip': ip,
            'ssh_test_result': ssh_test_result,
            'process': process
        })

    connected_duts = {}
    for process in processes:
        process['process'].join()
        ip = process['ip']
        connected_duts[ip] = {
            'mac_address': leases[ip],
            'ssh_connection_ok': process['ssh_test_result'].value
        }

    return connected_duts


def _test_dut_ssh_connection(ip):
    """ Test if a connected dut is accessible via ssh.
    The primary use case is to verify that the dut has a test image.

    @return: True if the ssh connection is good False else
    """
    cmd = ('ssh -o ConnectTimeout=2 -o StrictHostKeyChecking=no '
            "root@%s 'timeout 2 cat /etc/lsb-release'") % ip
    try:
        release = subprocess.check_output(cmd, shell=True)
        return 'CHROMEOS_RELEASE_APPID' in release
    except:
        return False


@rpc_utils.moblab_only
def add_moblab_dut(ipaddress):
    """ RPC handler to add a connected DUT to autotest.

    @param ipaddress: IP address of the DUT.

    @return: A string giving information about the status.
    """
    cmd = '/usr/local/autotest/cli/atest host create %s &' % ipaddress
    subprocess.call(cmd, shell=True)
    return (True, 'DUT %s added to Autotest' % ipaddress)


@rpc_utils.moblab_only
def remove_moblab_dut(ipaddress):
    """ RPC handler to remove DUT entry from autotest.

    @param ipaddress: IP address of the DUT.

    @return: True if the command succeeds without an exception
    """
    models.Host.smart_get(ipaddress).delete()
    return (True, 'DUT %s deleted from Autotest' % ipaddress)


@rpc_utils.moblab_only
def add_moblab_label(ipaddress, label_name):
    """ RPC handler to add a label in autotest to a DUT entry.

    @param ipaddress: IP address of the DUT.
    @param label_name: The label name.

    @return: A string giving information about the status.
    """
    # Try to create the label in case it does not already exist.
    label = None
    try:
        label = models.Label.add_object(name=label_name)
    except:
        label = models.Label.smart_get(label_name)
        if label.is_replaced_by_static():
            raise error.UnmodifiableLabelException(
                    'Failed to add label "%s" because it is a static label. '
                    'Use go/chromeos-skylab-inventory-tools to add this '
                    'label.' % label.name)

    host_obj = models.Host.smart_get(ipaddress)
    if label:
        label.host_set.add(host_obj)
        return (True, 'Added label %s to DUT %s' % (label_name, ipaddress))
    return (False,
            'Failed to add label %s to DUT %s' % (label_name, ipaddress))


@rpc_utils.moblab_only
def remove_moblab_label(ipaddress, label_name):
    """ RPC handler to remove a label in autotest from a DUT entry.

    @param ipaddress: IP address of the DUT.
    @param label_name: The label name.

    @return: A string giving information about the status.
    """
    host_obj = models.Host.smart_get(ipaddress)
    label = models.Label.smart_get(label_name)
    if label.is_replaced_by_static():
        raise error.UnmodifiableLabelException(
                    'Failed to remove label "%s" because it is a static label. '
                    'Use go/chromeos-skylab-inventory-tools to remove this '
                    'label.' % label.name)

    label.host_set.remove(host_obj)
    return (True, 'Removed label %s from DUT %s' % (label_name, ipaddress))


@rpc_utils.moblab_only
def set_host_attrib(ipaddress, attribute, value):
    """ RPC handler to set an attribute of a host.

    @param ipaddress: IP address of the DUT.
    @param attribute: string name of attribute
    @param value: string, or None to delete an attribute

    @return: True if the command succeeds without an exception
    """
    host_obj = models.Host.smart_get(ipaddress)
    host_obj.set_or_delete_attribute(attribute, value)
    return (True, 'Updated attribute %s to %s on DUT %s' % (
        attribute, value, ipaddress))


@rpc_utils.moblab_only
def delete_host_attrib(ipaddress, attribute):
    """ RPC handler to delete an attribute of a host.

    @param ipaddress: IP address of the DUT.
    @param attribute: string name of attribute

    @return: True if the command succeeds without an exception
    """
    host_obj = models.Host.smart_get(ipaddress)
    host_obj.set_or_delete_attribute(attribute, None)
    return (True, 'Deleted attribute %s from DUT %s' % (
        attribute, ipaddress))


def _get_connected_dut_labels(requested_label, only_first_label=True):
    """ Query the DUT's attached to the moblab and return a filtered list
        of labels.

    @param requested_label:  the label name you are requesting.
    @param only_first_label:  if the device has the same label name multiple
                              times only return the first label value in the
                              list.

    @return: A de-duped list of requested dut labels attached to the moblab.
    """
    hosts = list(rpc_utils.get_host_query((), False, True, {}))
    if not hosts:
        return []
    models.Host.objects.populate_relationships(hosts, models.Label,
                                               'label_list')
    labels = set()
    for host in hosts:
        for label in host.label_list:
            if requested_label in label.name:
                labels.add(label.name.replace(requested_label, ''))
                if only_first_label:
                    break
    return list(labels)

def _get_connected_dut_board_models():
    """ Get the boards and their models of attached DUTs

    @return: A de-duped list of dut board/model attached to the moblab
    format: [
        {
            "board": "carl",
            "model": "bruce"
        },
        {
            "board": "veyron_minnie",
            "model": "veyron_minnie"
        }
    ]
    """
    hosts = list(rpc_utils.get_host_query((), False, True, {}))
    if not hosts:
        return []
    models.Host.objects.populate_relationships(hosts, models.Label,
                                               'label_list')
    model_board_map = dict()
    for host in hosts:
        model = ''
        board = ''
        for label in host.label_list:
            if 'model:' in label.name:
                model = label.name.replace('model:', '')
            elif 'board:' in label.name:
                board = label.name.replace('board:', '')
        model_board_map[model] = board

    board_models_list = []
    for model in sorted(model_board_map.keys()):
        board_models_list.append({
            'model': model,
            'board': model_board_map[model]
        })
    return board_models_list


@rpc_utils.moblab_only
def get_connected_boards():
    """ RPC handler to get a list of the boards connected to the moblab.

    @return: A de-duped list of board types attached to the moblab.
    """
    return _get_connected_dut_board_models()


@rpc_utils.moblab_only
def get_connected_pools():
    """ RPC handler to get a list of the pools labels on the DUT's connected.

    @return: A de-duped list of pool labels.
    """
    pools = _get_connected_dut_labels("pool:", False)
    pools.sort()
    return pools


@rpc_utils.moblab_only
def get_builds_for_board(board_name):
    """ RPC handler to find the most recent builds for a board.


    @param board_name: The name of a connected board.
    @return: A list of string with the most recent builds for the latest
             three milestones.
    """
    return _get_builds_for_in_directory(board_name + '-release',
                                        milestone_limit=4)


@rpc_utils.moblab_only
def get_firmware_for_board(board_name):
    """ RPC handler to find the most recent firmware for a board.


    @param board_name: The name of a connected board.
    @return: A list of strings with the most recent firmware builds for the
             latest three milestones.
    """
    return _get_builds_for_in_directory(board_name + '-firmware')


def _get_sortable_build_number(sort_key):
    """ Converts a build number line cyan-release/R59-9460.27.0 into an integer.

        To be able to sort a list of builds you need to convert the build number
        into an integer so it can be compared correctly to other build.

        cyan-release/R59-9460.27.0 =>  5909460027000

        If the sort key is not recognised as a build number 1 will be returned.

    @param sort_key: A string that represents a build number like
                     cyan-release/R59-9460.27.0
    @return: An integer that represents that build number or 1 if not recognised
             as a build.
    """
    build_number = re.search('.*/R([0-9]*)-([0-9]*)\.([0-9]*)\.([0-9]*)',
                             sort_key)
    if not build_number or not len(build_number.groups()) == 4:
      return 1
    return int("%d%05d%03d%03d" % (int(build_number.group(1)),
                                   int(build_number.group(2)),
                                   int(build_number.group(3)),
                                   int(build_number.group(4))))

def _get_builds_for_in_directory(directory_name, milestone_limit=3,
                                 build_limit=20):
    """ Fetch the most recent builds for the last three milestones from gcs.


    @param directory_name: The sub-directory under the configured GCS image
                           storage bucket to search.


    @return: A string list no longer than <milestone_limit> x <build_limit>
             items, containing the most recent <build_limit> builds from the
             last milestone_limit milestones.
    """
    output = StringIO.StringIO()
    gs_image_location =_CONFIG.get_config_value('CROS', _IMAGE_STORAGE_SERVER)
    try:
        utils.run(GsUtil.get_gsutil_cmd(),
                  args=('ls', gs_image_location + directory_name),
                  stdout_tee=output)
    except error.CmdError as e:
        error_text = ('Failed to list builds from %s.\n'
                'Did you configure your boto key? Try running the config '
                'wizard again.\n\n%s') % ((gs_image_location + directory_name),
                    e.result_obj.stderr)
        raise error.RPCException(error_text)
    lines = output.getvalue().split('\n')
    output.close()
    builds = [line.replace(gs_image_location,'').strip('/ ')
              for line in lines if line != '']
    build_matcher = re.compile(r'^.*\/R([0-9]*)-.*')
    build_map = {}
    for build in builds:
        match = build_matcher.match(build)
        if match:
            milestone = match.group(1)
            if milestone not in build_map:
                build_map[milestone] = []
            build_map[milestone].append(build)
    milestones = build_map.keys()
    milestones.sort()
    milestones.reverse()
    build_list = []
    for milestone in milestones[:milestone_limit]:
         builds = build_map[milestone]
         builds.sort(key=_get_sortable_build_number)
         builds.reverse()
         build_list.extend(builds[:build_limit])
    return build_list


def _run_bucket_performance_test(key_id, key_secret, bucket_name,
                                 test_size='1M', iterations='1',
                                 result_file='/tmp/gsutil_perf.json'):
    """Run a gsutil perfdiag on a supplied bucket and output the results"

       @param key_id: boto key of the bucket to be accessed
       @param key_secret: boto secret of the bucket to be accessed
       @param bucket_name: bucket to be tested.
       @param test_size: size of file to use in test, see gsutil perfdiag help.
       @param iterations: number of times each test is run.
       @param result_file: name of file to write results out to.

       @return None
       @raises BucketPerformanceTestException if the command fails.
    """
    try:
      utils.run(GsUtil.get_gsutil_cmd(), args=(
          '-o', 'Credentials:gs_access_key_id=%s' % key_id,
          '-o', 'Credentials:gs_secret_access_key=%s' % key_secret,
          'perfdiag', '-s', test_size, '-o', result_file,
          '-n', iterations,
          bucket_name))
    except error.CmdError as e:
       logging.error(e)
       # Extract useful error from the stacktrace
       errormsg = str(e)
       start_error_pos = errormsg.find("<Error>")
       end_error_pos = errormsg.find("</Error>", start_error_pos)
       extracted_error_msg = errormsg[start_error_pos:end_error_pos]
       raise BucketPerformanceTestException(
           extracted_error_msg if extracted_error_msg else errormsg)
    # TODO(haddowk) send the results to the cloud console when that feature is
    # enabled.


# TODO(haddowk) Change suite_args name to "test_filter_list" or similar. May
# also need to make changes at MoblabRpcHelper.java
@rpc_utils.moblab_only
def run_suite(board, build, suite, model=None, ro_firmware=None,
              rw_firmware=None, pool=None, suite_args=None, bug_id=None,
              part_id=None):
    """ RPC handler to run a test suite.

    @param board: a board name connected to the moblab.
    @param build: a build name of a build in the GCS.
    @param suite: the name of a suite to run
    @param model: a board model name connected to the moblab.
    @param ro_firmware: Optional ro firmware build number to use.
    @param rw_firmware: Optional rw firmware build number to use.
    @param pool: Optional pool name to run the suite in.
    @param suite_args: Arguments to be used in the suite control file.
    @param bug_id: Optilnal bug ID used for AVL qualification process.
    @param part_id: Optilnal part ID used for AVL qualification
    process.

    @return: None
    """
    builds = {'cros-version': build}
    processed_suite_args = dict()
    if rw_firmware:
        builds['fwrw-version'] = rw_firmware
    if ro_firmware:
        builds['fwro-version'] = ro_firmware
    if suite_args:
        processed_suite_args['tests'] = \
            [s.strip() for s in suite_args.split(',')]
    if bug_id:
        processed_suite_args['bug_id'] = bug_id
    if part_id:
        processed_suite_args['part_id'] = part_id

    # set processed_suite_args to None instead of empty dict when there is no
    # argument in processed_suite_args
    if len(processed_suite_args) == 0:
        processed_suite_args = None

    test_args = {}

    ap_name =_CONFIG.get_config_value('MOBLAB', _WIFI_AP_NAME, default=None)
    test_args['ssid'] = ap_name
    ap_pass =_CONFIG.get_config_value('MOBLAB', _WIFI_AP_PASS, default='')
    test_args['wifipass'] = ap_pass

    afe = frontend.AFE(user='moblab')
    afe.run('create_suite_job', board=board, builds=builds, name=suite,
            pool=pool, run_prod_code=False, test_source_build=build,
            wait_for_results=True, suite_args=processed_suite_args,
            test_args=test_args, job_retry=True, max_retries=sys.maxint,
            model=model)


def _enable_notification_using_credentials_in_bucket():
    """ Check and enable cloud notification if a credentials file exits.
    @return: None
    """
    gs_image_location =_CONFIG.get_config_value('CROS', _IMAGE_STORAGE_SERVER)
    try:
        utils.run(GsUtil.get_gsutil_cmd(), args=(
            'cp', gs_image_location + 'pubsub-key-do-not-delete.json', '/tmp'))
        # This runs the copy as moblab user
        shutil.copyfile('/tmp/pubsub-key-do-not-delete.json',
                        moblab_host.MOBLAB_SERVICE_ACCOUNT_LOCATION)

    except error.CmdError as e:
        logging.error(e)
    else:
        logging.info('Enabling cloud notifications')
        config_update = {}
        config_update['CROS'] = [(_CLOUD_NOTIFICATION_ENABLED, True)]
        _update_partial_config(config_update)


@rpc_utils.moblab_only
def get_dut_wifi_info():
    """RPC handler to get the dut wifi AP information.
    """
    dut_wifi_info = {}
    value =_CONFIG.get_config_value('MOBLAB', _WIFI_AP_NAME,
        default=None)
    if value is not None:
        dut_wifi_info[_WIFI_AP_NAME] = value
    value = _CONFIG.get_config_value('MOBLAB', _WIFI_AP_PASS,
        default=None)
    if value is not None:
        dut_wifi_info[_WIFI_AP_PASS] = value
    return rpc_utils.prepare_for_serialization(dut_wifi_info)
