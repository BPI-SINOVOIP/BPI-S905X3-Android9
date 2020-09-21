#!/usr/bin/python3
#
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

from __future__ import print_function

import argparse
import atexit
import getpass
import hashlib
import itertools
import logging
import os
import re
import shutil
import string
import subprocess
import sys
import tempfile
import time
import xml.etree.cElementTree as ElementTree
import zipfile

import lib.build_artifact_fetcher
import lib.util


_SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

_EXPECTED_TARGET_PRODUCTS = {
    '^x86': ('cheets_x86', 'cheets_x86_64', 'bertha_x86', 'bertha_x86_64'),
    '^arm': ('cheets_arm', 'bertha_arm',),
    '^aarch64$': ('cheets_arm', 'bertha_arm',),
}
_ANDROID_ROOT = '/opt/google/containers/android'
_ANDROID_ROOT_STATEFUL = os.path.join('/usr/local',
                                      os.path.relpath(_ANDROID_ROOT, '/'))
_CONTAINER_INSTANCE_ROOT_WILDCARD = '/run/containers/android*'
_CONTAINER_ROOT = os.path.join(_ANDROID_ROOT, 'rootfs', 'root')
_RSYNC_COMMAND = ['rsync', '--inplace', '-v', '--progress']
_SCP_COMMAND = ['scp']

_BUILD_FILENAME = string.Template('${product}-img-${build_id}.zip')
_BUILD_TARGET = string.Template('${product}-${build_variant}')

_CHROMEOS_ARC_ANDROID_SDK_VERSION = 'CHROMEOS_ARC_ANDROID_SDK_VERSION='

_GENERIC_DEVICE = 'generic_%(arch)s_%(product)s'
_RO_BUILD_TYPE = 'ro.build.type='
_RO_BUILD_VERSION_SDK = 'ro.build.version.sdk='
_RO_PRODUCT_DEVICE = 'ro.product.device='

_ANDROID_REL_KEY_SIGNATURE_SUBSTRING = (
    '55b390dd7fdb9418631895d5f759f30112687ff621410c069308a')
_APK_KEY_DEBUG = 'debug-key'
_APK_KEY_RELEASE = 'release-key'
_APK_KEY_UNKNOWN = 'unknown'
_GMS_CORE_PACKAGE_NAME = 'com.google.android.gms'

_ANDROID_SDK_MAPPING = {
    23: "MNC (API 23)",
    24: "NYC (API 24)",
    25: "NYC_MR1 (API 25)",
    26: "OC (API 26)",
}

# Bytes per Megabyte.
_MB = 1024**2


class RemoteProxy(object):
  """Proxy class to run command line on the remote test device."""

  def __init__(self, remote, dryrun):
    self._remote = remote
    self._dryrun = dryrun
    self._sync_command = (
        _RSYNC_COMMAND if self._has_rsync_on_remote_device() else _SCP_COMMAND)

  def check_call(self, remote_command):
    """Runs |remote_command| on the remote test device via ssh."""
    command = self.get_ssh_commandline(remote_command)
    lib.util.check_call(dryrun=self._dryrun, *command)

  def check_output(self, remote_command):
    """Runs |remote_command| on the remote test device via ssh, and returns
       its output."""
    command = self.get_ssh_commandline(remote_command)
    return lib.util.check_output(dryrun=self._dryrun, *command)

  def sync(self, file_list, dest_dir):
    """Copies |file_list| to the |dest_dir| on the remote test device."""
    target = 'root@%s:%s' % (self._remote, dest_dir)
    command = self._sync_command + file_list + [target]
    lib.util.check_call(dryrun=self._dryrun, *command)

  def read(self, src_path):
    """Gets the contents of |src_path| from the remote test device."""
    return self.check_output('/bin/cat %s' % src_path)

  def write(self, contents, dest_path):
    """Writes |contents| into |dest_path| on the remote test device."""
    with tempfile.NamedTemporaryFile(mode='w') as f:
      f.write(contents)
      self.push(f.name, dest_path)

  def push(self, source_path, dest_path):
    """Pushes |source_path| on the host, to |dest_path| on the remote test
       device.

    Args:
        source_path: Host file path to be pushed.
        dest_path: Path to the destination location on the remote test device.
    """
    target = 'root@%s:%s' % (self._remote, dest_path)
    command = _SCP_COMMAND + [source_path, target]
    lib.util.check_call(dryrun=self._dryrun, *command)

  def pull(self, source_path, dest_path):
    """Pulls |source_path| from the remote test device, to |dest_path| on the
       host.

    Args:
        source_path: Remote test device file path to be pulled.
        dest_path: Path to the destination location on the host.
    """
    target = 'root@%s:%s' % (self._remote, source_path)
    command = _SCP_COMMAND + [target, dest_path]
    return lib.util.check_call(dryrun=self._dryrun, *command)

  def get_ssh_commandline(self, remote_command):
    return ['ssh', 'root@' + self._remote, remote_command]

  def _has_rsync_on_remote_device(self):
    command = self.get_ssh_commandline('which rsync')
    logging.debug('Calling: %s', lib.util.get_command_str(command))
    # Always return true for --dryrun.
    return self._dryrun or subprocess.call(command) == 0


class TemporaryDirectory(object):
  """A context object that has a temporary directory with the same lifetime."""

  def __init__(self):
    self.name = None

  def __enter__(self):
    self.name = tempfile.mkdtemp()
    return self

  def __exit__(self, exception_type, exception_value, traceback):
    lib.util.check_call('rm', '-rf', self.name, sudo=True)


class MountWrapper(object):
  """A context object that mounts an image during the lifetime."""

  def __init__(self, image_path, mountpoint):
    self._image_path = image_path
    self._mountpoint = mountpoint

  def __enter__(self):
    lib.util.check_call('/bin/mount', '-o', 'loop', self._image_path,
                        self._mountpoint, sudo=True)
    return self

  def __exit__(self, exception_type, exception_value, traceback):
    try:
      lib.util.check_call('/bin/umount', self._mountpoint, sudo=True)
    except Exception:
      if not exception_type:
        raise
      # Instead of propagate the exception, record the one from exit body.
      logging.exception('Failed to umount ' + self._mountpoint)


class Simg2img(object):
  """Wrapper class of simg2img"""

  def __init__(self, simg2img_path, dryrun):
    self._path = simg2img_path
    self._dryrun = dryrun

  def convert(self, src, dest):
    """Converts the image to the raw image by simg2img command line.

    If |dryrun| is set, does not execute the commandline.
    """
    lib.util.check_call(self._path, src, dest, dryrun=self._dryrun)


