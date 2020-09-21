#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import importlib
import os
import stat

from host_controller import common
from host_controller.build import build_flasher
from host_controller.command_processor import base_command_processor


class CommandFlash(base_command_processor.BaseCommandProcessor):
    """Command processor for flash command.

    Attributes:
        arg_parser: ConsoleArgumentParser object, argument parser.
        console: cmd.Cmd console object.
        command: string, command name which this processor will handle.
        command_detail: string, detailed explanation for the command.
    """

    command = "flash"
    command_detail = "Flash images to a device."

    # @Override
    def SetUp(self):
        """Initializes the parser for flash command."""
        self.arg_parser.add_argument(
            "--image",
            help=("The file name of an image to flash."
                  " Used to flash a single image."))
        self.arg_parser.add_argument(
            "--current",
            metavar="PARTITION_IMAGE",
            nargs="*",
            type=lambda x: x.split("="),
            help="The partitions and images to be flashed. The format is "
            "<partition>=<image>. If PARTITION_IMAGE list is empty, "
            "currently fetched " + ", ".join(
                common._DEFAULT_FLASH_IMAGES) + " will be flashed.")
        self.arg_parser.add_argument(
            "--serial", default="", help="Serial number for device.")
        self.arg_parser.add_argument(
            "--build_dir",
            help="Directory containing build images to be flashed.")
        self.arg_parser.add_argument(
            "--gsi", help="Path to generic system image")
        self.arg_parser.add_argument("--vbmeta", help="Path to vbmeta image")
        self.arg_parser.add_argument(
            "--flasher_type",
            default="fastboot",
            help="Flasher type. Valid arguments are \"fastboot\", \"custom\", "
            "and full module name followed by class name. The class must "
            "inherit build_flasher.BuildFlasher, and implement "
            "__init__(serial, flasher_path) and "
            "Flash(device_images, additional_files, *flasher_args).")
        self.arg_parser.add_argument(
            "--flasher_path", default=None, help="Path to a flasher binary")
        self.arg_parser.add_argument(
            "flasher_args",
            metavar="ARGUMENTS",
            nargs="*",
            help="The arguments passed to the flasher binary. If any argument "
            "starts with \"-\", place all of them after \"--\" at end of "
            "line.")
        self.arg_parser.add_argument(
            "--reboot_mode",
            default="bootloader",
            choices=("bootloader", "download"),
            help="Reboot device to bootloader/download mode")
        self.arg_parser.add_argument(
            "--repackage",
            default="tar.md5",
            choices=("tar.md5"),
            help="Repackage artifacts into given format before flashing.")
        self.arg_parser.add_argument(
            "--wait-for-boot",
            default="true",
            help="false to not wait for devie booting.")
        self.arg_parser.add_argument(
            "--reboot", default="false", help="true to reboot the device(s).")

    # @Override
    def Run(self, arg_line):
        """Flash GSI or build images to a device connected with ADB."""
        args = self.arg_parser.ParseLine(arg_line)

        # path
        if (self.console.tools_info is not None
                and args.flasher_path in self.console.tools_info):
            flasher_path = self.console.tools_info[args.flasher_path]
        elif args.flasher_path:
            flasher_path = args.flasher_path
        else:
            flasher_path = ""
        if os.path.exists(flasher_path):
            flasher_mode = os.stat(flasher_path).st_mode
            os.chmod(flasher_path,
                     flasher_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        # serial numbers
        if args.serial:
            flasher_serials = [args.serial]
        elif self.console._serials:
            flasher_serials = self.console._serials
        else:
            flasher_serials = [""]

        # images
        if args.image:
            partition_image = {}
            partition_image[args.image] = self.console.device_image_info[
                args.image]
        else:
            if args.current:
                partition_image = dict((partition,
                                        self.console.device_image_info[image])
                                       for partition, image in args.current)
            else:
                partition_image = dict(
                    (image.rsplit(".img", 1)[0],
                     self.console.device_image_info[image])
                    for image in common._DEFAULT_FLASH_IMAGES
                    if image in self.console.device_image_info)

        # type
        if args.flasher_type in ("fastboot", "custom"):
            flasher_class = build_flasher.BuildFlasher
        else:
            class_path = args.flasher_type.rsplit(".", 1)
            flasher_module = importlib.import_module(class_path[0])
            flasher_class = getattr(flasher_module, class_path[1])
            if not issubclass(flasher_class, build_flasher.BuildFlasher):
                raise TypeError(
                    "%s is not a subclass of BuildFlasher." % class_path[1])

        flashers = [flasher_class(s, flasher_path) for s in flasher_serials]

        # Can be parallelized as long as that's proven reliable.
        for flasher in flashers:
            ret_flash = True
            if args.flasher_type == "fastboot":
                if args.image is not None:
                    ret_flash = flasher.FlashImage(partition_image, True
                                                   if args.reboot == "true"
                                                   else False)
                elif args.current is not None:
                    ret_flash = flasher.Flash(partition_image)
                else:
                    if args.gsi is None and args.build_dir is None:
                        self.arg_parser.error("Nothing requested: "
                                              "specify --gsi or --build_dir")
                        return False
                    if args.build_dir is not None:
                        ret_flash = flasher.Flashall(args.build_dir)
                    if args.gsi is not None:
                        ret_flash = flasher.FlashGSI(args.gsi, args.vbmeta)
            elif args.flasher_type == "custom":
                if flasher_path is not None:
                    if args.repackage is not None:
                        flasher.RepackageArtifacts(
                            self.console.device_image_info, args.repackage)
                    ret_flash = flasher.FlashUsingCustomBinary(
                        self.console.device_image_info, args.reboot_mode,
                        args.flasher_args, 300)
                else:
                    self.arg_parser.error(
                        "Please specify the path to custom flash tool.")
                    return False
            else:
                ret_flash = flasher.Flash(partition_image,
                                          self.console.tools_info,
                                          *args.flasher_args)
            if ret_flash == False:
                return False

        if args.wait_for_boot == "true":
            for flasher in flashers:
                ret_wait = flasher.WaitForDevice()
                if ret_wait == False:
                    return False
