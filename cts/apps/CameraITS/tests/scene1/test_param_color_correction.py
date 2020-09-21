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
import its.caps
import its.device
import its.objects
import its.target
from matplotlib import pylab
import os.path
import matplotlib
import matplotlib.pyplot

def main():
    """Test that the android.colorCorrection.* params are applied when set.

    Takes shots with different transform and gains values, and tests that
    they look correspondingly different. The transform and gains are chosen
    to make the output go redder or bluer.

    Uses a linear tonemap.
    """
    NAME = os.path.basename(__file__).split(".")[0]

    THRESHOLD_MAX_DIFF = 0.1

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.compute_target_exposure(props) and
                             its.caps.per_frame_control(props) and
                             not its.caps.mono_camera(props))

        # Baseline request
        debug = its.caps.debug_mode()
        largest_yuv = its.objects.get_largest_yuv_format(props)
        if debug:
            fmt = largest_yuv
        else:
            match_ar = (largest_yuv['width'], largest_yuv['height'])
            fmt = its.objects.get_smallest_yuv_format(props, match_ar=match_ar)

        e, s = its.target.get_target_exposure_combos(cam)["midSensitivity"]
        req = its.objects.manual_capture_request(s, e, 0.0, True, props)
        req["android.colorCorrection.mode"] = 0

        # Transforms:
        # 1. Identity
        # 2. Identity
        # 3. Boost blue
        transforms = [its.objects.int_to_rational([1,0,0, 0,1,0, 0,0,1]),
                      its.objects.int_to_rational([1,0,0, 0,1,0, 0,0,1]),
                      its.objects.int_to_rational([1,0,0, 0,1,0, 0,0,2])]

        # Gains:
        # 1. Unit
        # 2. Boost red
        # 3. Unit
        gains = [[1,1,1,1], [2,1,1,1], [1,1,1,1]]

        r_means = []
        g_means = []
        b_means = []

        # Capture requests:
        # 1. With unit gains, and identity transform.
        # 2. With a higher red gain, and identity transform.
        # 3. With unit gains, and a transform that boosts blue.
        for i in range(len(transforms)):
            req["android.colorCorrection.transform"] = transforms[i]
            req["android.colorCorrection.gains"] = gains[i]
            cap = cam.do_capture(req, fmt)
            img = its.image.convert_capture_to_rgb_image(cap)
            its.image.write_image(img, "%s_req=%d.jpg" % (NAME, i))
            tile = its.image.get_image_patch(img, 0.45, 0.45, 0.1, 0.1)
            rgb_means = its.image.compute_image_means(tile)
            r_means.append(rgb_means[0])
            g_means.append(rgb_means[1])
            b_means.append(rgb_means[2])
            ratios = [rgb_means[0] / rgb_means[1], rgb_means[2] / rgb_means[1]]
            print "Means = ", rgb_means, "   Ratios =", ratios

        # Draw a plot.
        domain = range(len(transforms))
        pylab.plot(domain, r_means, 'r')
        pylab.plot(domain, g_means, 'g')
        pylab.plot(domain, b_means, 'b')
        pylab.ylim([0,1])
        matplotlib.pyplot.savefig("%s_plot_means.png" % (NAME))

        # Expect G0 == G1 == G2, R0 == 0.5*R1 == R2, B0 == B1 == 0.5*B2
        # Also need to ensure that the image is not clamped to white/black.
        assert(all(g_means[i] > 0.2 and g_means[i] < 0.8 for i in xrange(3)))
        assert(abs(g_means[1] - g_means[0]) < THRESHOLD_MAX_DIFF)
        assert(abs(g_means[2] - g_means[1]) < THRESHOLD_MAX_DIFF)
        assert(abs(r_means[2] - r_means[0]) < THRESHOLD_MAX_DIFF)
        assert(abs(r_means[1] - 2.0 * r_means[0]) < THRESHOLD_MAX_DIFF)
        assert(abs(b_means[1] - b_means[0]) < THRESHOLD_MAX_DIFF)
        assert(abs(b_means[2] - 2.0 * b_means[0]) < THRESHOLD_MAX_DIFF)

if __name__ == '__main__':
    main()