def _verify_machine_arch(remote_proxy, target_product, dryrun):
  """Verifies if the data being pushed is build for the target architecture.

  Args:
      remote_proxy: RemoteProxy instance for the remote test device.
      target_product: Target product name of the image being pushed. This is
          usually set by "lunch" command. E.g. "cheets_x86" or "cheets_arm".
      dryrun: If set, this function assumes the machine architectures match.

  Raises:
      AssertionError: If the pushing image does not match to the remote test
          device.
  """
  if dryrun:
    logging.debug('Pretending machine architectures match')
    return
  remote_arch = remote_proxy.check_output('uname -m')
  for arch_pattern, expected_set in _EXPECTED_TARGET_PRODUCTS.items():
    if re.search(arch_pattern, remote_arch):
      expected = itertools.chain.from_iterable(
          (expected, 'aosp_' + expected, 'sdk_google_' + expected)
          for expected in expected_set)
      assert target_product in expected, (
          ('Architecture mismatch: Deploying \'%s\' to \'%s\' seems incorrect.'
           % (target_product, remote_arch)))
      return
  logging.warning('Unknown remote machine type \'%s\'. Skipping '
                  'architecture sanity check.', remote_arch)


def _convert_images(simg2img, out, product, push_vendor_image,
                    shift_ugids, mksquashfs_path, unsquashfs_path,
                    shift_uid_py_path, dryrun):
  """Converts the images being pushed to the raw images.

  Returns:
      A tuple of (large_file_list, file_list). Each list consists of paths of
      converted files.
  """
  result = []
  result_large = []

  system_raw_img = os.path.join(out, 'system.raw.img')
  simg2img.convert(os.path.join(out, 'system.img'), system_raw_img)
  file_contexts_path = None
  if not dryrun:
    with ContainerImageEditor(mksquashfs_path, unsquashfs_path, system_raw_img,
                              '/', out) as e:
      file_contexts_path = e.file_contexts_path
      if 'x86' in product:
        logging.debug('Creating \'system/lib/arm\' dir and houdini symlinks in '
                      'the system image')
        # Create system/lib/arm dir
        dir_name = os.path.join(e.tmp_dir_name, 'system/lib/arm')
        logging.debug('Creating directory: %s', dir_name)
        lib.util.check_call('mkdir', '-p', dir_name, sudo=True)
        # Create a symlink: system/bin/houdini --> /vendor/bin/houdini
        lib.util.check_call('ln', '-sf', '/vendor/bin/houdini',
                            os.path.join(e.tmp_dir_name, 'system/bin/houdini'),
                            sudo=True)
        # Create a symlink: system/lib/libhoudini.so --> /vendor/lib/libhoudini.so
        lib.util.check_call('ln', '-sf', '/vendor/lib/libhoudini.so',
                            os.path.join(e.tmp_dir_name,
                                         'system/lib/libhoudini.so'), sudo=True)
      # TODO(b/65117245): This needs to be part of the build.
      if shift_ugids:
        # Shift the UIDs/GIDs.
        lib.util.check_call(shift_uid_py_path, e.tmp_dir_name, sudo=True)

  result_large.append(system_raw_img)

  if push_vendor_image:
    vendor_raw_img = os.path.join(out, 'vendor.raw.img')
    simg2img.convert(os.path.join(out, 'vendor.img'), vendor_raw_img)
    # TODO(b/65117245): This needs to be part of the build.
    if shift_ugids and not dryrun:
      with ContainerImageEditor(mksquashfs_path, unsquashfs_path,
                                vendor_raw_img, 'vendor', out,
                                file_contexts_path=file_contexts_path) as e:
        # Shift the UIDs/GIDs.
        lib.util.check_call(shift_uid_py_path, e.tmp_dir_name,
                            sudo=True)
    result.append(vendor_raw_img)

  return (result_large, result)


def _update_build_fingerprint(remote_proxy, build_fingerprint):
  """Updates CHROMEOS_ARC_VERSION in /etc/lsb-release.

  Args:
      remote_proxy: RemoteProxy instance connected to the test device.
      build_fingerprint: The version code which should be embedded into
          /etc/lsb-release.
  """
  if not build_fingerprint:
    logging.warning(
        'Skipping version update. ARC version will be reported incorrectly')
    return

  # Replace the ARC version on disk with what we're pushing there.
  logging.info('Updating CHROMEOS_ARC_VERSION...')
  remote_proxy.check_call(' '.join([
      '/bin/sed', '-i',
      # Note: we assume build_fingerprint does not contain any char which
      # needs to be escaped.
      r'"s/^\(CHROMEOS_ARC_VERSION=\).*/\1%(_BUILD_FINGERPRINT)s/"',
      '/etc/lsb-release'
  ]) % {'_BUILD_FINGERPRINT': build_fingerprint})


def _get_remote_device_android_sdk_version(remote_proxy, dryrun):
  """ Returns the Android SDK version on the remote device.

  Args:
      remote_proxy: RemoteProxy instance for the remote test device.
      dryrun: If set, this function assumes Android SDK version is 1.
  """
  if dryrun:
    logging.debug('Pretending target device\'s Android SDK version is 1')
    return 1
  try:
    line = remote_proxy.check_output(
        'grep ^%s /etc/lsb-release' % _CHROMEOS_ARC_ANDROID_SDK_VERSION).strip()
  except subprocess.CalledProcessError:
    logging.exception('Failed to inspect /etc/lsb-release remotely')
    return None

  if not line.startswith(_CHROMEOS_ARC_ANDROID_SDK_VERSION):
    logging.warning('Failed to find the correct string format.\n'
                    'Expected format: "%s"\nActual string: "%s"',
                    _CHROMEOS_ARC_ANDROID_SDK_VERSION, line)
    return None

  android_sdk_version = int(
      line[len(_CHROMEOS_ARC_ANDROID_SDK_VERSION):].strip())
  logging.debug('Target device\'s Android SDK version: %d', android_sdk_version)
  return android_sdk_version


def _verify_android_sdk_version(remote_proxy, provider, dryrun):
  """Verifies if the Android SDK versions of the pushing image and the test
  device are the same.

  Args:
      remote_proxy: RemoteProxy instance for the remote test device.
      provider: Android image provider.
      dryrun: If set, this function assumes Android SDK versions match.

  Raises:
      AssertionError: If the Android SDK version of pushing image does not match
          the Android SDK version on the remote test device.
  """
  if dryrun:
    logging.debug('Pretending Android SDK versions match')
    return
  logging.debug('New image\'s Android SDK version: %d',
                provider.get_build_version_sdk())

  device_android_sdk_version = _get_remote_device_android_sdk_version(
      remote_proxy, dryrun)

  if device_android_sdk_version is None:
    if not boolean_prompt('Unable to determine the target device\'s Android '
                          'SDK version. Continue?', default=False):
      sys.exit(1)
  else:
    assert device_android_sdk_version == provider.get_build_version_sdk(), (
        'Android SDK versions do not match. The target device has {}, while '
        'the new image is {}'.format(
            _android_sdk_version_to_string(device_android_sdk_version),
            _android_sdk_version_to_string(provider.get_build_version_sdk())))


def _android_sdk_version_to_string(android_sdk_version):
  """Converts the |android_sdk_version| to a human readable string

  Args:
    android_sdk_version: The Android SDK version number as a string
  """
  return _ANDROID_SDK_MAPPING.get(
      android_sdk_version,
      'Unknown SDK Version (API {})'.format(android_sdk_version))


