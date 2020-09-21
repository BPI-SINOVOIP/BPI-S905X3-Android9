#
# Copyright (C) 2017 The Android Open Source Project
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
#
"""Class to flash build artifacts onto devices"""

import hashlib
import logging
import os
import resource
import sys
import tempfile
import time

from host_controller import common
from vts.utils.python.common import cmd_utils
from vts.utils.python.controllers import android_device


class BuildFlasher(object):
    """Client that manages build flashing.

    Attributes:
        device: AndroidDevice, the device associated with the client.
    """

    def __init__(self, serial="", customflasher_path=""):
        """Initialize the client.

        If serial not provided, find single device connected. Error if
        zero or > 1 devices connected.

        Args:
            serial: optional string, serial number for the device.
            customflasher_path: optional string, set device to use specified
                                binary to flash a device
        """
        if serial != "":
          self.device = android_device.AndroidDevice(
                serial, device_callback_port=-1)
        else:
            serials = android_device.list_adb_devices()
            if len(serials) == 0:
                serials = android_device.list_fastboot_devices()
                if len(serials) == 0:
                    raise android_device.AndroidDeviceError(
                        "ADB and fastboot could not find any target devices.")
            if len(serials) > 1:
                print(
                    "ADB or fastboot found more than one device: %s" % serials)
            self.device = android_device.AndroidDevice(
                serials[0], device_callback_port=-1)
            if customflasher_path:
                self.device.SetCustomFlasherPath(customflasher_path)

    def SetSerial(self, serial):
        """Sets device serial.

        Args:
            serial: string, a device serial.

        Returns:
            True if successful; False otherwise.
        """
        if not serial:
            print("no serial is given to BuildFlasher.SetSerial.")
            return False

        self.device = android_device.AndroidDevice(
            serial, device_callback_port=-1)
        return True

    def FlashGSI(self, system_img, vbmeta_img=None, skip_check=False):
        """Flash the Generic System Image to the device.

        Args:
            system_img: string, path to GSI
            vbmeta_img: string, optional, path to vbmeta image for new devices
            skip_check: boolean, set True to skip adb-based checks when
                        the DUT is already running its bootloader.
        """
        if not os.path.exists(system_img):
            raise ValueError("Couldn't find system image at %s" % system_img)
        if not skip_check:
            self.device.adb.wait_for_device()
            if not self.device.isBootloaderMode:
                self.device.log.info(self.device.adb.reboot_bootloader())
        if vbmeta_img is not None:
            self.device.log.info(
                self.device.fastboot.flash('vbmeta', vbmeta_img))
        self.device.log.info(self.device.fastboot.erase('system'))
        self.device.log.info(self.device.fastboot.flash('system', system_img))
        self.device.log.info(self.device.fastboot.erase('metadata'))
        self.device.log.info(self.device.fastboot._w())
        self.device.log.info(self.device.fastboot.reboot())

    def Flashall(self, directory):
        """Flash all images in a directory to the device using flashall.

        Generally the directory is the result of unzipping the .zip from AB.
        Args:
            directory: string, path to directory containing images
        """
        # fastboot flashall looks for imgs in $ANDROID_PRODUCT_OUT
        os.environ['ANDROID_PRODUCT_OUT'] = directory
        self.device.adb.wait_for_device()
        if not self.device.isBootloaderMode:
            self.device.log.info(self.device.adb.reboot_bootloader())
        self.device.log.info(self.device.fastboot.flashall())

    def Flash(self, device_images):
        """Flash the Generic System Image to the device.

        Args:
            device_images: dict, where the key is partition name and value is
                           image file path.

        Returns:
            True if succesful; False otherwise
        """
        if not device_images:
            logging.warn("Flash skipped because no device image is given.")
            return False

        if not self.device.isBootloaderMode:
            self.device.adb.wait_for_device()
            print("rebooting to bootloader")
            self.device.log.info(self.device.adb.reboot_bootloader())

        print("checking to flash bootloader.img and radio.img")
        for partition in ["bootloader", "radio"]:
            if partition in device_images:
                image_path = device_images[partition]
                self.device.log.info("fastboot flash %s %s",
                                     partition, image_path)
                self.device.log.info(
                    self.device.fastboot.flash(partition, image_path))
                self.device.log.info("fastboot reboot_bootloader")
                self.device.log.info(self.device.fastboot.reboot_bootloader())

        print("starting to flash vendor and other images...")
        if common.FULL_ZIPFILE in device_images:
            print("fastboot update %s --skip-reboot" %
                  (device_images[common.FULL_ZIPFILE]))
            self.device.log.info(
                self.device.fastboot.update(
                    device_images[common.FULL_ZIPFILE],
                    "--skip-reboot"))

        for partition, image_path in device_images.iteritems():
            if partition in (common.FULL_ZIPFILE, "system", "vbmeta",
                             "bootloader", "radio"):
                continue
            if not image_path:
                self.device.log.warning("%s image is empty", partition)
                continue
            self.device.log.info("fastboot flash %s %s", partition, image_path)
            self.device.log.info(
                self.device.fastboot.flash(partition, image_path))

        print("starting to flash system and other images...")
        if "system" in device_images and device_images["system"]:
            system_img = device_images["system"]
            vbmeta_img = device_images["vbmeta"] if (
                "vbmeta" in device_images
                and device_images["vbmeta"]) else None
            self.FlashGSI(system_img, vbmeta_img, skip_check=True)
        else:
            self.device.log.info(self.device.fastboot.reboot())
        return True

    def FlashImage(self, device_images, reboot=False):
        """Flash specified image(s) to the device.

        Args:
            device_images: dict, where the key is partition name and value is
                           image file path.
            reboot: boolean, true to reboot the device.

        Returns:
            True if successful, False otherwise
        """
        if not device_images:
            logging.warn("Flash skipped because no device image is given.")
            return False

        if not self.device.isBootloaderMode:
            self.device.adb.wait_for_device()
            self.device.log.info(self.device.adb.reboot_bootloader())

        for partition, image_path in device_images.iteritems():
            if partition.endswith(".img"):
                partition = partition[:-4]
            self.device.log.info(
                self.device.fastboot.flash(partition, image_path))
        if reboot:
            self.device.log.info(self.device.fastboot.reboot())
        return True

    def WaitForDevice(self, timeout_secs=600):
        """Waits for the device to boot completely.

        Args:
            timeout_secs: integer, the maximum timeout value for this
                          operation (unit: seconds).

        Returns:
            True if device is booted successfully; False otherwise.
        """
        return self.device.waitForBootCompletion(timeout=timeout_secs)

    def FlashUsingCustomBinary(self,
                               device_images,
                               reboot_mode,
                               flasher_args,
                               timeout_secs_for_reboot=900):
        """Flash the customized image to the device.

        Args:
            device_images: dict, where the key is partition name and value is
                           image file path.
            reboot_mode: string, decides which mode device will reboot into.
                         ("bootloader"/"download").
            flasher_args: list of strings, arguments that will be passed to the
                          flash binary.
            timeout_secs_for_reboot: integer, the maximum timeout value for
                                     reboot to flash-able mode(unit: seconds).

        Returns:
            True if successful; False otherwise.
        """
        if not device_images:
            logging.warn("Flash skipped because no device image is given.")
            return False

        if not flasher_args:
            logging.error("No arguments.")
            return False

        if not self.device.isBootloaderMode:
            self.device.adb.wait_for_device()
            print("rebooting to %s mode" % reboot_mode)
            self.device.log.info(self.device.adb.reboot(reboot_mode))

        start = time.time()
        while not self.device.customflasher._l():
            if time.time() - start >= timeout_secs_for_reboot:
                logging.error(
                    "Timeout while waiting for %s mode boot completion." %
                    reboot_mode)
                return False
            time.sleep(1)

        flasher_output = self.device.customflasher.ExecCustomFlasherCmd(
            flasher_args[0],
            " ".join(flasher_args[1:] + [device_images["img"]]))
        self.device.log.info(flasher_output)

        return True

    def RepackageArtifacts(self, device_images, repackage_form):
        """Repackage artifacts into a given format.

        Once repackaged, device_images becomes
        {"img": "path_to_repackaged_image"}

        Args:
            device_images: dict, where the key is partition name and value is
                           image file path.
            repackage_form: string, format to repackage.

        Returns:
            True if succesful; False otherwise.
        """
        if not device_images:
            logging.warn("Repackage skipped because no device image is given.")
            return False

        if repackage_form == "tar.md5":
            tmp_file_name = next(tempfile._get_candidate_names()) + ".tar"
            tmp_dir_path = os.path.dirname(
                device_images[device_images.keys()[0]])
            for img in device_images:
                if os.path.dirname(device_images[img]) != tmp_dir_path:
                    os.rename(device_images[img],
                              os.path.join(tmp_dir_path, img))
                    device_images[img] = os.path.join(tmp_dir_path, img)

            current_dir = os.getcwd()
            os.chdir(tmp_dir_path)

            if sys.platform == "linux2":
                tar_cmd = "tar -cf %s %s" % (tmp_file_name, ' '.join(
                    (device_images.keys())))
            else:
                logging.error("Unsupported OS for the given repackage form.")
                return False
            logging.info(tar_cmd)
            std_out, std_err, err_code = cmd_utils.ExecuteOneShellCommand(
                tar_cmd)
            if err_code:
                logging.error(std_err)
                return False

            hash_md5 = hashlib.md5()
            try:
                with open(tmp_file_name, "rb") as file:
                    data_chunk = 0
                    chunk_size = resource.getpagesize()
                    while data_chunk != b'':
                        data_chunk = file.read(chunk_size)
                        hash_md5.update(data_chunk)
                    hash_ret = hash_md5.hexdigest()
                with open(tmp_file_name, "a") as file:
                    file.write("%s  %s" % (hash_ret, tmp_file_name))
            except IOError as e:
                logging.error(e.strerror)
                return False

            device_images.clear()
            device_images["img"] = os.path.join(tmp_dir_path, tmp_file_name)

            os.chdir(current_dir)
        else:
            logging.error(
                "Please specify correct repackage form: --repackage=%s" %
                repackage_form)
            return False

        return True
