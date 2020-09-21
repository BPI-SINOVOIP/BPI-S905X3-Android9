# Copyright (C) 2012 The Android Open Source Project
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

"""Emit extra commands needed for Group during OTA installation
(installing the bootloader)."""

import os
import sys
import shutil
import tempfile
import struct
import common
import sparse_img
import add_img_to_target_files

OPTIONS = common.OPTIONS
OPTIONS.ota_zip_check = True
OPTIONS.data_save = False
OPTIONS.backup_zip = True
OPTIONS.hdcp_key_write = False
OPTIONS.ddr_wipe = False

def SetBootloaderEnv(script, name, val):
  """Set bootloader env name with val."""
  script.AppendExtra('set_bootloader_env("%s", "%s");' % (name, val))

def HasTargetImage(target_files_zip, image_path):
  try:
    target_files_zip.getinfo(image_path)
    return True
  except KeyError:
    return False

def ZipOtherImage(which, tmpdir, output):
  """Returns an image object from IMAGES.

  'which' partition eg "logo", "dtb". A prebuilt image and file
  map must already exist in tmpdir.
  """

  amlogic_img_path = os.path.join(tmpdir, "IMAGES", which + ".img")
  if os.path.exists(amlogic_img_path):
    f = open(amlogic_img_path, "rb")
    data = f.read()
    f.close()
    common.ZipWriteStr(output, which + ".img", data)

def GetImage(which, tmpdir):
  """Returns an image object suitable for passing to BlockImageDiff.

  'which' partition must be "system" or "vendor". A prebuilt image and file
  map must already exist in tmpdir.
  """

  #assert which in ("system", "vendor", "odm", "product")

  path = os.path.join(tmpdir, "IMAGES", which + ".img")
  mappath = os.path.join(tmpdir, "IMAGES", which + ".map")

  # The image and map files must have been created prior to calling
  # ota_from_target_files.py (since LMP).
  assert os.path.exists(path) and os.path.exists(mappath)

  # Bug: http://b/20939131
  # In ext4 filesystems, block 0 might be changed even being mounted
  # R/O. We add it to clobbered_blocks so that it will be written to the
  # target unconditionally. Note that they are still part of care_map.
  clobbered_blocks = "0"

  return sparse_img.SparseImage(path, mappath, clobbered_blocks)

def AddCustomerImage(info, tmpdir):
  file_list = os.listdir(tmpdir + "/IMAGES")
  for file in file_list:
    if os.path.splitext(file)[1] == '.map':
      of = file.rfind('.')
      name = file[:of]
      if name not in ["system", "vendor", "odm", "product"]:
          tmp_tgt = GetImage(name, OPTIONS.input_tmp)
          tmp_tgt.ResetFileMap()
          tmp_diff = common.BlockDifference(name, tmp_tgt)
          tmp_diff.WriteScript(info.script, info.output_zip)

