# coding: utf-8
# Copyright 2013 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.
"""bot_config for swarming bots running in Home CI.

A copy of this file is downloaded to every swarming bot automatically.
Updates to this file are propogated to bots within ~10 minutes.

Prerequisites:
  - bot has eurtest_libs installed via: cipd install home_internal/eurtest_libs
  - eurtest_libs directory is in PYTHONPATH

There's 3 types of functions in this file:
  - get_*() to return properties to describe this bot.
  - on_*() as hooks based on events happening on the bot.
  - setup_*() to setup global state on the host.
"""

import collections
import json
import logging
import os
import re
import sys
import time

from api import os_utilities
from api import platforms

# Hack to de-prioritize sys.path variables that bot_code imports which are
# incompatible with the libraries we're importing below.
# see https://crbug.com/758609
sys.path.sort(key=lambda x: 'swarm_env' not in x)
# each bot must be run in a virtualenv that has eurtest_libs in PYTHONPATH
try:
  from eurtest.device import device as device_lib
  from utilities import ssh_helper
except ImportError as e:
  sys.stderr.write('Exception: %s\n' % e)
  sys.stderr.write('sys.path:\n%s' % str(sys.path))
  # Temporary while debugging issue
  import google
  print 'google.__path__: %s' % str(google.__path__)
  raise

# Unused argument 'bot' - pylint: disable=W0613
DeviceBot = collections.namedtuple('DeviceBot', 'dimensions state')


def device(device_type, ip, groups=None, nic=None, serial=None, extra_state=None):
  if not hasattr(ip, '__iter__'):
    ip = [ip]
  dimensions = {
    u'device_ip': ip,
    u'device_type': [device_type],
    u'group': groups or ['COMMON'],
    u'nic': [nic or 'ethernet'],
  }
  # You can't advertise a key that cannot be selected,
  # so only set a dimension key if the value is not None
  if serial:
    dimensions[u'serial'] = [serial]
  if len(ip) > 1:
    dimensions[u'group'].append('multiconfig_box')
  return DeviceBot(dimensions, extra_state)

_BOT_DEVICE_MAP = {
  ### home-demo pool ### - enforced via bot name prefix
  'homedemo--1': device('assistantdefault', '1.2.3.4', ['abc_test']),
}

# global for caching device properties gathered dynamically during runtime
_DEVICE = None

### Helper Methods


class SSHError(Exception):
  pass


class Device(object):
  """Retrieves device property information for a device over SSH."""
  # amount of time (secs) until state is due for a refresh
  _STALE_STATE_DELTA = 60

  def __init__(self, device_ip, bot_id):
    self.device_ip = device_ip
    self.bot_id = bot_id
    self._device_client = device_lib.Device(
        ethernet_ip=self.device_ip)
    self._device_client.UseSsh()
    self._last_connect_succeeded = False
    self._properties = {}
    self._stale_state_ts = 0

  @property
  def build_fingerprint(self):
    return self.properties.get('ro.build.fingerprint', None)

  @property
  def hardware(self):
    return self.properties.get('ro.hardware', None)

  @property
  def is_reachable(self):
    """Checks if device is reachable over ssh."""
    if self._properties is None or time.time() > self._stale_state_ts:
      self._properties = self._fetch_properties()
    return self._last_connect_succeeded

  @property
  def product(self):
    return self.properties.get('ro.build.product', None)

  @property
  def properties(self):
    """Returns all of the last-known device properties as a dict."""
    if self._properties is None or time.time() > self._stale_state_ts:
      try:
        # Keep last-known properties when SSHError is raised
        self._properties = self._fetch_properties()
      except SSHError as e:
        logging.exception(e)
    return self._properties

  def _exec_ssh_cmd(self, cmd):
    """Executes the given cmd against the device over SSH.
    Arguments:
    - cmd: the command to be run on the device over ssh.
    Returns:
      stdout of cmd output
    Raises:
      SSHError: upon ssh cmd exec failure
    """
    # Temporary: fetch device properties from static local file when present.
    # This let's us demo what our swarm bots will look like, even though the
    # demo host ('voyager') is on a lab network in WAT that can't reach the
    # MTV devices over ssh.  The real device properties are fetched over ssh
    # by a script on Corp and copied to the swarm host before starting bots.
    # TODO(jonesmi): remove this temporary workaround when bots run in MTV lab
    bot_common_dir = os.getenv('BOT_COMMON_DIR')
    if bot_common_dir:
      props_file = os.path.join(bot_common_dir, '%s.props' % self.bot_id)
      if os.path.isfile(props_file):
        with open(props_file, 'r') as f:
          # pretend that we read this from SSH connection
          self._last_connect_succeeded = True
          return f.read()
    try:
      if self._device_client.Cmd(cmd, timeout_secs=30):
        self._last_connect_succeeded = True
        return self._device_client.GetCmdOutput()
      else:
        self._last_connect_succeeded = False
        raise SSHError('failed to execute cmd over ssh: %s' % cmd)
    except ssh_helper.CommandThreadTimeoutError:
      self._last_connect_succeeded = False
      raise SSHError('timeout when waiting for ssh connect: %s' % cmd)

  def _fetch_properties(self):
    """Fetches device properties over ssh using getprop command."""
    self._stale_state_ts = time.time() + self._STALE_STATE_DELTA
    properties = {}
    stdout = self._exec_ssh_cmd('getprop')
    for line in stdout.strip().split('\n'):
      match = re.match(r'\[(.*)\]: \[(.*)\]', line)
      if match:
        properties[match.group(1)] = match.group(2)
    return properties


