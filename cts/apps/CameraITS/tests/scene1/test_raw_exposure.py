# Copyright 2018 The Android Open Source Project
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

import its.device
import its.caps
import its.objects
import its.image
import os.path
import numpy as np
from matplotlib import pylab
import matplotlib.pyplot

IMG_STATS_GRID = 9  # find used to find the center 11.11%
NAME = os.path.basename(__file__).split(".")[0]
NUM_ISO_STEPS = 5
SATURATION_TOL = 0.01
BLK_LVL_TOL = 0.1
# Test 3 steps per 2x exposure
EXP_MULT = pow(2, 1.0/3)
INCREASING_THR = 0.99
# slice captures into burst of SLICE_LEN requests
SLICE_LEN = 10

def main():
    """Capture a set of raw images with increasing exposure time and measure the pixel values.
    """

    with its.device.ItsSession() as cam:

        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.raw16(props) and
                             its.caps.manual_sensor(props) and
                             its.caps.read_3a(props) and
                             its.caps.per_frame_control(props) and
                             not its.caps.mono_camera(props))
        debug = its.caps.debug_mode()

        # Expose for the scene with min sensitivity
        exp_min, exp_max = props["android.sensor.info.exposureTimeRange"]
        sens_min, _ = props["android.sensor.info.sensitivityRange"]
        # Digital gains might not be visible on RAW data
        sens_max = props["android.sensor.maxAnalogSensitivity"]
        sens_step = (sens_max - sens_min) / NUM_ISO_STEPS
        white_level = float(props["android.sensor.info.whiteLevel"])
        black_levels = [its.image.get_black_level(i,props) for i in range(4)]
        # Get the active array width and height.
        aax = props["android.sensor.info.activeArraySize"]["left"]
        aay = props["android.sensor.info.activeArraySize"]["top"]
        aaw = props["android.sensor.info.activeArraySize"]["right"]-aax
        aah = props["android.sensor.info.activeArraySize"]["bottom"]-aay
        raw_stat_fmt = {"format": "rawStats",
                        "gridWidth": aaw/IMG_STATS_GRID,
                        "gridHeight": aah/IMG_STATS_GRID}

        e_test = []
        mult = 1.0
        while exp_min*mult < exp_max:
            e_test.append(int(exp_min*mult))
            mult *= EXP_MULT
        if e_test[-1] < exp_max * INCREASING_THR:
            e_test.append(int(exp_max))
        e_test_ms = [e / 1000000.0 for e in e_test]

        for s in range(sens_min, sens_max, sens_step):
            means = []
            means.append(black_levels)
            reqs = [its.objects.manual_capture_request(s, e, 0) for e in e_test]
            # Capture raw in debug mode, rawStats otherwise
            caps = []
            for i in range(len(reqs) / SLICE_LEN):
                if debug:
                    caps += cam.do_capture(reqs[i*SLICE_LEN:(i+1)*SLICE_LEN], cam.CAP_RAW)
                else:
                    caps += cam.do_capture(reqs[i*SLICE_LEN:(i+1)*SLICE_LEN], raw_stat_fmt)
            last_n = len(reqs) % SLICE_LEN
            if last_n == 1:
                if debug:
                    caps += [cam.do_capture(reqs[-last_n:], cam.CAP_RAW)]
                else:
                    caps += [cam.do_capture(reqs[-last_n:], raw_stat_fmt)]
            elif last_n > 0:
                if debug:
                    caps += cam.do_capture(reqs[-last_n:], cam.CAP_RAW)
                else:
                    caps += cam.do_capture(reqs[-last_n:], raw_stat_fmt)

            # Measure the mean of each channel.
            # Each shot should be brighter (except underexposed/overexposed scene)
            for i,cap in enumerate(caps):
                if debug:
                    planes = its.image.convert_capture_to_planes(cap, props)
                    tiles = [its.image.get_image_patch(p, 0.445, 0.445, 0.11, 0.11) for p in planes]
                    mean = [m * white_level for tile in tiles
                            for m in its.image.compute_image_means(tile)]
                    img = its.image.convert_capture_to_rgb_image(cap, props=props)
                    its.image.write_image(img, "%s_s=%d_e=%05d.jpg" % (NAME, s, e_test))
                else:
                    mean_image, _ = its.image.unpack_rawstats_capture(cap)
                    mean = mean_image[IMG_STATS_GRID/2, IMG_STATS_GRID/2]

                print "ISO=%d, exposure time=%.3fms, mean=%s" % (
                        s, e_test[i] / 1000000.0, str(mean))
                means.append(mean)


            # means[0] is black level value
            r = [m[0] for m in means[1:]]
            gr = [m[1] for m in means[1:]]
            gb = [m[2] for m in means[1:]]
            b = [m[3] for m in means[1:]]

            pylab.plot(e_test_ms, r, "r.-")
            pylab.plot(e_test_ms, b, "b.-")
            pylab.plot(e_test_ms, gr, "g.-")
            pylab.plot(e_test_ms, gb, "k.-")
            pylab.xscale('log')
            pylab.yscale('log')
            pylab.title("%s ISO=%d" % (NAME, s))
            pylab.xlabel("Exposure time (ms)")
            pylab.ylabel("Center patch pixel mean")
            matplotlib.pyplot.savefig("%s_s=%d.png" % (NAME, s))
            pylab.clf()

            allow_under_saturated = True
            for i in xrange(1, len(means)):
                prev_mean = means[i-1]
                mean = means[i]

                if np.isclose(max(mean), white_level, rtol=SATURATION_TOL):
                    print "Saturated: white_level %f, max_mean %f"% (white_level, max(mean))
                    break;

                if allow_under_saturated and np.allclose(mean, black_levels, rtol=BLK_LVL_TOL):
                    # All channel means are close to black level
                    continue

                allow_under_saturated = False
                # Check pixel means are increasing (with small tolerance)
                channels = ["Red", "Gr", "Gb", "Blue"]
                for chan in range(4):
                    err_msg = "ISO=%d, %s, exptime %3fms mean: %.2f, %s mean: %.2f, TOL=%.f%%" % (
                            s, channels[chan],
                            e_test_ms[i-1], mean[chan],
                            "black level" if i == 1 else "exptime %3fms"%e_test_ms[i-2],
                            prev_mean[chan],
                            INCREASING_THR*100)
                    assert mean[chan] > prev_mean[chan] * INCREASING_THR, err_msg

if __name__ == "__main__":
    main()