def FullOTA_Assertions(info):
  print "amlogic extensions:FullOTA_Assertions"
  try:
    bootloader_img = info.input_zip.read("RADIO/bootloader.img")
  except KeyError:
    OPTIONS.ota_partition_change = False
    print "no bootloader.img in target_files; skipping install"
  else:
    OPTIONS.ota_partition_change = True
    common.ZipWriteStr(info.output_zip, "bootloader.img", bootloader_img)
  if OPTIONS.ota_zip_check:
    info.script.AppendExtra('if ota_zip_check() == "1" then')
    if OPTIONS.data_save:
      info.script.AppendExtra('backup_data_partition_check("500");')
    info.script.AppendExtra('if recovery_backup_exist() == "0" then')
    info.script.AppendExtra('package_extract_file("dt.img", "/cache/recovery/dtb.img");')
    info.script.AppendExtra('package_extract_file("recovery.img", "/cache/recovery/recovery.img");')
    info.script.AppendExtra('endif;')
    info.script.AppendExtra('set_bootloader_env("upgrade_step", "3");')
    if OPTIONS.ota_partition_change:
      info.script.AppendExtra('ui_print("update bootloader.img...");')
      info.script.AppendExtra('write_bootloader_image(package_extract_file("bootloader.img"));')
      info.script.AppendExtra('set_bootloader_env("recovery_from_flash", "defenv_resev;save;reset");')
    # backup the update package to /cache or /dev/block/mmcblk0
    if OPTIONS.backup_zip:
      info.script.AppendExtra('backup_update_package("/dev/block/mmcblk0", "1894");')
    # use resize2fs for /dev/block/data and backup data partition
    if OPTIONS.data_save:
      info.script.AppendExtra('backup_data_partition();')
    info.script.AppendExtra('package_extract_file("logo.img", "/dev/block/logo");')
    info.script.AppendExtra('write_dtb_image(package_extract_file("dt.img"));')
    info.script.WriteRawImage("/recovery", "recovery.img")
    info.script.AppendExtra('delete_file("/cache/recovery/dtb.img");')
    info.script.AppendExtra('delete_file("/cache/recovery/recovery.img");')
    info.script.AppendExtra('reboot_recovery();')
    info.script.AppendExtra('else')

def FullOTA_InstallBegin(info):
  print "amlogic extensions:FullOTA_InstallBegin"
  SetBootloaderEnv(info.script, "upgrade_step", "3")
  if OPTIONS.data_save:
    info.script.AppendExtra('recovery_data_partition();')

def FullOTA_InstallEnd(info):
  print "amlogic extensions:FullOTA_InstallEnd"
  print "******has odm partition********* %s" % (OPTIONS.input_tmp)

  odm_tgt = GetImage("odm", OPTIONS.input_tmp)
  odm_tgt.ResetFileMap()
  odm_diff = common.BlockDifference("odm", odm_tgt)
  odm_diff.WriteScript(info.script, info.output_zip)

  product_tgt = GetImage("product", OPTIONS.input_tmp)
  product_tgt.ResetFileMap()
  product_diff = common.BlockDifference("product", product_tgt)
  product_diff.WriteScript(info.script, info.output_zip)

  AddCustomerImage(info, OPTIONS.input_tmp)

  ZipOtherImage("logo", OPTIONS.input_tmp, info.output_zip)
  ZipOtherImage("dt", OPTIONS.input_tmp, info.output_zip)
  ZipOtherImage("dtbo", OPTIONS.input_tmp, info.output_zip)
  ZipOtherImage("vbmeta", OPTIONS.input_tmp, info.output_zip)
  if not OPTIONS.two_step:
    ZipOtherImage("recovery", OPTIONS.input_tmp, info.output_zip)

  info.script.AppendExtra("""ui_print("update logo.img...");
package_extract_file("logo.img", "/dev/block/logo");
ui_print("update dtbo.img...");
package_extract_file("dtbo.img", "/dev/block/dtbo");
ui_print("update dtb.img...");
if recovery_backup_exist() == "0" then
backup_data_cache(dtb, /cache/recovery/);
backup_data_cache(recovery, /cache/recovery/);
endif;
write_dtb_image(package_extract_file("dt.img"));
ui_print("update recovery.img...");
package_extract_file("recovery.img", "/dev/block/recovery");
ui_print("update vbmeta.img...");
package_extract_file("vbmeta.img", "/dev/block/vbmeta");""")

  info.script.AppendExtra('delete_file("/cache/recovery/dtb.img");')
  info.script.AppendExtra('delete_file("/cache/recovery/recovery.img");')

  if OPTIONS.ota_partition_change:
    info.script.AppendExtra('ui_print("update bootloader.img...");')
    info.script.AppendExtra('write_bootloader_image(package_extract_file("bootloader.img"));')
    if OPTIONS.ddr_wipe:
      info.script.AppendExtra('wipe_ddr_parameter();')

  if OPTIONS.hdcp_key_write:
    info.script.AppendExtra('mount("ext4", "EMMC", "/dev/block/param", "/param");')
    info.script.AppendExtra('ui_print("update hdcp22_rx_fw...");')
    info.script.AppendExtra('write_hdcp_22rxfw();')
    info.script.AppendExtra('unmount("/param");')

  info.script.AppendExtra('if get_update_stage() == "2" then')
  info.script.FormatPartition("/metadata")
  info.script.FormatPartition("/tee")
  info.script.AppendExtra('wipe_cache();')
  info.script.AppendExtra('set_update_stage("0");')
  info.script.AppendExtra('endif;')

  SetBootloaderEnv(info.script, "upgrade_step", "1")
  SetBootloaderEnv(info.script, "force_auto_update", "false")

  if OPTIONS.ota_zip_check:
    info.script.AppendExtra('endif;')


