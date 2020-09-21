# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils


class graphics_GLAPICheck(graphics_utils.GraphicsTest):
    """
    Verify correctness of OpenGL/GLES.
    """
    version = 1
    error_message = ''

    def __check_extensions(self, info, ext_entries):
        info_split = info.split()
        comply = True
        for extension in ext_entries:
            match = extension in info_split
            if not match:
                self.error_message += ' ' + extension
                comply = False
        return comply

    def __check_gles_extensions(self, info):
        extensions = [
            'GL_OES_EGL_image',
            'EGL_KHR_image'
        ]
        extensions2 = [
            'GL_OES_EGL_image',
            'EGL_KHR_image_base',
            'EGL_KHR_image_pixmap'
        ]
        return (self.__check_extensions(info, extensions) or
                self.__check_extensions(info, extensions2))

    def __check_wflinfo(self):
        # TODO(ihf): Extend this function once gl(es)_APICheck code has
        # been upstreamed to waffle.
        version_major, version_minor = graphics_utils.get_gles_version()
        if version_major:
            version_info = ('GLES_VERSION = %d.%d' %
                            (version_major, version_minor))
            logging.info(version_info)
            # GLES version has to be 2.0 or above.
            if version_major < 2:
                self.error_message = ' %s' % version_info
                return False
            version_major, version_minor = graphics_utils.get_egl_version()
            if version_major:
                version_info = ('EGL_VERSION = %d.%d' %
                                (version_major, version_minor))
                logging.info(version_info)
                # EGL version has to be 1.3 or above.
                if (version_major == 1 and version_minor >= 3 or
                    version_major > 1):
                    logging.warning('Please add missing extension check. '
                                    'Details crbug.com/413079')
                    # return self.__check_gles_extensions(wflinfo + eglinfo)
                    return True
                else:
                    self.error_message = version_info
                    return False
            # No EGL version info found.
            self.error_message = ' missing EGL version info'
            return False
        # No GLES version info found.
        self.error_message = ' missing GLES version info'
        return False

    def initialize(self):
        super(graphics_GLAPICheck, self).initialize()

    def cleanup(self):
        super(graphics_GLAPICheck, self).cleanup()

    @graphics_utils.GraphicsTest.failure_report_decorator('graphics_GLAPICheck')
    def run_once(self):
        if not self.__check_wflinfo():
            raise error.TestFail('Failed: GLES API insufficient:' +
                                 self.error_message)
        return
