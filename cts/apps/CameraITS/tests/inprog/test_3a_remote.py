# Copyright 2013 The Android Open Source Project
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

import its.image
import its.device
import its.objects
import os.path
import pprint
import math
import numpy
import matplotlib.pyplot
import mpl_toolkits.mplot3d

def main():
    """Run 3A remotely (from this script).
    """
    NAME = os.path.basename(__file__).split(".")[0]

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()

        # TODO: Test for 3A convergence, and exit this test once converged.

        triggered = False
        while True:
            req = its.objects.auto_capture_request()
            req["android.statistics.lensShadingMapMode"] = 1
            req['android.control.aePrecaptureTrigger'] = (0 if triggered else 1)
            req['android.control.afTrigger'] = (0 if triggered else 1)
            triggered = True

            cap = cam.do_capture(req)

            ae_state = cap["metadata"]["android.control.aeState"]
            awb_state = cap["metadata"]["android.control.awbState"]
            af_state = cap["metadata"]["android.control.afState"]
            gains = cap["metadata"]["android.colorCorrection.gains"]
            transform = cap["metadata"]["android.colorCorrection.transform"]
            exp_time = cap["metadata"]['android.sensor.exposureTime']
            lsc_obj = cap_res["android.statistics.lensShadingCorrectionMap"]
            lsc_map = lsc_obj["map"]
            w_map = lsc_obj["width"]
            h_map = lsc_obj["height"]
            foc_dist = cap["metadata"]['android.lens.focusDistance']
            foc_range = cap["metadata"]['android.lens.focusRange']

            print "States (AE,AWB,AF):", ae_state, awb_state, af_state
            print "Gains:", gains
            print "Transform:", [its.objects.rational_to_float(t)
                                 for t in transform]
            print "AE region:", cap["metadata"]['android.control.aeRegions']
            print "AF region:", cap["metadata"]['android.control.afRegions']
            print "AWB region:", cap["metadata"]['android.control.awbRegions']
            print "LSC map:", w_map, h_map, lsc_map[:8]
            print "Focus (dist,range):", foc_dist, foc_range
            print ""

if __name__ == '__main__':
    main()

