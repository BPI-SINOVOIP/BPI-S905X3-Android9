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

import os.path

import its.caps
import its.device
import its.image
import its.objects
import its.target

import numpy as np
NAME = os.path.basename(__file__).split('.')[0]
PATCH_SIZE = 0.0625  # 1/16 x 1/16 in center of image
PATCH_LOC = (1-PATCH_SIZE)/2
THRESH_DIFF = 0.06
THRESH_GAIN = 0.1
THRESH_EXP = 0.05


def main():
    """Test both cameras give similar RBG values for gray patch."""

    yuv_sizes = {}
    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.compute_target_exposure(props) and
                             its.caps.per_frame_control(props) and
                             its.caps.logical_multi_camera(props) and
                             its.caps.raw16(props) and
                             its.caps.manual_sensor(props))
        ids = its.caps.logical_multi_camera_physical_ids(props)
        s, e, _, _, f = cam.do_3a(get_results=True)
        req = its.objects.manual_capture_request(s, e, f)
        max_raw_size = its.objects.get_available_output_sizes('raw', props)[0]
        for i in ids:
            physical_props = cam.get_camera_properties_by_id(i)
            yuv_sizes[i] = its.objects.get_available_output_sizes(
                    'yuv', physical_props, match_ar_size=max_raw_size)
            if i == ids[0]:
                yuv_match_sizes = yuv_sizes[i]
            else:
                list(set(yuv_sizes[i]).intersection(yuv_match_sizes))
        yuv_match_sizes.sort()
        w = yuv_match_sizes[-1][0]
        h = yuv_match_sizes[-1][1]
        print 'RAW size: (%d, %d)' % (max_raw_size[0], max_raw_size[1])
        print 'YUV size: (%d, %d)' % (w, h)

        # capture YUVs
        out_surfaces = [{'format': 'raw'},
                        {'format': 'yuv', 'width': w, 'height': h,
                         'physicalCamera': ids[0]},
                        {'format': 'yuv', 'width': w, 'height': h,
                         'physicalCamera': ids[1]}]
        cap_raw, cap_yuv1, cap_yuv2 = cam.do_capture(req, out_surfaces)

        img_raw = its.image.convert_capture_to_rgb_image(cap_raw, props=props)
        its.image.write_image(img_raw, '%s_raw.jpg' % NAME)
        rgb_means_raw = its.image.compute_image_means(
                its.image.get_image_patch(img_raw, PATCH_LOC, PATCH_LOC,
                                          PATCH_SIZE, PATCH_SIZE))

        img_yuv1 = its.image.convert_capture_to_rgb_image(
                cap_yuv1, props=props)
        its.image.write_image(img_yuv1, '%s_yuv1.jpg' % NAME)
        y1, _, _ = its.image.convert_capture_to_planes(
                cap_yuv1, props=props)
        y1_mean = its.image.compute_image_means(
                its.image.get_image_patch(y1, PATCH_LOC, PATCH_LOC,
                                          PATCH_SIZE, PATCH_SIZE))[0]

        img_yuv2 = its.image.convert_capture_to_rgb_image(
                cap_yuv2, props=props)
        its.image.write_image(img_yuv2, '%s_yuv2.jpg' % NAME)
        y2, _, _ = its.image.convert_capture_to_planes(
                cap_yuv2, props=props)
        y2_mean = its.image.compute_image_means(
                its.image.get_image_patch(y2, PATCH_LOC, PATCH_LOC,
                                          PATCH_SIZE, PATCH_SIZE))[0]
        print 'rgb_raw:', rgb_means_raw
        print 'y1_mean:', y1_mean
        print 'y2_mean:', y2_mean

        # assert gain/exp values are near written values
        s_yuv1 = cap_yuv1['metadata']['android.sensor.sensitivity']
        e_yuv1 = cap_yuv1['metadata']['android.sensor.exposureTime']
        msg = 'yuv_gain(write): %d, (read): %d' % (s, s_yuv1)
        assert 0 <= s - s_yuv1 < s * THRESH_GAIN, msg
        msg = 'yuv_exp(write): %.3fms, (read): %.3fms' % (e*1E6, e_yuv1*1E6)
        assert 0 <= e - e_yuv1 < e * THRESH_EXP, msg

        # compare YUVs
        msg = 'y1: %.3f, y2: %.3f, TOL=%.5f' % (y1_mean, y2_mean, THRESH_DIFF)
        assert np.isclose(y1_mean, y2_mean, rtol=THRESH_DIFF), msg


if __name__ == '__main__':
    main()
