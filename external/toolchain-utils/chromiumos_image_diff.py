#!/usr/bin/env python2
"""Diff 2 chromiumos images by comparing each elf file.

   The script diffs every *ELF* files by dissembling every *executable*
   section, which means it is not a FULL elf differ.

   A simple usage example -
     chromiumos_image_diff.py --image1 image-path-1 --image2 image-path-2

   Note that image path should be inside the chroot, if not (ie, image is
   downloaded from web), please specify a chromiumos checkout via
   "--chromeos_root".

   And this script should be executed outside chroot.
"""

from __future__ import print_function

__author__ = 'shenhan@google.com (Han Shen)'

import argparse
import os
import re
import sys
import tempfile

import image_chromeos
from cros_utils import command_executer
from cros_utils import logger
from cros_utils import misc


class CrosImage(object):
  """A cros image object."""

  def __init__(self, image, chromeos_root, no_unmount):
    self.image = image
    self.chromeos_root = chromeos_root
    self.mounted = False
    self._ce = command_executer.GetCommandExecuter()
    self.logger = logger.GetLogger()
    self.elf_files = []
    self.no_unmount = no_unmount
    self.unmount_script = ''
    self.stateful = ''
    self.rootfs = ''

  def MountImage(self, mount_basename):
    """Mount/unpack the image."""

    if mount_basename:
      self.rootfs = '/tmp/{0}.rootfs'.format(mount_basename)
      self.stateful = '/tmp/{0}.stateful'.format(mount_basename)
      self.unmount_script = '/tmp/{0}.unmount.sh'.format(mount_basename)
    else:
      self.rootfs = tempfile.mkdtemp(
          suffix='.rootfs', prefix='chromiumos_image_diff')
      ## rootfs is like /tmp/tmpxyz012.rootfs.
      match = re.match(r'^(.*)\.rootfs$', self.rootfs)
      basename = match.group(1)
      self.stateful = basename + '.stateful'
      os.mkdir(self.stateful)
      self.unmount_script = '{0}.unmount.sh'.format(basename)

    self.logger.LogOutput('Mounting "{0}" onto "{1}" and "{2}"'.format(
        self.image, self.rootfs, self.stateful))
    ## First of all creating an unmount image
    self.CreateUnmountScript()
    command = image_chromeos.GetImageMountCommand(
        self.chromeos_root, self.image, self.rootfs, self.stateful)
    rv = self._ce.RunCommand(command, print_to_console=True)
    self.mounted = (rv == 0)
    if not self.mounted:
      self.logger.LogError('Failed to mount "{0}" onto "{1}" and "{2}".'.format(
          self.image, self.rootfs, self.stateful))
    return self.mounted

  def CreateUnmountScript(self):
    command = ('sudo umount {r}/usr/local {r}/usr/share/oem '
               '{r}/var {r}/mnt/stateful_partition {r}; sudo umount {s} ; '
               'rmdir {r} ; rmdir {s}\n').format(
                   r=self.rootfs, s=self.stateful)
    f = open(self.unmount_script, 'w')
    f.write(command)
    f.close()
    self._ce.RunCommand(
        'chmod +x {}'.format(self.unmount_script), print_to_console=False)
    self.logger.LogOutput(
        'Created an unmount script - "{0}"'.format(self.unmount_script))

  def UnmountImage(self):
    """Unmount the image and delete mount point."""

    self.logger.LogOutput('Unmounting image "{0}" from "{1}" and "{2}"'.format(
        self.image, self.rootfs, self.stateful))
    if self.mounted:
      command = 'bash "{0}"'.format(self.unmount_script)
      if self.no_unmount:
        self.logger.LogOutput(('Please unmount manually - \n'
                               '\t bash "{0}"'.format(self.unmount_script)))
      else:
        if self._ce.RunCommand(command, print_to_console=True) == 0:
          self._ce.RunCommand('rm {0}'.format(self.unmount_script))
          self.mounted = False
          self.rootfs = None
          self.stateful = None
          self.unmount_script = None

    return not self.mounted

  def FindElfFiles(self):
    """Find all elf files for the image.

    Returns:
      Always true
    """

    self.logger.LogOutput(
        'Finding all elf files in "{0}" ...'.format(self.rootfs))
    # Note '\;' must be prefixed by 'r'.
    command = ('find "{0}" -type f -exec '
               'bash -c \'file -b "{{}}" | grep -q "ELF"\''
               r' \; '
               r'-exec echo "{{}}" \;').format(self.rootfs)
    self.logger.LogCmd(command)
    _, out, _ = self._ce.RunCommandWOutput(command, print_to_console=False)
    self.elf_files = out.splitlines()
    self.logger.LogOutput(
        'Total {0} elf files found.'.format(len(self.elf_files)))
    return True