def _get_selinux_file_contexts_contents(out):
  """Returns the final contents of the SELinux file_contexts file."""
  contents = []
  for filename in ('file_contexts', 'plat_file_contexts',
                   'nonplat_file_contexts'):
    path = os.path.join(out, 'root', filename)
    # Some files are always expected to be missing due to not being present in
    # the branch.
    if not os.path.exists(path):
      logging.debug('Skipping %s since it is missing', path)
      continue
    with open(path, 'r') as f:
      contents.append(f.read())
  return '\n'.join(contents)


def _is_selinux_file_contexts_updated(remote_proxy, out, dryrun):
  """Returns True if SELinux file_contexts is updated."""
  if dryrun:
    logging.debug('Pretending file_contexts is not updated in dryrun mode')
    return False
  remote_file_contexts_sha1, _ = remote_proxy.check_output(
      'sha1sum /etc/selinux/arc/contexts/files/android_file_contexts').split()
  file_contexts_contents = _get_selinux_file_contexts_contents(out)
  # hashlib expects bytes, not str.
  host_file_contexts_sha1 = hashlib.sha1(
      file_contexts_contents.encode('utf-8')).hexdigest()
  return remote_file_contexts_sha1 != host_file_contexts_sha1


def _update_selinux_file_contexts(remote_proxy, out):
  """Updates the selinux file_contexts file."""
  android_file_contexts_contents = _get_selinux_file_contexts_contents(out)
  remote_proxy.write(android_file_contexts_contents,
                     '/etc/selinux/arc/contexts/files/android_file_contexts')
  file_contexts_contents = []
  for line in android_file_contexts_contents.split('\n'):
    line = line.strip()
    if not line or line.startswith('#'):
      continue
    file_contexts_contents.append(
        '%s/rootfs/root%s' % (_ANDROID_ROOT, line))
  remote_file_contexts_path = '/etc/selinux/arc/contexts/files/file_contexts'
  remote_file_contexts_contents = remote_proxy.read(remote_file_contexts_path)
  try:
    # This string comes from
    # private-overlays/project-cheets-private/chromeos-base/\
    #    android-container-<VERSION>/files/chromeos_file_contexts
    header_idx = remote_file_contexts_contents.index(
        '# Chrome OS file contexts')
  except ValueError:
    # The header was missing. Will concat the whole file.
    logging.warning('Could not find Chrome OS header in %s. '
                    'Will use the whole file', remote_file_contexts_path)
    header_idx = 0
  file_contexts_contents.append(remote_file_contexts_contents[header_idx:])
  remote_proxy.write('\n'.join(file_contexts_contents),
                     remote_file_contexts_path)


def _is_selinux_policy_updated(remote_proxy, out, dryrun):
  """Returns True if SELinux policy is updated."""
  if dryrun:
    logging.debug('Pretending sepolicy is not updated in dryrun mode')
    return False
  remote_sepolicy_sha1, _ = remote_proxy.check_output(
      'sha1sum /etc/selinux/arc/policy/policy.30').split()
  with open(os.path.join(out, 'root', 'sepolicy'), 'rb') as f:
    host_sepolicy_sha1 = hashlib.sha1(f.read()).hexdigest()
  return remote_sepolicy_sha1 != host_sepolicy_sha1


def _update_selinux_policy(remote_proxy, out):
  """Updates the selinux policy file."""
  remote_proxy.push(os.path.join(out, 'root', 'sepolicy'),
                    '/etc/selinux/arc/policy/policy.30')


def _remount_rootfs_as_writable(remote_proxy):
  """Remounts root file system to make it writable."""
  remote_proxy.check_call('mount -o remount,rw /')


def _get_free_space(remote_proxy):
  """Gets the number of free bytes in the root partition."""
  return int(remote_proxy.check_output(
      'echo $(( '
      '    $(df --output=avail --local --block-size 1 / | tail -n1) + '
      '    $(du --bytes /opt/google/containers/android/system.raw.img | '
      '      awk \'{print $1}\') '
      '))'))


def boolean_prompt(prompt, default=True, true_value='yes', false_value='no',
                   prolog=None):
  """Helper function for processing boolean choice prompts.

  Args:
    prompt: The question to present to the user.
    default: Boolean to return if the user just presses enter.
    true_value: The text to display that represents a True returned.
    false_value: The text to display that represents a False returned.
    prolog: The text to display before prompt.

  Returns:
    True or False.
  """
  true_value, false_value = true_value.lower(), false_value.lower()
  true_text, false_text = true_value, false_value
  if true_value == false_value:
    raise ValueError('true_value and false_value must differ: got %r'
                     % true_value)

  if default:
    true_text = true_text[0].upper() + true_text[1:]
  else:
    false_text = false_text[0].upper() + false_text[1:]

  prompt = ('\n%s (%s/%s)? ' % (prompt, true_text, false_text))

  if prolog:
    prompt = ('\n%s\n%s' % (prolog, prompt))

  while True:
    try:
      response = input(prompt).lower()
    except EOFError:
      # If the user hits CTRL+D, or stdin is disabled, use the default.
      print(file=sys.stderr)
      response = None
    except KeyboardInterrupt:
      # If the user hits CTRL+C, just exit the process.
      print(file=sys.stderr)
      print('CTRL+C detected; exiting', file=sys.stderr)
      raise

    if not response:
      return default
    if true_value.startswith(response):
      if not false_value.startswith(response):
        return True
      # common prefix between the two...
    elif false_value.startswith(response):
      return False


def _disable_rootfs_verification(force, remote_proxy):
  make_dev_ssd_path = \
      '/usr/libexec/debugd/helpers/dev_features_rootfs_verification'
  make_dev_ssd_command = remote_proxy.get_ssh_commandline(make_dev_ssd_path)
  logging.info('Detected that the device has rootfs verification enabled.')
  logging.info('This script can remove the rootfs verification using `%s`, '
               'which requires that the device is rebooted afterwards.',
               lib.util.get_command_str(make_dev_ssd_command))
  if not force:
    logging.info('Automatically remove rootfs verification and skip this '
                 'prompt by specifying --force.')
    if not boolean_prompt('Remove rootfs verification?', default=False):
      return False
  remote_proxy.check_call(make_dev_ssd_path)
  reboot_time = time.time()
  remote_proxy.check_call('reboot')
  logging.debug('Waiting up to 10 seconds for the machine to reboot')
  for _ in range(10):
    time.sleep(1)
    try:
      device_boot_time = remote_proxy.check_output('grep btime /proc/stat | ' +
                                                   'cut -d" " -f2')
      if int(device_boot_time) >= reboot_time:
        return True
    except subprocess.CalledProcessError:
      pass
  logging.error('Failed to detect whether the device had successfully rebooted')
  return False


def _stop_ui(remote_proxy):
  remote_proxy.check_call('\n'.join([
      # Unmount the container root/vendor and root if necessary. This also stops
      # UI.
      'stop arc-system-mount || true',
  ]))


