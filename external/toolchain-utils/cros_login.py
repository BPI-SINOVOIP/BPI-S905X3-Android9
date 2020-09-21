#!/usr/bin/env python2
#
# Copyright 2010~2015 Google Inc. All Rights Reserved.
"""Script to get past the login screen of ChromeOS."""

from __future__ import print_function

import argparse
import os
import sys
import tempfile

from cros_utils import command_executer

LOGIN_PROMPT_VISIBLE_MAGIC_FILE = '/tmp/uptime-login-prompt-visible'
LOGGED_IN_MAGIC_FILE = '/var/run/state/logged-in'

script_header = """
import os
import autox
import time
"""

wait_for_login_screen = """

while True:
  print 'Waiting for login screen to appear...'
  if os.path.isfile('%s'):
    break
  time.sleep(1)
  print 'Done'

time.sleep(20)
""" % LOGIN_PROMPT_VISIBLE_MAGIC_FILE

do_login = """
xauth_filename = '/home/chronos/.Xauthority'
os.environ.setdefault('XAUTHORITY', xauth_filename)
os.environ.setdefault('DISPLAY', ':0.0')

print 'Now sending the hotkeys for logging in.'
ax = autox.AutoX()
# navigate to login screen
ax.send_hotkey('Ctrl+Shift+q')
ax.send_hotkey('Ctrl+Alt+l')
# escape out of any login screen menus (e.g., the network select menu)
time.sleep(2)
ax.send_hotkey('Escape')
time.sleep(2)
ax.send_hotkey('Tab')
time.sleep(0.5)
ax.send_hotkey('Tab')
time.sleep(0.5)
ax.send_hotkey('Tab')
time.sleep(0.5)
ax.send_hotkey('Tab')
time.sleep(0.5)
ax.send_hotkey('Return')
print 'Waiting for Chrome to appear...'
while True:
  if os.path.isfile('%s'):
    break
  time.sleep(1)
print 'Done'
""" % LOGGED_IN_MAGIC_FILE


def RestartUI(remote, chromeos_root, login=True):
  chromeos_root = os.path.expanduser(chromeos_root)
  ce = command_executer.GetCommandExecuter()
  # First, restart ui.
  command = 'rm -rf %s && restart ui' % LOGIN_PROMPT_VISIBLE_MAGIC_FILE
  ce.CrosRunCommand(command, machine=remote, chromeos_root=chromeos_root)
  host_login_script = tempfile.mktemp()
  device_login_script = '/tmp/login.py'
  login_script_list = [script_header, wait_for_login_screen]
  if login:
    login_script_list.append(do_login)

  full_login_script_contents = '\n'.join(login_script_list)

  with open(host_login_script, 'w') as f:
    f.write(full_login_script_contents)
  ce.CopyFiles(
      host_login_script,
      device_login_script,
      dest_machine=remote,
      chromeos_root=chromeos_root,
      recursive=False,
      dest_cros=True)
  ret = ce.CrosRunCommand(
      'python %s' % device_login_script,
      chromeos_root=chromeos_root,
      machine=remote)
  if os.path.exists(host_login_script):
    os.remove(host_login_script)
  return ret


def Main(argv):
  """The main function."""
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-r', '--remote', dest='remote', help='The remote ChromeOS box.')
  parser.add_argument(
      '-c', '--chromeos_root', dest='chromeos_root', help='The ChromeOS root.')

  options = parser.parse_args(argv)

  return RestartUI(options.remote, options.chromeos_root)


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)
