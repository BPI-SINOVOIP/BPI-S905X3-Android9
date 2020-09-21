# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.site_utils.sponge_lib import acts_job_info
from autotest_lib.site_utils.sponge_lib import autotest_job_info


class DynamicJobInfo(autotest_job_info.AutotestJobInfo):
    """A job that will create tasks based on the info they contain."""

    def create_task_info(self, test):
        """Dynamically creates tasks based on the type of test run."""
        if test.subdir and 'android_ACTS' in test.subdir:
            logging.info('Using ACTS task info for %s.', test.testname)
            return acts_job_info.ACTSTaskInfo(test, self)

        return super(DynamicJobInfo, self).create_task_info(test)