class ImageUpdateMode(object):
  """Context object to manage remote host writable status."""

  def __init__(self, remote_proxy, is_selinux_policy_updated, push_to_stateful,
               clobber_data, force):
    self._remote_proxy = remote_proxy
    self._is_selinux_policy_updated = is_selinux_policy_updated
    self._push_to_stateful = push_to_stateful
    self._clobber_data = clobber_data
    self._force = force

  def __enter__(self):
    logging.info('Setting up ChromeOS device to image-writable...')

    if self._clobber_data:
      self._remote_proxy.check_call(
          'if [ -e %(ANDROID_ROOT_WILDCARD)s/root/data ]; then'
          '  kill -9 `cat %(ANDROID_ROOT_WILDCARD)s/container.pid`;'
          '  find %(ANDROID_ROOT_WILDCARD)s/root/data'
          '       %(ANDROID_ROOT_WILDCARD)s/root/cache -mindepth 1 -delete;'
          'fi' % {'ANDROID_ROOT_WILDCARD': _CONTAINER_INSTANCE_ROOT_WILDCARD})

    _stop_ui(self._remote_proxy)
    try:
      _remount_rootfs_as_writable(self._remote_proxy)
    except subprocess.CalledProcessError:
      if not _disable_rootfs_verification(self._force, self._remote_proxy):
        raise
      _stop_ui(self._remote_proxy)
      # Try to remount rootfs as writable. Bail out if it fails this time.
      _remount_rootfs_as_writable(self._remote_proxy)
    self._remote_proxy.check_call('\n'.join([
        # Delete the image file if it is a symlink.
        'if [ -L %(_ANDROID_ROOT)s/system.raw.img ]; then'
        '  rm %(_ANDROID_ROOT)s/system.raw.img;'
        'fi',
    ]) % {'_ANDROID_ROOT': _ANDROID_ROOT})
    if self._push_to_stateful:
      self._remote_proxy.check_call('\n'.join([
          # Create the destination directory in the stateful partition.
          'mkdir -p %(_ANDROID_ROOT_STATEFUL)s',
      ]) % {'_ANDROID_ROOT_STATEFUL': _ANDROID_ROOT_STATEFUL})

  def __exit__(self, exc_type, exc_value, traceback):
    if self._push_to_stateful:
      # Push the image to _ANDROID_ROOT_STATEFUL instead of _ANDROID_ROOT.
      # Create a symlink so that arc-system-mount can handle it.
      self._remote_proxy.check_call('\n'.join([
          'ln -sf %(_ANDROID_ROOT_STATEFUL)s/system.raw.img '
          '  %(_ANDROID_ROOT)s/system.raw.img',
      ]) % {'_ANDROID_ROOT': _ANDROID_ROOT,
            '_ANDROID_ROOT_STATEFUL': _ANDROID_ROOT_STATEFUL})

    if self._is_selinux_policy_updated:
      logging.info('*** SELinux policy updated. ***')
    else:
      logging.info('*** SELinux policy is not updated. Restarting ui. ***')
      try:
        self._remote_proxy.check_call('\n'.join([
            # Make the whole invocation fail if any individual command does.
            'set -e',

            # Remount the root file system to readonly.
            'mount -o remount,ro /',

            # Restart UI.
            'start ui',

            # Mount the updated {system,vendor}.raw.img. This will also trigger
            # android-ureadahead once it's done and should remove the packfile.
            'start arc-system-mount',
        ]))
        return
      except Exception:
        # The above commands are just an optimization to avoid having to reboot
        # every single time an image is pushed, which saves 6-10s. If any of
        # them fail, the only safe thing to do is reboot the device.
        logging.exception('Failed to cleanly restart ui, fall back to reboot')

    logging.info('*** Reboot required. ***')
    try:
      self._remote_proxy.check_call('reboot')
    except Exception:
      if exc_type is None:
        raise
      # If the body block of a with statement also raises an error, here we
      # just log the exception, so that the main exception will be propagated to
      # the caller properly.
      logging.exception('Failed to reboot the device')


class PreserveTimestamps(object):
  """Context object to modify a file but preserve the original timestamp."""

  def __init__(self, path):
    self.path = path
    self._original_timestamp = None

  def __enter__(self):
    # Save the original timestamp
    self._original_timestamp = os.stat(self.path)
    return self

  def __exit__(self, exception_type, exception_value, traceback):
    # Apply the original timestamp
    os.utime(self.path, (self._original_timestamp.st_atime,
                         self._original_timestamp.st_mtime))


def _download_artifact(out_dir, build_id, product, build_variant):
  mapping = dict(build_id=build_id, product=product)
  uploaded_filename = _BUILD_FILENAME.substitute(mapping)
  filename = os.path.join(out_dir, uploaded_filename)
  fetcher = lib.build_artifact_fetcher.BuildArtifactFetcher(
      lib.util.get_product_arch(product), build_variant, build_id)
  fetcher.fetch(uploaded_filename, filename)
  return filename


def _extract_files(unsquashfs_path, out_dir, system_raw_img, paths):
  with TemporaryDirectory() as tmp_dir:
    if lib.util.get_image_type(system_raw_img) is 'squashfs':
      lib.util.check_call(
          unsquashfs_path, '-d', tmp_dir.name, '-no-progress', '-f',
          system_raw_img, '-no-xattrs', *[path[0] for path in paths], sudo=True)
      _extract_files_helper(tmp_dir.name, out_dir, paths)
    else:
      with MountWrapper(system_raw_img, tmp_dir.name):
        _extract_files_helper(tmp_dir.name, out_dir, paths)


def _extract_files_helper(source_root, destination_root, paths):
  for source, destination in paths:
    source = os.path.join(source_root, source)
    # Some files are always expected to be missing due to not being
    # present in the branch.
    if not os.path.exists(source):
      logging.debug('Skipping %s since it is missing', source)
      continue
    destination = os.path.join(destination_root, destination)
    if not os.path.exists(os.path.dirname(destination)):
      os.makedirs(os.path.dirname(destination))
    # 'sudo' is needed due to b/65117245. Android P shifts ugids of extracted
    # files.
    lib.util.check_call('cp', source, destination, sudo=True)
    lib.util.check_call('chown', getpass.getuser(), destination, sudo=True)


def _extract_artifact(simg2img, unsquashfs_path, out_dir, filename):
  with zipfile.ZipFile(filename, 'r') as z:
    z.extract('system.img', out_dir)
    z.extract('vendor.img', out_dir)
  # Note that the same simg2img conversion is performed again for system.img
  # later, but the extra run is acceptable (<2s).  If this is important, we
  # could try to change the program flow.
  simg2img.convert(os.path.join(out_dir, 'system.img'),
                   os.path.join(out_dir, 'system.raw.img'))
  _extract_files(unsquashfs_path, out_dir,
                 os.path.join(out_dir, 'system.raw.img'),
                 [('sepolicy', 'root/sepolicy'),
                  ('file_contexts', 'root/file_contexts'),
                  ('plat_file_contexts', 'root/plat_file_contexts'),
                  ('nonplat_file_contexts', 'root/nonplat_file_contexts'),
                  ('system/build.prop', 'build.prop')])


def _make_tempdir_deleted_on_exit():
  d = tempfile.mkdtemp()
  atexit.register(shutil.rmtree, d, ignore_errors=True)
  return d