### _get* Hooks
def get_dimensions(bot):
  # pylint: disable=line-too-long
  """Returns dict with the bot's dimensions.
  The dimensions are what are used to select the bot that can run each task.
  The bot id will be automatically selected based on the hostname with
  os_utilities.get_dimensions(). If you want something more special, specify it
  in your bot_config.py and override the item 'id'.
  The dimensions returned here will be joined with server defined dimensions
  (extracted from bots.cfg config file based on the bot id). Server defined
  dimensions override the ones provided by the bot. See bot.Bot.dimensions for
  more information.
  See https://github.com/luci/luci-py/tree/master/appengine/swarming/doc/Magic-Values.md.
  Arguments:
  - bot: bot.Bot instance or None. See ../api/bot.py.
  """
  dimensions = os_utilities.get_dimensions()
  # The bot base directory is formatted like <HOME>/bots/<Id>
  id = '%s--%s' % (
      os_utilities.get_hostname_short(),
      os.path.basename(bot.base_dir))
  dimensions[u'id'] = [id]
  if id in _BOT_DEVICE_MAP:
    dimensions.update(_BOT_DEVICE_MAP[id].dimensions)
    global _DEVICE
    if not _DEVICE:
      device_ip = dimensions[u'device_ip']
      # use first device_ip in list of device IPs for pulling device attributes
      if device_ip:
        assert isinstance(device_ip, (list, tuple)), repr(device_ip)
        _DEVICE = Device(device_ip[0], id)
    if _DEVICE:
      dimensions[u'build'] = _DEVICE.build_fingerprint,
      dimensions[u'hardware'] = _DEVICE.hardware,
      dimensions[u'product'] = _DEVICE.product,
  # TODO(jonesmi): don't strip these anymore after swarming fix goes in for limit on # dimensions
  del dimensions[u'cores']
  del dimensions[u'cpu']
  del dimensions[u'gpu']
  del dimensions[u'machine_type']
  return dimensions


def get_state(bot):
  # pylint: disable=line-too-long
  """Returns dict with a state of the bot reported to the server with each poll.
  It is only for dynamic state that changes while bot is running for information
  for the sysadmins.
  The server can not use this state for immediate scheduling purposes (use
  'dimensions' for that), but it can use it for maintenance and bookkeeping
  tasks.
  See https://github.com/luci/luci-py/tree/master/appengine/swarming/doc/Magic-Values.md.
  Arguments:
  - bot: bot.Bot instance or None. See ../api/bot.py.
  """
  state = os_utilities.get_state()
  if _DEVICE:
    state[u'device'] = _DEVICE.properties
    if _DEVICE.bot_id in _BOT_DEVICE_MAP:
      extra_state = _BOT_DEVICE_MAP[_DEVICE.bot_id].state
      if extra_state:
        state[u'device'].update(extra_state)
    if not _DEVICE.is_reachable:
      state[u'quarantined'] = 'device is not reachable'
  return state


