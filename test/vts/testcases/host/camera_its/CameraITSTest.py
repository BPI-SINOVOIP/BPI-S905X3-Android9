#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
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

import inspect
import logging
import os
import re
import subprocess
import sys

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner


class CameraITSTest(base_test.BaseTestClass):
    """Running CameraITS tests in VTS.

    Attributes:
        _TABLET_TYPES: a list of strings, known tablet product types.
        dut1: AndroidDevice instance, for 1st device whose front-facing camera
              is tested.
        dut2: AndroidDevice instance, for 2nd device whose back-facing camera
              is tested.
        display_device: AndroidDevice instance, for a tablet device used as
                        a display screen.
    """
    _TABLET_TYPES = ["dragon"]

    # TODO: use config file to pass in:
    #          - serial for dut and screen
    #          - camera id
    #       so that we can run other test scenes with ITS-in-a-box
    def setUpClass(self):
        """Setup ITS running python environment and check for required python modules
        """
        # When VTS lab infra is used, the provided device order is:
        #   DUT (front-facing camera),
        #   DUT (back-facing camera),
        #   Tablet (display).
        # This order shall be preserved when a custom test serving infra is
        # used.
        self.dut1 = self.android_devices[0]
        self.dut2 = self.android_devices[1]
        self.display_device = self.android_devices[2]
        # In a local run (e.g., using a VTS test harness), the device order
        # can be arbitrary. So tablet is detected and chosen as a display
        # device. Similarly, we need a mechanism to detect DUT which uses
        # front-facing camera (that can be done here or inside another layer).

        if self.dut1.product_type in self._TABLET_TYPES:
            temp_device = self.dut1
            self.dut1 = self.dut2
            self.dut2 = self.dut3
            self.display_device = temp_device
        elif self.dut2.product_type in self._TABLET_TYPES:
            temp_device = self.dut2
            self.dut2 = self.display_device
            self.display_device = temp_device

        self.device_arg = "device=%s" % self.dut1.serial
        self.display_device_arg = "chart=%s" % self.display_device.serial
        self.its_path = str(
            os.path.abspath(os.path.join(self.data_file_path, 'CameraITS')))
        logging.info("cwd: %s", os.getcwd())
        logging.info("its_path: %s", self.its_path)
        self.out_path = logging.log_path
        os.environ["CAMERA_ITS_TOP"] = self.its_path
        # Below module check code assumes tradefed is running python 2.7
        # If tradefed switches to python3, then we will be checking modules in python3 while ITS
        # scripts should be ran in 2.7.
        if sys.version_info[:2] != (2, 7):
            logging.warning("Python version %s found; "
                            "CameraITSTest only tested with Python 2.7." %
                            (str(sys.version_info[:3])))
        logging.info("===============================")
        logging.info("Python path is: %s" % (sys.executable))
        logging.info("PYTHONPATH env is: " + os.environ["PYTHONPATH"])
        import PIL
        if hasattr(PIL, "__version__"):
            logging.info("PIL version is %s", PIL.__version__)
        logging.info("PIL path is " + inspect.getfile(PIL))
        from PIL import Image
        logging.info("Image path is " + inspect.getfile(Image))
        import numpy
        logging.info("numpy version is " + numpy.__version__)
        logging.info("numpy path is " + inspect.getfile(numpy))
        import scipy
        logging.info("scipy version is " + scipy.__version__)
        logging.info("scipy path is " + inspect.getfile(scipy))
        import matplotlib
        logging.info("matplotlib version is " + matplotlib.__version__)
        logging.info("matplotlib path is " + inspect.getfile(matplotlib))
        from matplotlib import pylab
        logging.info("pylab path is " + inspect.getfile(pylab))
        logging.info("===============================")
        modules = [
            "numpy", "PIL", "Image", "matplotlib", "pylab", "scipy.stats",
            "scipy.spatial"
        ]
        for m in modules:
            try:
                if m == "Image":
                    # Image modules are now imported from PIL
                    exec ("from PIL import Image")
                elif m == "pylab":
                    exec ("from matplotlib import pylab")
                else:
                    exec ("import " + m)
            except ImportError as e:
                asserts.fail("Cannot import python module %s: %s" % (m,
                                                                     str(e)))

        # Add ITS module path to path
        its_path = os.path.join(self.its_path, "pymodules")
        env_python_path = os.environ["PYTHONPATH"]
        self.pythonpath = env_python_path if its_path in env_python_path else \
                "%s:%s" % (its_path, env_python_path)
        os.environ["PYTHONPATH"] = self.pythonpath
        logging.info("new PYTHONPATH: %s", self.pythonpath)

    def RunTestcase(self, testpath):
        """Runs the given testcase and asserts the result.

        Args:
            testpath: string, format tests/[scenename]/[testname].py
        """
        testname = re.split("/|\.", testpath)[-2]
        cmd = [
            'python', os.path.join(self.its_path, testpath), self.device_arg,
            self.display_device_arg
        ]
        outdir = self.out_path
        outpath = os.path.join(outdir, testname + "_stdout.txt")
        errpath = os.path.join(outdir, testname + "_stderr.txt")
        logging.info("cwd: %s", os.getcwd())
        logging.info("cmd: %s", cmd)
        logging.info("outpath: %s", outpath)
        logging.info("errpath: %s", errpath)
        with open(outpath, "w") as fout, open(errpath, "w") as ferr:
            retcode = subprocess.call(
                cmd, stderr=ferr, stdout=fout, cwd=outdir)
        if retcode != 0 and retcode != 101:
            # Dump all logs to host log if the test failed
            with open(outpath, "r") as fout, open(errpath, "r") as ferr:
                logging.info(fout.read())
                logging.error(ferr.read())

        asserts.assertTrue(retcode == 0 or retcode == 101,
                           "ITS %s retcode %d" % (testname, retcode))

    def FetchTestPaths(self, scene):
        """Returns a list of test paths for a given test scene.

        Args:
            scnee: one of ITS test scene name.
        """
        its_path = self.its_path
        paths = [
            os.path.join("tests", scene, s)
            for s in os.listdir(os.path.join(its_path, "tests", scene))
            if s[-3:] == ".py" and s[:4] == "test"
        ]
        paths.sort()
        return paths

    def generateAllTestCases(self):
        # Unused packages:
        # * No plan to support in the near future
        #   - "dng_noise_model"
        #   - "inprog"
        #   - "rolling_shutter_skew"
        # * Not for ITS box (but for diffuser plate)
        #   - "scene5"
        # * Not for ITS box (but for sensor_fusion rotation rig, high-end only)
        #   - "sensor_fusion"
        # * Add those scenes to the below list after scene swapping is ported:
        #     "scene1"
        #     "scene2"
        #     "scene3"
        #     "scene4"
        for test_package_name in ["scene0"]:
            testpaths = self.FetchTestPaths(test_package_name)
            self.runGeneratedTests(
                test_func=self.RunTestcase,
                settings=testpaths,
                name_func=lambda path: "%s_%s" % (re.split("/|\.", path)[-3], re.split("/|\.", path)[-2]))


if __name__ == "__main__":
    test_runner.main()