def _detect_cert_inconsistency(force, remote_proxy, new_variant, dryrun):
  """Prompt to ask for deleting data based on detected situation (best effort).

  Detection is only accurate for active session, so it won't fix other profiles.

  As GMS apps are signed with different key between user and non-user build,
  the container won't run correctly if old key has been registered in /data.
  """
  if dryrun:
    return False

  # Get current build variant on device.
  cmd = 'grep %s %s' % (_RO_BUILD_TYPE,
                        os.path.join(_CONTAINER_ROOT, 'system/build.prop'))
  try:
    line = remote_proxy.check_output(cmd).strip()
  except subprocess.CalledProcessError:
    # Catch any error to avoid blocking the push.
    logging.exception('Failed to inspect build property remotely')
    return False
  device_variant = line[len(_RO_BUILD_TYPE):]

  device_apk_key = _APK_KEY_UNKNOWN
  try:
    device_apk_key = _get_remote_device_apk_key(remote_proxy)
  except Exception as e:
    logging.warning('There was an error getting the remote device APK '
                    'key signature %s. Assuming APK key signature is '
                    '\'unknown\'', e)

  logging.debug('device apk key: %s; build variant: %s -> %s', device_apk_key,
                device_variant, new_variant)

  # GMS signature in /data is inconsistent with the new build.
  is_inconsistent = (
      (device_apk_key == _APK_KEY_RELEASE and new_variant != 'user') or
      (device_apk_key == _APK_KEY_DEBUG and new_variant == 'user'))

  if is_inconsistent:
    new_apk_key = _APK_KEY_RELEASE if new_variant == 'user' else _APK_KEY_DEBUG
    logging.info('Detected apk signature change (%s -> %s[%s]) on current user.'
                 % (device_apk_key, new_apk_key, new_variant))
    if force:
        logging.info('Deleting /data and /cache.')
        return True
    logging.info('Automatically delete and skip this prompt by specifying '
                 '--force.')
    return boolean_prompt('Delete /data and /cache?', default=True)

  # Switching from/to user build.
  if (device_variant == 'user') != (new_variant == 'user'):
    logging.warn('\n\n** You are switching build variant (%s -> %s).  If you '
                 'have ever run with the old image, make sure to wipe out '
                 '/data first before starting the container. **\n',
                 device_variant, new_variant)
  return False


def _get_remote_device_apk_key(remote_proxy):
  """Retrieves the APK key signature of the remote test device.

    Args:
        remote_proxy: RemoteProxy instance for the remote test device.
  """
  remote_packages_xml = os.path.join(_CONTAINER_INSTANCE_ROOT_WILDCARD,
                                     'root/data/system/packages.xml')
  with TemporaryDirectory() as tmp_dir:
    host_packages_xml = os.path.join(tmp_dir.name, 'packages.xml')
    remote_proxy.pull(remote_packages_xml, host_packages_xml)
    return _get_apk_key_from_xml(host_packages_xml)


def _get_apk_key_from_xml(xml_file):
  """Parses |xml_file| to determine the APK key signature.

    Args:
        xml_file: The XML file to parse.
  """
  if not os.path.exists(xml_file):
    logging.warning('XML file doesn\'t exist: %s' % xml_file)
    return _APK_KEY_UNKNOWN

  root = ElementTree.parse(xml_file).getroot()
  gms_core_elements = root.findall('package[@name=\'%s\']'
                                   % _GMS_CORE_PACKAGE_NAME)
  assert len(gms_core_elements) == 1, ('Invalid number of GmsCore package '
                                       'elements. Expected: 1 Actual: %d'
                                       % len(gms_core_elements))
  gms_core_element = gms_core_elements[0]
  sigs_element = gms_core_element.find('sigs')
  assert sigs_element, ('Unable to find the |sigs| tag under the GmsCore '
                        'package tag.')
  sigs_count_attribute = int(sigs_element.get('count'))
  assert sigs_count_attribute == 1, ('Invalid signature count. Expected: 1 '
                                     'Actual: %d' % sigs_count_attribute)
  cert_element = sigs_element.find('cert')
  gms_core_cert_index = int(cert_element.get('index', -1))
  logging.debug('GmsCore cert index: %d' % gms_core_cert_index)
  if gms_core_cert_index == -1:
    logging.warning('Invalid cert index (%d)' % gms_core_cert_index)
    return _APK_KEY_UNKNOWN

  cert_key = cert_element.get('key')
  if cert_key:
    return _get_android_key_type_from_cert_key(cert_key)

  # The GmsCore package element for |cert| contains the cert index, but not the
  # cert key. Find its the matching cert key.
  for cert_element in root.findall('package/sigs/cert'):
    cert_index = int(cert_element.get('index'))
    cert_key = cert_element.get('key')
    if cert_key and cert_index == gms_core_cert_index:
      return _get_android_key_type_from_cert_key(cert_key)
  logging.warning('Unable to find a cert key matching index %d' % cert_index)
  return _APK_KEY_UNKNOWN


def _get_android_key_type_from_cert_key(cert_key):
  """Returns |_APK_KEY_RELEASE| if |cert_key| contains the Android release key
     signature substring, otherwise it returns |_APK_KEY_DEBUG|."""
  if _ANDROID_REL_KEY_SIGNATURE_SUBSTRING in cert_key:
    return _APK_KEY_RELEASE
  else:
    return _APK_KEY_DEBUG


def _find_build_property(line, build_property_name):
  """Returns the value that matches |build_property_name| in |line|."""
  if line.startswith(build_property_name):
    return line[len(build_property_name):].strip()
  return None


