# Copyright (C) 2016 The Android Open-Source Project
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

"""Various utility functions"""

import logging
import os
import shlex
import subprocess


# The default location in which symbols and minidumps will be saved.
_DEFAULT_ARTIFACT_CACHE_ROOT = os.environ.get('ARC_ARTIFACT_CACHE_ROOT',
                                              '/tmp/arc-artifact-cache')


def get_command_str(command):
  """Returns a quoted version of the command, friendly to copy/paste."""
  return ' '.join(shlex.quote(arg) for arg in command)


def check_call(*subprocess_args, sudo=False, dryrun=False, **kwargs):
  """Runs a subprocess and returns its exit code."""
  if sudo:
    subprocess_args = ('/usr/bin/sudo',) + subprocess_args
  if logging.getLogger().isEnabledFor(logging.DEBUG):
    if kwargs:
      logging.debug('Calling: %s (kwargs %r)', get_command_str(subprocess_args),
                    kwargs)
    else:
      logging.debug('Calling: %s', get_command_str(subprocess_args))
  if dryrun:
    return
  try:
    return subprocess.check_call(subprocess_args, **kwargs)
  except subprocess.CalledProcessError as e:
    logging.error('Error while executing %s', get_command_str(subprocess_args))
    logging.error(e.output)
    raise


def check_output(*subprocess_args, sudo=False, dryrun=False,
                 universal_newlines=True, **kwargs):
  """Runs a subprocess and returns its output."""
  if sudo:
    subprocess_args = ('/usr/bin/sudo',) + subprocess_args
  if logging.getLogger().isEnabledFor(logging.DEBUG):
    if kwargs:
      logging.debug('Calling: %s (kwargs %r)', get_command_str(subprocess_args),
                    kwargs)
    else:
      logging.debug('Calling: %s', get_command_str(subprocess_args))
  if dryrun:
    logging.info('Cannot return any output without running the command. '
                 'Returning an empty string instead.')
    return ''
  try:
    return subprocess.check_output(subprocess_args,
                                   universal_newlines=universal_newlines,
                                   **kwargs)
  except subprocess.CalledProcessError as e:
    logging.error('Error while executing %s', get_command_str(subprocess_args))
    logging.error(e.output)
    raise


def find_repo_root(path=None):
  """Locate the top level of this repo checkout starting at |path|."""
  if path is None:
    path = os.getcwd()
  orig_path = path
  path = os.path.abspath(path)
  while not os.path.exists(os.path.join(path, '.repo')):
    path = os.path.dirname(path)
    if path == '/':
      raise ValueError('Could not locate .repo in %s' % orig_path)
  return path


def makedirs(path):
  """Makes directories if necessary, like 'mkdir -p'"""
  if not os.path.exists(path):
    os.makedirs(path)


def get_prebuilt(tool):
  """Locates a prebuilt file to run."""
  return os.path.abspath(os.path.join(
      os.path.dirname(os.path.dirname(__file__)), 'prebuilt/x86-linux/', tool))


def helper_temp_path(*path, artifact_cache_root=_DEFAULT_ARTIFACT_CACHE_ROOT):
  """Returns the path to use for temporary/cached files."""
  return os.path.join(artifact_cache_root, *path)


def get_product_arch(product):
  """Returns the architecture of a given target |product|."""
  # The prefix can itself have other prefixes, like 'generic_' or 'aosp_'.
  for product_prefix in ('bertha_', 'cheets_'):
    idx = product.find(product_prefix)
    if idx < 0:
      continue
    return product[idx + len(product_prefix):]
  raise Exception('Unrecognized product: %s' % product)


def get_product_name(product):
  """Returns the name of a given target |product|."""
  # The prefix can itself have other prefixes, like 'generic_' or 'aosp_'.
  for product_prefix in ('bertha_', 'cheets_'):
    idx = product.find(product_prefix)
    if idx < 0:
      continue
    return product_prefix.rstrip('_')
  raise Exception('Unrecognized product: %s' % product)


def get_image_type(image_path):
  """Returns the type of a given image |image_path|."""
  if 'Squashfs' in subprocess.check_output(
      ['/usr/bin/file', '--brief', image_path], universal_newlines=True):
    return 'squashfs'
  else:
    return 'ext4'