def IncrementalOTA_VerifyBegin(info):
  print "amlogic extensions:IncrementalOTA_VerifyBegin"

def IncrementalOTA_VerifyEnd(info):
  print "amlogic extensions:IncrementalOTA_VerifyEnd"

def IncrementalOTA_InstallBegin(info):
  print "amlogic extensions:IncrementalOTA_InstallBegin"
  SetBootloaderEnv(info.script, "upgrade_step", "3")

def IncrementalOTA_ImageCheck(info, name):
  source_image = False; target_image = False; updating_image = False;

  image_path = "IMAGES/" + name + ".img"
  image_name = name + ".img"

  if HasTargetImage(info.source_zip, image_path):
    source_image = common.File(image_name, info.source_zip.read(image_path));

  if HasTargetImage(info.target_zip, image_path):
    target_image = common.File(image_name, info.target_zip.read(image_path));

  if target_image:
    if source_image:
      updating_image = (source_image.data != target_image.data);
    else:
      updating_image = 1;

  if updating_image:
    message_process = "install " + name + " image..."
    info.script.Print(message_process);
    common.ZipWriteStr(info.output_zip, image_name, target_image.data)
    if name == "dt":
      info.script.AppendExtra('write_dtb_image(package_extract_file("dt.img"));')
    else:
      if name == "bootloader":
        info.script.AppendExtra('write_bootloader_image(package_extract_file("bootloader.img"));')
      else:
        info.script.WriteRawImage("/" + name, image_name)

  if name == "bootloader":
    if updating_image:
      SetBootloaderEnv(info.script, "upgrade_step", "1")
    else:
      SetBootloaderEnv(info.script, "upgrade_step", "2")

def IncrementalOTA_Ext4ImageCheck(info, name):
  source_image = False; target_image = False; updating_image = False;

  image_path = "IMAGES/" + name + ".img"
  image_name = name + ".img"

  if HasTargetImage(info.source_zip, image_path):
    source_image = GetImage(name, OPTIONS.source_tmp)

  if HasTargetImage(info.target_zip, image_path):
    target_image = GetImage(name, OPTIONS.target_tmp)

  if source_image:
    if target_image:
      updating_image = common.BlockDifference(name, target_image, source_image,
                                       True,
                                       version=4,
                                       disable_imgdiff=False)
      updating_image.WriteScript(info.script, info.output_zip, progress=0.1)


def IncrementalOTA_InstallEnd(info):
  print "amlogic extensions:IncrementalOTA_InstallEnd"
  IncrementalOTA_Ext4ImageCheck(info, "odm");
  IncrementalOTA_Ext4ImageCheck(info, "product");
  IncrementalOTA_ImageCheck(info, "logo");
  IncrementalOTA_ImageCheck(info, "dt");
  IncrementalOTA_ImageCheck(info, "recovery");
  IncrementalOTA_ImageCheck(info, "vbmeta");
  info.script.FormatPartition("/metadata")
  IncrementalOTA_ImageCheck(info, "bootloader");
