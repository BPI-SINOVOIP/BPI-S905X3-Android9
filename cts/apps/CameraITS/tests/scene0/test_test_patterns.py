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

import os

import its.caps
import its.device
import its.image
import its.objects
import numpy as np

NAME = os.path.basename(__file__).split('.')[0]
PATTERNS = [1, 2]
COLOR_BAR_ORDER = ['WHITE', 'YELLOW', 'CYAN', 'GREEN', 'MAGENTA', 'RED',
                   'BLUE', 'BLACK']
COLOR_CHECKER = {'BLACK': [0, 0, 0], 'RED': [1, 0, 0], 'GREEN': [0, 1, 0],
                 'BLUE': [0, 0, 1], 'MAGENTA': [1, 0, 1], 'CYAN': [0, 1, 1],
                 'YELLOW': [1, 1, 0], 'WHITE': [1, 1, 1]}
CH_TOL = 2E-3  # 1/2 DN in [0:1]
LSFR_COEFFS = 0b100010000  # PN9


def check_solid_color(cap, props):
    """Simple test for solid color.

    Args:
        cap: capture element
        props: capture properties
    Returns:
        True/False
    """
    print 'Checking solid TestPattern...'
    r, gr, gb, b = its.image.convert_capture_to_planes(cap, props)
    r_tile = its.image.get_image_patch(r, 0.0, 0.0, 1.0, 1.0)
    gr_tile = its.image.get_image_patch(gr, 0.0, 0.0, 1.0, 1.0)
    gb_tile = its.image.get_image_patch(gb, 0.0, 0.0, 1.0, 1.0)
    b_tile = its.image.get_image_patch(b, 0.0, 0.0, 1.0, 1.0)
    var_max = max(np.amax(r_tile), np.amax(gr_tile), np.amax(gb_tile),
                  np.amax(b_tile))
    var_min = min(np.amin(r_tile), np.amin(gr_tile), np.amin(gb_tile),
                  np.amin(b_tile))
    white_level = int(props['android.sensor.info.whiteLevel'])
    print ' pixel min: %.f, pixel max: %.f' % (white_level*var_min,
                                               white_level*var_max)
    return np.isclose(var_max, var_min, atol=CH_TOL)


def check_color_bars(cap, props, mirror=False):
    """Test image for color bars.

    Compute avg of bars and compare to ideal

    Args:
        cap:            capture element
        props:          capture properties
        mirror (bool):  whether to mirror image or not
    Returns:
        True/False
    """
    print 'Checking color bar TestPattern...'
    delta = 0.0005
    num_bars = len(COLOR_BAR_ORDER)
    color_match = []
    img = its.image.convert_capture_to_rgb_image(cap, props=props)
    if mirror:
        print ' Image mirrored'
        img = np.fliplr(img)
    for i, color in enumerate(COLOR_BAR_ORDER):
        tile = its.image.get_image_patch(img, float(i)/num_bars+delta,
                                         0.0, 1.0/num_bars-2*delta, 1.0)
        color_match.append(np.allclose(its.image.compute_image_means(tile),
                                       COLOR_CHECKER[color], atol=CH_TOL))
    print COLOR_BAR_ORDER
    print color_match
    return all(color_match)


def check_pattern(cap, props, pattern):
    """Simple tests for pattern correctness.

    Args:
        cap: capture element
        props: capture properties
        pattern (int): valid number for pattern
    Returns:
        boolean
    """

    # white_level = int(props['android.sensor.info.whiteLevel'])
    if pattern == 1:  # solid color
        return check_solid_color(cap, props)

    elif pattern == 2:  # color bars
        striped = check_color_bars(cap, props, mirror=False)
        # check mirrored version in case image rotated from sensor orientation
        if not striped:
            striped = check_color_bars(cap, props, mirror=True)
        return striped

    else:
        print 'No specific test for TestPattern %d' % pattern
        return True


def test_test_patterns(cam, props, af_fd):
    """test image sensor test patterns.

    Args:
        cam: An open device session.
        props: Properties of cam
        af_fd: Focus distance
    """

    avail_patterns = props['android.sensor.availableTestPatternModes']
    print 'avail_patterns: ', avail_patterns
    sens_min, _ = props['android.sensor.info.sensitivityRange']
    exposure = min(props['android.sensor.info.exposureTimeRange'])

    for pattern in PATTERNS:
        if pattern in avail_patterns:
            req = its.objects.manual_capture_request(int(sens_min),
                                                     exposure)
            req['android.lens.focusDistance'] = af_fd
            req['android.sensor.testPatternMode'] = pattern
            fmt = {'format': 'raw'}
            cap = cam.do_capture(req, fmt)
            img = its.image.convert_capture_to_rgb_image(cap, props=props)

            # Save pattern
            its.image.write_image(img, '%s_%d.jpg' % (NAME, pattern), True)

            # Check pattern for correctness
            assert check_pattern(cap, props, pattern)
        else:
            print 'Pattern not in android.sensor.availableTestPatternModes.'


def main():
    """Test pattern generation test.

    Test: capture frames for each valid test pattern and check if
    generated correctly.
    android.sensor.testPatternMode
    0: OFF
    1: SOLID_COLOR
    2: COLOR_BARS
    3: COLOR_BARS_FADE_TO_GREY
    4: PN9
    """

    print '\nStarting %s' % NAME
    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.raw16(props) and
                             its.caps.manual_sensor(props) and
                             its.caps.per_frame_control(props))

        # For test pattern, use min_fd
        fd = props['android.lens.info.minimumFocusDistance']
        test_test_patterns(cam, props, fd)

if __name__ == '__main__':
    main()
