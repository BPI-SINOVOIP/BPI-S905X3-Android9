#
#   Copyright 2018 - The Android Open Source Project
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

import logging
import struct

OS_VERSOIN_OFFSET_BOOTIMG = 44
SPL_YEAR_BASE_BOOTIMG = 2000
SPL_YEAR_MASK_BOOTIMG = 127 << 4
SPL_MONTH_MASK_BOOTIMG = 15


def GetSPLVersionFromBootImg(img_file_path):
    """Gets SPL version from given boot.img file path.

    Args:
        img_file_path : string, path to the boot.img file.
    Returns:
        A dict contains SPL version's year and date value.
    """
    try:
        fo = open(img_file_path, "rb")
        fo.seek(OS_VERSOIN_OFFSET_BOOTIMG, 0)

        os_version = fo.read(4)
    except IOError as e:
        logging.error(e.strerror)
        return {}

    os_version_int = struct.unpack("i", os_version)[0]

    year = (os_version_int & SPL_YEAR_MASK_BOOTIMG) >> 4
    month = os_version_int & SPL_MONTH_MASK_BOOTIMG

    version_date = {}
    version_date["year"] = SPL_YEAR_BASE_BOOTIMG + year
    version_date["month"] = month

    return version_date