def get_authentication_headers(bot):
  """Returns authentication headers and their expiration time.
  The returned headers will be passed with each HTTP request to the Swarming
  server (and only Swarming server). The bot will use the returned headers until
  they are close to expiration (usually 6 min, see AUTH_HEADERS_EXPIRATION_SEC
  in remote_client.py), and then it'll attempt to refresh them by calling
  get_authentication_headers again.
  Can be used to implement per-bot authentication. If no headers are returned,
  the server will use only IP whitelist for bot authentication.
  On GCE will use OAuth token of the default GCE service account. It should have
  "User info" API scope enabled (this can be set when starting an instance). The
  server should be configured (via bots.cfg) to trust this account (see
  'require_service_account' in bots.proto).
  May be called by different threads, but never concurrently.
  Arguments:
  - bot: bot.Bot instance. See ../api/bot.py.
  Returns:
    Tuple (dict with headers or None, unix timestamp of when they expire).
  """
  if platforms.is_gce():
    tok = platforms.gce.oauth2_access_token()
    return {'Authorization': 'Bearer %s' % tok}, time.time() + 5*60
  return (None, None)


### on_* Hooks
def on_bot_shutdown(bot):
  """Hook function called when the bot shuts down, usually rebooting.
  It's a good time to do other kinds of cleanup.
  Arguments:
  - bot: bot.Bot instance. See ../api/bot.py.
  """
  pass


def on_handshake(bot):
  """Hook function called when the bot starts, after handshake with the server.
  Here the bot already knows server enforced dimensions (defined in server side
  bots.cfg file).
  This is called right before starting to poll for tasks. It's a good time to
  do some final initialization or cleanup that may depend on server provided
  configuration.
  Arguments:
  - bot: bot.Bot instance. See ../api/bot.py.
  """
  pass


def on_before_task(bot, bot_file):
  """Hook function called before running a task.
  It shouldn't do much, since it can't cancel the task so it shouldn't do
  anything too fancy.
  Arguments:
  - bot: bot.Bot instance. See ../api/bot.py.
  - bot_file: Path to file to write information about the state of the bot.
              This file can be used to pass certain info about the bot
              to tasks, such as which connected android devices to run on. See
              https://github.com/luci/luci-py/tree/master/appengine/swarming/doc/Magic-Values.md#run_isolated
  """
  with open(bot_file, 'wb') as f:
    json.dump(bot.dimensions, f)


def on_after_task(bot, failure, internal_failure, dimensions, summary):
  """Hook function called after running a task.
  It is an excellent place to do post-task cleanup of temporary files.
  The default implementation restarts after a task failure or an internal
  failure.
  Arguments:
  - bot: bot.Bot instance. See ../api/bot.py.
  - failure: bool, True if the task failed.
  - internal_failure: bool, True if an internal failure happened.
  - dimensions: dict, Dimensions requested as part of the task.
  - summary: dict, Summary of the task execution.
  """
  # Example code:
  #if failure:
  #  bot.restart('Task failure')
  #elif internal_failure:
  #  bot.restart('Internal failure')


def on_bot_idle(bot, since_last_action):
  """Hook function called once when the bot has been idle; when it has no
  command to execute.
  This is an excellent place to put device in 'cool down' mode or any
  "pre-warming" kind of stuff that could take several seconds to do, that would
  not be appropriate to do in on_after_task(). It could be worth waiting for
  `since_last_action` to be several seconds before doing a more lengthy
  operation.
  This function is called repeatedly until an action is taken (a task, updating,
  etc).
  This is a good place to do "auto reboot" for hardware based bots that are
  rebooted periodically.
  Arguments:
  - bot: bot.Bot instance. See ../api/bot.py.
  - since_last_action: time in second since last action; e.g. amount of time the
                       bot has been idle.
  """
  pass