class ContainerImageEditor(object):
  """A context object that allows edits to the Android container image"""

  def __init__(self, mksquashfs_path, unsquashfs_path, image_path, mount_point,
               out_dir, file_contexts_path=None):
    self._mksquashfs_path = mksquashfs_path
    self._unsquashfs_path = unsquashfs_path
    self._image_path = image_path
    self._mount_point = mount_point
    self._out_dir = out_dir
    self.file_contexts_path = file_contexts_path
    # Since we shift UIDs/GIDs of all extracted files there, we are going to
    # need sudo permission to remove the temporary directory. shutil doesn't
    # have the ability to run as sudo, so not using TemporaryDirectory() here.
    self.tmp_dir_name = tempfile.mkdtemp()

  def __enter__(self):
    self._is_squashfs = lib.util.get_image_type(self._image_path) is 'squashfs'
    logging.debug('Is the file system squashfs? %s', self._is_squashfs)

    if self._is_squashfs:
      # Extract the files.
      lib.util.check_call(self._unsquashfs_path, '-force', '-d',
                          self.tmp_dir_name, self._image_path, sudo=True)

      self._update_file_context_path()
      if not os.path.exists(self.file_contexts_path):
        raise EnvironmentError('"%s" not found.' % self.file_contexts_path)
    else:
      self._mount_wrapper = MountWrapper(self._image_path, self.tmp_dir_name)
      self._mount_wrapper.__enter__()
      self._update_file_context_path()

    return self

  def __exit__(self, exception_type, exception_value, traceback):
    try:
      if self._is_squashfs:
        # Re-compress the files back to raw.img.
        lib.util.check_call(
            self._mksquashfs_path, self.tmp_dir_name, self._image_path,
            '-no-progress', '-comp', 'gzip', '-no-exports', '-noappend',
            '-mount-point', self._mount_point, '-product-out', self._out_dir,
            '-context-file', self.file_contexts_path, '-no-recovery',
            '-no-fragments', '-no-duplicates', '-b', '131072', '-t', '0',
            sudo=True)
      else:
        self._mount_wrapper.__exit__(exception_type, exception_value, traceback)
    finally:
        lib.util.check_call('rm', '-rf', self.tmp_dir_name, sudo=True)

  def _update_file_context_path(self):
    if self.file_contexts_path:
      logging.debug('file_contexts_path is already set')
      return
    self.file_contexts_path = os.path.join(_make_tempdir_deleted_on_exit(),
                                           'file_contexts')
    file_contexts_path = os.path.join(self.tmp_dir_name, 'file_contexts')
    if os.path.exists(file_contexts_path):
      logging.debug('Found file_contexts in image')
      lib.util.check_call('cp', file_contexts_path, self.file_contexts_path)
      return
    plat_file_contexts_path = os.path.join(self.tmp_dir_name,
                                           'plat_file_contexts')
    nonplat_file_contexts_path = os.path.join(self.tmp_dir_name,
                                              'nonplat_file_contexts')
    if (os.path.exists(plat_file_contexts_path) and
       os.path.exists(nonplat_file_contexts_path)):
      logging.debug('Combining \'plat_file_contexts\' and '
                    '\'nonplat_file_contexts\' files')
      with TemporaryDirectory() as tmp_dir:
        file_contexts_path = os.path.join(tmp_dir.name, 'file_contexts')
        with open(file_contexts_path, 'w') as outfile:
          if os.path.exists(plat_file_contexts_path):
            logging.debug('Writing plat_file_contexts to %s',
                          file_contexts_path)
          with open(plat_file_contexts_path) as infile:
            for line in infile:
              outfile.write(line)
          if os.path.exists(nonplat_file_contexts_path):
            logging.debug('Writing nonplat_file_contexts to %s',
                          file_contexts_path)
            with open(nonplat_file_contexts_path) as infile:
              for line in infile:
                outfile.write(line)
        if not os.path.exists(file_contexts_path):
          raise EnvironmentError('%s not found.' % file_contexts_path)
        lib.util.check_call('cp', file_contexts_path, self.file_contexts_path)
      return
    raise EnvironmentError('Unable to find file_contexts or '
                           '[non]plat_file_contexts in the image')


class BaseProvider(object):
  """Base class of image provider.

  Subclass should provide a directory with images in it.
  """

  def __init__(self):
    self._build_variant = None
    self._build_version_sdk = None

  def prepare(self):
    """Subclass should prepare image in its implementation.

    Subclass must return the (image directory, product, fingerprint) tuple.
    Product is a string like "cheets_arm".  Fingerprint is the string that
    will be updated to CHROMEOS_ARC_VERSION in /etc/lsb-release.
    """
    raise NotImplementedError()

  def get_build_variant(self):
    """ Returns the extracted build variant."""
    return self._build_variant

  def get_build_version_sdk(self):
    """ Returns the extracted Android SDK version."""
    return self._build_version_sdk

  def read_build_prop_file(self, build_prop_file, remove_file=True):
    """ Reads the specified build property file, and extracts the
    "ro.build.variant" and "ro.build.version.sdk" fields. This method optionally
    deletes |build_prop_file| when done

    Args:
        build_prop_file: The fully qualified path to the build.prop file.
        remove_file: Removes the |build_prop_file| when done. (default=True)
    """
    logging.debug('Reading build prop file: %s', build_prop_file)
    with open(build_prop_file, 'r') as f:
      for line in f:
        if self._build_version_sdk is None:
          value = _find_build_property(line, _RO_BUILD_VERSION_SDK)
          if value is not None:
            self._build_version_sdk = int(value)
        if self._build_variant is None:
          value = _find_build_property(line, _RO_BUILD_TYPE)
          if value is not None:
            self._build_variant = value
        if self._build_variant and self._build_version_sdk:
          break
    if remove_file:
      logging.info('Deleting prop file: %s...', build_prop_file)
      os.remove(build_prop_file)


class PrebuiltProvider(BaseProvider):
  """A provider to provides prebuilt image from Android builder."""

  def __init__(self, product, build_variant, build_id, simg2img,
               unsquashfs_path):
    super(PrebuiltProvider, self).__init__()
    self._product = product
    self._build_variant = build_variant
    self._build_id = build_id
    self._simg2img = simg2img
    self._unsquashfs_path = unsquashfs_path

  def prepare(self):
    fingerprint = '_'.join([self._product, self._build_variant, self._build_id])

    out_dir = _make_tempdir_deleted_on_exit()
    filename = _download_artifact(out_dir, self._build_id, self._product,
                                  self._build_variant)
    _extract_artifact(self._simg2img, self._unsquashfs_path, out_dir, filename)

    build_prop_file = os.path.join(out_dir, 'build.prop')
    self.read_build_prop_file(build_prop_file)
    return out_dir, self._product, fingerprint


class LocalPrebuiltProvider(BaseProvider):
  """A provider that provides prebuilt image from a local file."""

  def __init__(self, prebuilt_file, simg2img, unsquashfs_path):
    super(LocalPrebuiltProvider, self).__init__()
    self._prebuilt_file = prebuilt_file
    self._simg2img = simg2img
    self._unsquashfs_path = unsquashfs_path

  def prepare(self):
    out_dir = _make_tempdir_deleted_on_exit()
    logging.debug('Extracting artifacts')
    _extract_artifact(self._simg2img, self._unsquashfs_path, out_dir,
                      self._prebuilt_file)

    build_prop_file = os.path.join(out_dir, 'build.prop')
    self.read_build_prop_file(build_prop_file)
    if self._build_variant is None:
      self._build_variant = 'user'  # default to non-eng

    m = re.match(r'((?:bertha|cheets)_\w+)-img-P?\d+\.zip',
                 os.path.basename(self._prebuilt_file))
    if not m:
      sys.exit('Unrecognized file name of prebuilt image archive.')
    product = m.group(1)

    fingerprint = os.path.splitext(os.path.basename(self._prebuilt_file))[0]
    return out_dir, product, fingerprint


