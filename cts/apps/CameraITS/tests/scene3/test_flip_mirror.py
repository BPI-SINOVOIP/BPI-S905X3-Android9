# Copyright 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import cv2

import its.caps
import its.cv2image
import its.device
import its.image
import its.objects
import numpy as np

NAME = os.path.basename(__file__).split('.')[0]
CHART_FILE = os.path.join(os.environ['CAMERA_ITS_TOP'], 'pymodules', 'its',
                          'test_images', 'ISO12233.png')
CHART_HEIGHT = 13.5  # cm
CHART_DISTANCE = 30.0  # cm
CHART_SCALE_START = 0.65
CHART_SCALE_STOP = 1.35
CHART_SCALE_STEP = 0.025
CHART_ORIENTATIONS = ['nominal', 'flip', 'mirror', 'rotate']
VGA_WIDTH = 640
VGA_HEIGHT = 480
(X_CROP, Y_CROP) = (0.5, 0.5)  # crop center area of ISO12233 chart


def test_flip_mirror(cam, props, fmt, chart):
    """Return if image is flipped or mirrored.

    Args:
        cam (class): An open device session
        props (class): Properties of cam
        fmt (dict): Capture format
        chart (class): Object with chart properties

    Returns:
        boolean: True if flipped, False if not
    """

    # determine if monochrome camera
    mono_camera = its.caps.mono_camera(props)

    # determine if in debug mode
    debug = its.caps.debug_mode()

    # get a local copy of the chart template
    template = cv2.imread(CHART_FILE, cv2.IMREAD_ANYDEPTH)

    # take img, crop chart, scale and prep for cv2 template match
    s, e, _, _, fd = cam.do_3a(get_results=True, mono_camera=mono_camera)
    req = its.objects.manual_capture_request(s, e, fd)
    cap = cam.do_capture(req, fmt)
    y, _, _ = its.image.convert_capture_to_planes(cap, props)
    y = its.image.rotate_img_per_argv(y)
    patch = its.image.get_image_patch(y, chart.xnorm, chart.ynorm,
                                      chart.wnorm, chart.hnorm)
    patch = 255 * its.cv2image.gray_scale_img(patch)
    patch = its.cv2image.scale_img(patch.astype(np.uint8), chart.scale)

    # sanity check on image
    assert np.max(patch)-np.min(patch) > 255/8

    # save full images if in debug
    if debug:
        its.image.write_image(template[:, :, np.newaxis]/255.0,
                              '%s_template.jpg' % NAME)

    # save patch
    its.image.write_image(patch[:, :, np.newaxis]/255.0,
                          '%s_scene_patch.jpg' % NAME)

    # crop center areas and strip off any extra rows/columns
    template = its.image.get_image_patch(template, (1-X_CROP)/2, (1-Y_CROP)/2,
                                         X_CROP, Y_CROP)
    patch = its.image.get_image_patch(patch, (1-X_CROP)/2,
                                      (1-Y_CROP)/2, X_CROP, Y_CROP)
    patch = patch[0:min(patch.shape[0], template.shape[0]),
                  0:min(patch.shape[1], template.shape[1])]
    comp_chart = patch

    # determine optimum orientation
    opts = []
    for orientation in CHART_ORIENTATIONS:
        if orientation == 'flip':
            comp_chart = np.flipud(patch)
        elif orientation == 'mirror':
            comp_chart = np.fliplr(patch)
        elif orientation == 'rotate':
            comp_chart = np.flipud(np.fliplr(patch))
        correlation = cv2.matchTemplate(comp_chart, template, cv2.TM_CCOEFF)
        _, opt_val, _, _ = cv2.minMaxLoc(correlation)
        if debug:
            cv2.imwrite('%s_%s.jpg' % (NAME, orientation), comp_chart)
        print ' %s correlation value: %d' % (orientation, opt_val)
        opts.append(opt_val)

    # determine if 'nominal' or 'rotated' is best orientation
    assert_flag = (opts[0] == max(opts) or opts[3] == max(opts))
    assert assert_flag, ('Optimum orientation is %s' %
                         CHART_ORIENTATIONS[np.argmax(opts)])
    # print warning if rotated
    if opts[3] == max(opts):
        print 'Image is rotated 180 degrees. Try "rotate" flag.'


def main():
    """Test if image is properly oriented."""

    print '\nStarting test_flip_mirror.py'

    # check skip conditions
    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.read_3a(props))
    # initialize chart class and locate chart in scene
    chart = its.cv2image.Chart(CHART_FILE, CHART_HEIGHT, CHART_DISTANCE,
                               CHART_SCALE_START, CHART_SCALE_STOP,
                               CHART_SCALE_STEP)

    with its.device.ItsSession() as cam:
        fmt = {'format': 'yuv', 'width': VGA_WIDTH, 'height': VGA_HEIGHT}

        # test that image is not flipped, mirrored, or rotated
        test_flip_mirror(cam, props, fmt, chart)


if __name__ == '__main__':
    main()
