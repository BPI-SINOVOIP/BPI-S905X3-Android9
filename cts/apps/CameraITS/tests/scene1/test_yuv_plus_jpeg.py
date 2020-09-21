# Copyright 2014 The Android Open Source Project
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
import os.path
import math

def main():
    """Test capturing a single frame as both YUV and JPEG outputs.
    """
    NAME = os.path.basename(__file__).split(".")[0]

    THRESHOLD_MAX_RMS_DIFF = 0.01

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.compute_target_exposure(props))

        max_jpeg_size = \
                its.objects.get_available_output_sizes("jpeg", props)[0]
        w,h = its.objects.get_available_output_sizes(
                "yuv", props, (1920, 1080), max_jpeg_size)[0]
        fmt_yuv =  {"format":"yuv", "width":w, "height":h}
        fmt_jpeg = {"format":"jpeg"}

        # Use a manual request with a linear tonemap so that the YUV and JPEG
        # should look the same (once converted by the its.image module).
        e, s = its.target.get_target_exposure_combos(cam)["midExposureTime"]
        req = its.objects.manual_capture_request(s, e, 0.0, True, props)

        cap_yuv, cap_jpeg = cam.do_capture(req, [fmt_yuv, fmt_jpeg])

        img = its.image.convert_capture_to_rgb_image(cap_yuv, True)
        its.image.write_image(img, "%s_yuv.jpg" % (NAME))
        tile = its.image.get_image_patch(img, 0.45, 0.45, 0.1, 0.1)
        rgb0 = its.image.compute_image_means(tile)

        img = its.image.convert_capture_to_rgb_image(cap_jpeg, True)
        its.image.write_image(img, "%s_jpeg.jpg" % (NAME))
        tile = its.image.get_image_patch(img, 0.45, 0.45, 0.1, 0.1)
        rgb1 = its.image.compute_image_means(tile)

        rms_diff = math.sqrt(
                sum([pow(rgb0[i] - rgb1[i], 2.0) for i in range(3)]) / 3.0)
        print "RMS difference:", rms_diff
        assert(rms_diff < THRESHOLD_MAX_RMS_DIFF)

if __name__ == '__main__':
    main()

