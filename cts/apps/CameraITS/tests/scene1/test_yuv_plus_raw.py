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

import math
import os.path
import its.caps
import its.device
import its.image
import its.objects
import its.target

NAME = os.path.basename(__file__).split(".")[0]
THRESHOLD_MAX_RMS_DIFF = 0.035


def main():
    """Test capturing a single frame as both RAW and YUV outputs."""

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.compute_target_exposure(props) and
                             its.caps.raw16(props) and
                             its.caps.per_frame_control(props) and
                             not its.caps.mono_camera(props))

        # Use a manual request with a linear tonemap so that the YUV and RAW
        # should look the same (once converted by the its.image module).
        e, s = its.target.get_target_exposure_combos(cam)["midExposureTime"]
        req = its.objects.manual_capture_request(s, e, 0.0, True, props)

        mode = req["android.shading.mode"]
        print "shading mode:", mode

        max_raw_size = its.objects.get_available_output_sizes("raw", props)[0]
        w, h = its.objects.get_available_output_sizes(
                "yuv", props, (1920, 1080), max_raw_size)[0]
        out_surfaces = [{"format": "raw"},
                        {"format": "yuv", "width": w, "height": h}]
        cap_raw, cap_yuv = cam.do_capture(req, out_surfaces)

        img = its.image.convert_capture_to_rgb_image(cap_yuv)
        its.image.write_image(img, "%s_shading=%d_yuv.jpg" % (NAME, mode), True)
        tile = its.image.get_image_patch(img, 0.45, 0.45, 0.1, 0.1)
        rgb0 = its.image.compute_image_means(tile)

        # Raw shots are 1/2 x 1/2 smaller after conversion to RGB, but tile
        # cropping is relative.
        img = its.image.convert_capture_to_rgb_image(cap_raw, props=props)
        its.image.write_image(img, "%s_shading=%d_raw.jpg" % (NAME, mode), True)
        tile = its.image.get_image_patch(img, 0.45, 0.45, 0.1, 0.1)
        rgb1 = its.image.compute_image_means(tile)

        rms_diff = math.sqrt(
                sum([pow(rgb0[i] - rgb1[i], 2.0) for i in range(3)]) / 3.0)
        msg = "RMS difference: %.4f, spec: %.3f" % (rms_diff,
                                                    THRESHOLD_MAX_RMS_DIFF)
        print msg
        assert rms_diff < THRESHOLD_MAX_RMS_DIFF, msg

if __name__ == "__main__":
    main()