class ImageComparator(object):
  """A class that wraps comparsion actions."""

  def __init__(self, images, diff_file):
    self.images = images
    self.logger = logger.GetLogger()
    self.diff_file = diff_file
    self.tempf1 = None
    self.tempf2 = None

  def Cleanup(self):
    if self.tempf1 and self.tempf2:
      command_executer.GetCommandExecuter().RunCommand(
          'rm {0} {1}'.format(self.tempf1, self.tempf2))
      logger.GetLogger(
          'Removed "{0}" and "{1}".'.format(self.tempf1, self.tempf2))

  def CheckElfFileSetEquality(self):
    """Checking whether images have exactly number of elf files."""

    self.logger.LogOutput('Checking elf file equality ...')
    i1 = self.images[0]
    i2 = self.images[1]
    t1 = i1.rootfs + '/'
    elfset1 = set([e.replace(t1, '') for e in i1.elf_files])
    t2 = i2.rootfs + '/'
    elfset2 = set([e.replace(t2, '') for e in i2.elf_files])
    dif1 = elfset1.difference(elfset2)
    msg = None
    if dif1:
      msg = 'The following files are not in "{image}" - "{rootfs}":\n'.format(
          image=i2.image, rootfs=i2.rootfs)
      for d in dif1:
        msg += '\t' + d + '\n'
    dif2 = elfset2.difference(elfset1)
    if dif2:
      msg = 'The following files are not in "{image}" - "{rootfs}":\n'.format(
          image=i1.image, rootfs=i1.rootfs)
      for d in dif2:
        msg += '\t' + d + '\n'
    if msg:
      self.logger.LogError(msg)
      return False
    return True

  def CompareImages(self):
    """Do the comparsion work."""

    if not self.CheckElfFileSetEquality():
      return False

    mismatch_list = []
    match_count = 0
    i1 = self.images[0]
    i2 = self.images[1]
    self.logger.LogOutput(
        'Start comparing {0} elf file by file ...'.format(len(i1.elf_files)))
    ## Note - i1.elf_files and i2.elf_files have exactly the same entries here.

    ## Create 2 temp files to be used for all disassembed files.
    handle, self.tempf1 = tempfile.mkstemp()
    os.close(handle)  # We do not need the handle
    handle, self.tempf2 = tempfile.mkstemp()
    os.close(handle)

    cmde = command_executer.GetCommandExecuter()
    for elf1 in i1.elf_files:
      tmp_rootfs = i1.rootfs + '/'
      f1 = elf1.replace(tmp_rootfs, '')
      full_path1 = elf1
      full_path2 = elf1.replace(i1.rootfs, i2.rootfs)

      if full_path1 == full_path2:
        self.logger.LogError(
            'Error:  We\'re comparing the SAME file - {0}'.format(f1))
        continue

      command = (
          'objdump -d "{f1}" > {tempf1} ; '
          'objdump -d "{f2}" > {tempf2} ; '
          # Remove path string inside the dissemble
          'sed -i \'s!{rootfs1}!!g\' {tempf1} ; '
          'sed -i \'s!{rootfs2}!!g\' {tempf2} ; '
          'diff {tempf1} {tempf2} 1>/dev/null 2>&1').format(
              f1=full_path1,
              f2=full_path2,
              rootfs1=i1.rootfs,
              rootfs2=i2.rootfs,
              tempf1=self.tempf1,
              tempf2=self.tempf2)
      ret = cmde.RunCommand(command, print_to_console=False)
      if ret != 0:
        self.logger.LogOutput(
            '*** Not match - "{0}" "{1}"'.format(full_path1, full_path2))
        mismatch_list.append(f1)
        if self.diff_file:
          command = ('echo "Diffs of disassemble of \"{f1}\" and \"{f2}\"" '
                     '>> {diff_file} ; diff {tempf1} {tempf2} '
                     '>> {diff_file}').format(
                         f1=full_path1,
                         f2=full_path2,
                         diff_file=self.diff_file,
                         tempf1=self.tempf1,
                         tempf2=self.tempf2)
          cmde.RunCommand(command, print_to_console=False)
      else:
        match_count += 1
    ## End of comparing every elf files.

    if not mismatch_list:
      self.logger.LogOutput(
          '** COOL, ALL {0} BINARIES MATCHED!! **'.format(match_count))
      return True

    mismatch_str = 'Found {0} mismatch:\n'.format(len(mismatch_list))
    for b in mismatch_list:
      mismatch_str += '\t' + b + '\n'

    self.logger.LogOutput(mismatch_str)
    return False


