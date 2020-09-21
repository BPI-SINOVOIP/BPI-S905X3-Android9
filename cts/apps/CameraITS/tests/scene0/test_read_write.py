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

NAME = os.path.basename(__file__).split('.')[0]
RTOL_EXP_GAIN = 0.97
TEST_EXP_RANGE = [1E6, 1E9]  # ns [1ms, 1s]


def main():
    """Test that the device will write/read correct exp/gain values."""

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.manual_sensor(props) and
                             its.caps.per_frame_control(props))

        # determine capture format
        debug = its.caps.debug_mode()
        largest_yuv = its.objects.get_largest_yuv_format(props)
        if debug:
            fmt = largest_yuv
        else:
            match_ar = (largest_yuv['width'], largest_yuv['height'])
            fmt = its.objects.get_smallest_yuv_format(props, match_ar=match_ar)

        # grab exp/gain ranges from camera
        sensor_exp_range = props['android.sensor.info.exposureTimeRange']
        sens_range = props['android.sensor.info.sensitivityRange']
        print 'sensor e range:', sensor_exp_range
        print 'sensor s range:', sens_range

        # determine if exposure test range is within sensor reported range
        exp_range = []
        if sensor_exp_range[0] < TEST_EXP_RANGE[0]:
            exp_range.append(TEST_EXP_RANGE[0])
        else:
            exp_range.append(sensor_exp_range[0])
        if sensor_exp_range[1] > TEST_EXP_RANGE[1]:
            exp_range.append(TEST_EXP_RANGE[1])
        else:
            exp_range.append(sensor_exp_range[1])

        # build requests
        reqs = []
        index_list = []
        for exp in exp_range:
            for sens in sens_range:
                reqs.append(its.objects.manual_capture_request(sens, exp))
                index_list.append((exp, sens))

        # take shots
        caps = cam.do_capture(reqs, fmt)

        # extract exp/sensitivity data
        data = {}
        for i, cap in enumerate(caps):
            e_read = cap['metadata']['android.sensor.exposureTime']
            s_read = cap['metadata']['android.sensor.sensitivity']
            data[index_list[i]] = (e_read, s_read)

        # check read/write match across all shots
        e_failed = []
        s_failed = []
        for e_write in exp_range:
            for s_write in sens_range:
                (e_read, s_read) = data[(e_write, s_write)]
                if e_write < e_read or e_read/float(e_write) <= RTOL_EXP_GAIN:
                    e_failed.append({'e_write': e_write,
                                     'e_read': e_read,
                                     's_write': s_write,
                                     's_read': s_read})
                if s_write < s_read or s_read/float(s_write) <= RTOL_EXP_GAIN:
                    s_failed.append({'e_write': e_write,
                                     'e_read': e_read,
                                     's_write': s_write,
                                     's_read': s_read})

        # print results
        if e_failed:
            print '\nFAILs for exposure time'
            for fail in e_failed:
                print ' e_write: %d, e_read: %d, RTOL: %.2f, ' % (
                        fail['e_write'], fail['e_read'], RTOL_EXP_GAIN),
                print 's_write: %d, s_read: %d, RTOL: %.2f' % (
                        fail['s_write'], fail['s_read'], RTOL_EXP_GAIN)
        if s_failed:
            print 'FAILs for sensitivity(ISO)'
            for fail in s_failed:
                print 's_write: %d, s_read: %d, RTOL: %.2f, ' % (
                        fail['s_write'], fail['s_read'], RTOL_EXP_GAIN),
                print ' e_write: %d, e_read: %d, RTOL: %.2f' % (
                        fail['e_write'], fail['e_read'], RTOL_EXP_GAIN)

        # assert PASS/FAIL
        assert not e_failed+s_failed


if __name__ == '__main__':
    main()