class LocalBuildProvider(BaseProvider):
  """A provider that provides local built image."""

  def __init__(self, build_fingerprint, skip_build_prop_update,
               added_build_properties, dryrun):
    super(LocalBuildProvider, self).__init__()
    self._build_fingerprint = build_fingerprint
    self._skip_build_prop_update = skip_build_prop_update
    self._added_build_properties = added_build_properties
    self._dryrun = dryrun
    expected_env = ('TARGET_BUILD_VARIANT', 'TARGET_PRODUCT', 'OUT')
    if not all(var in os.environ for var in expected_env):
      sys.exit('Did you run lunch?')
    self._build_variant = os.environ.get('TARGET_BUILD_VARIANT')
    self._target_product = os.environ.get('TARGET_PRODUCT')
    self._out_dir = os.environ.get('OUT')

    # Create the generic device name by extracting the architecture type
    # from the target product.
    generic_device = _GENERIC_DEVICE % dict(
        arch=lib.util.get_product_arch(self._target_product),
        product=lib.util.get_product_name(self._target_product))
    new_prop_value = '%s%s' % (_RO_PRODUCT_DEVICE, generic_device)
    if new_prop_value not in self._added_build_properties:
      self._added_build_properties.append(new_prop_value)

  def prepare(self):
    # Use build fingerprint if set. Otherwise, read it from the text file.
    build_fingerprint = self._build_fingerprint
    if not build_fingerprint:
      fingerprint_filepath = os.path.join(self._out_dir,
                                          'build_fingerprint.txt')
      if os.path.isfile(fingerprint_filepath):
        with open(fingerprint_filepath) as f:
          build_fingerprint = f.read().strip().replace('/', '_')

    # Find the absolute path of build.prop.
    build_prop_file = os.path.join(self._out_dir, 'system/build.prop')
    if not self._skip_build_prop_update:
      self._update_local_build_prop_file(build_prop_file)
    self.read_build_prop_file(build_prop_file, False)
    return self._out_dir, self._target_product, build_fingerprint

  def _update_local_build_prop_file(self, build_prop_file):
    """Updates build.prop of the local prebuilt image."""

    if not build_prop_file:
      logging.warning('Skipping. build_prop_file was not specified.')
      return

    file_updated = False
    for prop in self._added_build_properties:
      key_and_value = prop.split('=')
      # Check whether the property is already in build.prop.
      try:
        current_prop_value = lib.util.check_output('grep',
                                                   '%s=' % (key_and_value[0]),
                                                   build_prop_file).strip()
      except subprocess.CalledProcessError:
        # grep did not find a match
        current_prop_value = None

      if not current_prop_value:
        file_updated = True
        logging.info('Appending "%s" to build.prop...', prop)

        with PreserveTimestamps(build_prop_file) as f:
          lib.util.check_call(
              '/bin/sed', '-i',
              r'$a%s' % prop,
              f.path)
      elif prop != current_prop_value:
        file_updated = True
        logging.info('Setting "%s" to "%s" in build.prop...',
                     key_and_value[0], key_and_value[1])
        with PreserveTimestamps(build_prop_file) as f:
          # Make the changes to build.prop
          lib.util.check_call(
              '/bin/sed', '-i',
              r's/^\(%(_KEY)s=\).*/\1%(_VALUE)s/'
              % {'_KEY': key_and_value[0], '_VALUE': key_and_value[1]},
              f.path)

    if not file_updated:
      logging.info('build.prop does not need to be updated.')
      return

    logging.info('Recreating the system image with the updated build.prop ' +
                 'file...')
    system_dir = os.path.join(self._out_dir, 'system')
    system_image_info_file = os.path.join(
        self._out_dir,
        'obj/PACKAGING/systemimage_intermediates/system_image_info.txt')
    system_image_file = os.path.join(self._out_dir, 'system.img')
    if self._dryrun:
      return
    with PreserveTimestamps(system_image_file) as f:
      # Recreate system.img
      lib.util.check_call(
          './build/tools/releasetools/build_image.py',
          system_dir,
          system_image_info_file,
          f.path,
          system_dir)


class NullProvider(BaseProvider):
  """ Provider used for dry runs """

  def __init__(self):
    super(NullProvider, self).__init__()
    self._build_variant = 'user'
    self._build_version_sdk = 1

  def prepare(self):
    return ('<dir>', '<product>', '<fingerprint>')


def _parse_prebuilt(param):
  m = re.search(
      r'^((?:bertha|cheets)_(?:arm|x86|x86_64))/(user|userdebug|eng)/(P?\d+)$',
      param)
  if not m:
    sys.exit('Invalid format of --use-prebuilt')
  return m.group(1), m.group(2), m.group(3)


def _default_mksquashfs_path():
  # Automatically resolve mksquashfs path if possible.
  android_host_out_path = os.environ.get('ANDROID_HOST_OUT')
  if android_host_out_path:
    path = os.path.join(android_host_out_path, 'bin', 'mksquashfs')
    if os.path.isfile(path):
      return path
  path = os.path.join(_SCRIPT_DIR, 'mksquashfs')
  if os.path.isfile(path):
    return path
  return None


def _default_unsquashfs_path():
  # Automatically resolve unsquashfs path if possible.
  _UNSQUASHFS_PATH = '/usr/bin/unsquashfs'
  if os.path.exists(_UNSQUASHFS_PATH):
    return _UNSQUASHFS_PATH
  return None


def _default_shift_uid_py_path():
  # Automatically resolve shift_uid.py path if possible.
  path = os.path.join(_SCRIPT_DIR, 'shift_uid.py')
  if os.path.isfile(path):
    return path
  _SHIFT_UID_PATH = os.path.join(
      lib.util.find_repo_root(),
      'vendor/google_devices/bertha/scripts/shift_uid.py')
  if os.path.exists(_SHIFT_UID_PATH):
    return _SHIFT_UID_PATH
  return None


def _default_simg2img_path():
  # Automatically resolve simg2img path if possible.
  dirs_to_find = []
  if 'ANDROID_HOST_OUT' in os.environ:
    dirs_to_find.append(os.path.join(os.environ.get('ANDROID_HOST_OUT'), 'bin'))
  dirs_to_find.append(_SCRIPT_DIR)
  if 'PATH' in os.environ:
    dirs_to_find += os.environ['PATH'].split(os.pathsep)

  for dir in dirs_to_find:
    path = os.path.join(dir, 'simg2img')
    if os.path.isfile(path) and os.access(path, os.X_OK):
      return path
  return None


def _resolve_args(args):
  if not args.simg2img_path:
    sys.exit('Cannot determine the path of simg2img')
  if not args.mksquashfs_path:
    sys.exit('Cannot determine the path of mksquashfs')
  if not args.unsquashfs_path:
    sys.exit('Cannot determine the path of unsquashfs. You may need to '
             'install it with sudo apt install squashfs-tools.')