def Main(argv):
  """The main function."""

  command_executer.InitCommandExecuter()
  images = []

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--no_unmount',
      action='store_true',
      dest='no_unmount',
      default=False,
      help='Do not unmount after finish, this is useful for debugging.')
  parser.add_argument(
      '--chromeos_root',
      dest='chromeos_root',
      default=None,
      action='store',
      help=('[Optional] Specify a chromeos tree instead of '
            'deducing it from image path so that we can compare '
            '2 images that are downloaded.'))
  parser.add_argument(
      '--mount_basename',
      dest='mount_basename',
      default=None,
      action='store',
      help=('Specify a meaningful name for the mount point. With this being '
            'set, the mount points would be "/tmp/mount_basename.x.rootfs" '
            ' and "/tmp/mount_basename.x.stateful". (x is 1 or 2).'))
  parser.add_argument(
      '--diff_file',
      dest='diff_file',
      default=None,
      help='Dumping all the diffs (if any) to the diff file')
  parser.add_argument(
      '--image1',
      dest='image1',
      default=None,
      required=True,
      help=('Image 1 file name.'))
  parser.add_argument(
      '--image2',
      dest='image2',
      default=None,
      required=True,
      help=('Image 2 file name.'))
  options = parser.parse_args(argv[1:])

  if options.mount_basename and options.mount_basename.find('/') >= 0:
    logger.GetLogger().LogError(
        '"--mount_basename" must be a name, not a path.')
    parser.print_help()
    return 1

  result = False
  image_comparator = None
  try:
    for i, image_path in enumerate([options.image1, options.image2], start=1):
      image_path = os.path.realpath(image_path)
      if not os.path.isfile(image_path):
        logger.getLogger().LogError('"{0}" is not a file.'.format(image_path))
        return 1

      chromeos_root = None
      if options.chromeos_root:
        chromeos_root = options.chromeos_root
      else:
        ## Deduce chromeos root from image
        t = image_path
        while t != '/':
          if misc.IsChromeOsTree(t):
            break
          t = os.path.dirname(t)
        if misc.IsChromeOsTree(t):
          chromeos_root = t

      if not chromeos_root:
        logger.GetLogger().LogError(
            'Please provide a valid chromeos root via --chromeos_root')
        return 1

      image = CrosImage(image_path, chromeos_root, options.no_unmount)

      if options.mount_basename:
        mount_basename = '{basename}.{index}'.format(
            basename=options.mount_basename, index=i)
      else:
        mount_basename = None

      if image.MountImage(mount_basename):
        images.append(image)
        image.FindElfFiles()

    if len(images) == 2:
      image_comparator = ImageComparator(images, options.diff_file)
      result = image_comparator.CompareImages()
  finally:
    for image in images:
      image.UnmountImage()
    if image_comparator:
      image_comparator.Cleanup()

  return 0 if result else 1


if __name__ == '__main__':
  Main(sys.argv)