def _parse_args():
  """Parses the arguments."""
  parser = argparse.ArgumentParser(
      formatter_class=argparse.RawDescriptionHelpFormatter,
      description='Push image to Chromebook',
      epilog="""Examples:

To push from local build
$ %(prog)s <remote>

To push from Android build prebuilt
$ %(prog)s --use-prebuilt cheets_arm/eng/123456 <remote>

To push from local prebuilt
$ %(prog)s --use-prebuilt-file path/to/cheets_arm-img-123456.zip <remote>
""")
  parser.add_argument(
      '--push-vendor-image', action='store_true', help='Push vendor image')
  parser.add_argument(
      '--use-prebuilt', metavar='PRODUCT/BUILD_VARIANT/BUILD_ID',
      type=_parse_prebuilt,
      help='Push prebuilt image instead.  Example value: cheets_arm/eng/123456')
  parser.add_argument(
      '--use-prebuilt-file', dest='prebuilt_file', metavar='<path>',
      help='The downloaded image path')
  parser.add_argument(
      '--build-fingerprint', default=os.environ.get('BUILD_FINGERPRINT'),
      help='If set, embed this fingerprint data to the /etc/lsb-release '
      'as CHROMEOS_ARC_VERSION value.')
  parser.add_argument(
      '--dryrun', action='store_true',
      help='Do not execute subprocesses.')
  parser.add_argument(
      '--loglevel', default='INFO',
      choices=('DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'),
      help='Logging level.')
  parser.add_argument(
      '--simg2img-path', default=_default_simg2img_path(),
      help='Executable path of simg2img')
  parser.add_argument(
      '--mksquashfs-path', default=_default_mksquashfs_path(),
      help='Executable path of mksquashfs')
  parser.add_argument(
      '--unsquashfs-path', default=_default_unsquashfs_path(),
      help='Executable path of unsquashfs')
  parser.add_argument(
      '--shift-uid-py-path', default=_default_shift_uid_py_path(),
      help='Executable path of shift_uid.py')
  parser.add_argument(
      '--shift-ugids', action='store_true', help='Shift UIDs/GIDs recursively')
  parser.add_argument(
      '--force', action='store_true',
      help=('Skip all prompts (i.e., for disabling of rootfs verification).  '
            'This may result in the target machine being rebooted'))
  parser.add_argument(
      '--try-clobber-data', action='store_true',
      help='If currently logged in, also clobber /data and /cache')
  parser.add_argument(
      '--skip_build_prop_update', action='store_true',
      help=('Do not change ro.product.device to  "generic_cheets" for local '
            'builds'))
  parser.add_argument(
      '--push-to-stateful-partition', action='store_true',
      help=('Place the system.raw.img on the stateful partition instead of /. '
            'This is always used for builds that do not fit on /.'))
  parser.add_argument(
      '--add-build-property', action='append', default=[],
      dest='added_build_properties',
      help=('Update build.prop with the given property e.g. some.property=true,'
            'If the given property exists, it would update the property.'
            'Otherwise it would append the given property to the end.'))
  parser.add_argument(
      '--enable-assistant-prop', action='store_true',
      help=('Update build.prop with ro.opa.eligible_device=true, '
            'this is required to run assistant on ChromeOS.'))
  parser.add_argument(
      'remote',
      help=('The target test device. This is passed to ssh command etc., '
            'so IP or the name registered in your .ssh/config file can be '
            'accepted.'))
  args = parser.parse_args()

  _resolve_args(args)
  return args


def main():
  # Set up arguments.
  args = _parse_args()
  logging.basicConfig(level=getattr(logging, args.loglevel))

  if args.enable_assistant_prop:
    args.added_build_properties.append('ro.opa.eligible_device=true')

  simg2img = Simg2img(args.simg2img_path, args.dryrun)

  # Prepare local source.  A preparer is responsible to return an directory that
  # contains necessary files to push.  It also needs to return metadata like
  # product (e.g. cheets_arm) and a build fingerprint.
  if args.dryrun:
    provider = NullProvider()
  elif args.use_prebuilt:
    product, build_variant, build_id = args.use_prebuilt
    provider = PrebuiltProvider(product, build_variant, build_id, simg2img,
                                args.unsquashfs_path)
  elif args.prebuilt_file:
    provider = LocalPrebuiltProvider(args.prebuilt_file, simg2img,
                                     args.unsquashfs_path)
  else:
    provider = LocalBuildProvider(args.build_fingerprint,
                                  args.skip_build_prop_update,
                                  args.added_build_properties,
                                  args.dryrun)

  # Actually prepare the files to push.
  out, product, fingerprint = provider.prepare()

  # Update the image.
  remote_proxy = RemoteProxy(args.remote, args.dryrun)
  _verify_android_sdk_version(remote_proxy, provider, args.dryrun)
  _verify_machine_arch(remote_proxy, product, args.dryrun)

  if args.try_clobber_data:
    clobber_data = True
  else:
    clobber_data = _detect_cert_inconsistency(
        args.force, remote_proxy, provider.get_build_variant(), args.dryrun)

  logging.info('Converting images to raw images...')
  (large_image_list, image_list) = _convert_images(
      simg2img, out, product, args.push_vendor_image, args.shift_ugids,
      args.mksquashfs_path, args.unsquashfs_path, args.shift_uid_py_path, args.dryrun)

  is_selinux_policy_updated = _is_selinux_policy_updated(remote_proxy, out,
                                                         args.dryrun)
  is_selinux_file_contexts_updated = _is_selinux_file_contexts_updated(
      remote_proxy, out, args.dryrun)
  total_bytes = sum(os.stat(filename).st_size for filename in large_image_list)
  free_bytes = _get_free_space(remote_proxy)
  push_to_stateful = (args.push_to_stateful_partition or
                      total_bytes >= free_bytes)

  if not args.push_to_stateful_partition and push_to_stateful:
    logging.info('Pushing image to stateful partition '
                 'since it does not fit on / (%.2f MiB, %.2f free MiB).',
                 float(total_bytes) / _MB, float(free_bytes) / _MB)

  with ImageUpdateMode(remote_proxy, is_selinux_policy_updated,
                       push_to_stateful, clobber_data, args.force):
    is_debuggable = 'user' != provider.get_build_variant()
    try:
      remote_proxy.check_call(' '.join([
          '/bin/sed', '-i',
          r'"s/^\(export ANDROID_DEBUGGABLE=\).*/\1%(_IS_DEBUGGABLE)d/"',
          '/etc/init/arc-setup-env'
      ]) % {'_IS_DEBUGGABLE': is_debuggable})
      # Unconditionally disable font sharing so that 'adb sync' will always
      # work. Disabling the feature is safe because locally built system
      # image always has all fonts. Images from ab/ also have all fonts.
      remote_proxy.check_call(' '.join([
          '/bin/sed', '-i',
          r'"s/^\(export SHARE_FONTS=\).*/\1%(_SHARE_FONTS)d/"',
          '/etc/init/arc-setup-env'
      ]) % {'_SHARE_FONTS': False})
    except Exception:
      # The device is old and doesn't have arc-setup-env. Fall back to the
      # older method.
      # TODO(yusukes): Remove the fallback code.
      remote_proxy.check_call(' '.join([
          '/bin/sed', '-i',
          r'"s/^\(env ANDROID_DEBUGGABLE=\).*/\1%(_IS_DEBUGGABLE)d/"',
          '/etc/init/arc-setup.conf'
      ]) % {'_IS_DEBUGGABLE': is_debuggable})
      remote_proxy.check_call(' '.join([
          '/bin/sed', '-i',
          r'"s/^\(env SHARE_FONTS=\).*/\1%(_SHARE_FONTS)d/"',
          '/etc/init/arc-system-mount.conf'
      ]) % {'_SHARE_FONTS': False})

    logging.info('Syncing image files to ChromeOS...')
    if large_image_list:
      remote_proxy.sync(large_image_list,
                        _ANDROID_ROOT_STATEFUL if push_to_stateful else
                        _ANDROID_ROOT)
    if image_list:
      remote_proxy.sync(image_list, _ANDROID_ROOT)
    _update_build_fingerprint(remote_proxy, fingerprint)
    if is_selinux_file_contexts_updated:
      _update_selinux_file_contexts(remote_proxy, out)
    if is_selinux_policy_updated:
      _update_selinux_policy(remote_proxy, out)


if __name__ == '__main__':
  main()
