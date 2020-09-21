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

import math
import os.path
import re
import sys
import cv2

import its.caps
import its.device
import its.image
import its.objects

import numpy as np

ALIGN_TOL_PERCENT = 1
CHART_DISTANCE_CM = 22  # cm
CIRCLE_TOL_PERCENT = 10
NAME = os.path.basename(__file__).split('.')[0]
ROTATE_REF_MATRIX = np.array([0, 0, 0, 1])
TRANS_REF_MATRIX = np.array([0, 0, 0])


def rotation_matrix(rotation):
    """Convert the rotation parameters to 3-axis data.

    Args:
        rotation:   android.lens.Rotation vector
    Returns:
        3x3 matrix w/ rotation parameters
    """
    x = rotation[0]
    y = rotation[1]
    z = rotation[2]
    w = rotation[3]
    return np.array([[1-2*y**2-2*z**2, 2*x*y-2*z*w, 2*x*z+2*y*w],
                     [2*x*y+2*z*w, 1-2*x**2-2*z**2, 2*y*z-2*x*w],
                     [2*x*z-2*y*w, 2*y*z+2*x*w, 1-2*x**2-2*y**2]])


def find_circle(gray, name):
    """Find the circle in the image.

    Args:
        gray:           gray scale image array [0,255]
        name:           string of file name
    Returns:
        circle:         (circle_center_x, circle_center_y, radius)
    """

    cv2_version = cv2.__version__
    try:
        if cv2_version.startswith('2.4.'):
            circle = cv2.HoughCircles(gray, cv2.cv.CV_HOUGH_GRADIENT,
                                      1, 20)[0][0]
        elif cv2_version.startswith('3.2.'):
            circle = cv2.HoughCircles(gray, cv2.HOUGH_GRADIENT,
                                      1, 20)[0][0]
    except TypeError:
        circle = None
        its.image.write_image(gray[..., np.newaxis]/255.0, name)
    assert circle is not None, 'No circle found!'
    return circle


def main():
    """Test the multi camera system parameters related to camera spacing."""
    chart_distance = CHART_DISTANCE_CM
    for s in sys.argv[1:]:
        if s[:5] == 'dist=' and len(s) > 5:
            chart_distance = float(re.sub('cm', '', s[5:]))
            print 'Using chart distance: %.1fcm' % chart_distance

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.compute_target_exposure(props) and
                             its.caps.per_frame_control(props) and
                             its.caps.logical_multi_camera(props) and
                             its.caps.raw16(props) and
                             its.caps.manual_sensor(props))
        debug = its.caps.debug_mode()
        avail_fls = props['android.lens.info.availableFocalLengths']

        max_raw_size = its.objects.get_available_output_sizes('raw', props)[0]
        w, h = its.objects.get_available_output_sizes(
                'yuv', props, match_ar_size=max_raw_size)[0]

        # Do 3A and get the values
        s, e, _, _, fd = cam.do_3a(get_results=True,
                                   lock_ae=True, lock_awb=True)
        req = its.objects.manual_capture_request(s, e, fd, True, props)

        # get physical camera properties
        ids = its.caps.logical_multi_camera_physical_ids(props)
        props_physical = {}
        for i in ids:
            props_physical[i] = cam.get_camera_properties_by_id(i)

        # capture RAWs of 1st 2 cameras
        cap_raw = {}
        out_surfaces = [{'format': 'yuv', 'width': w, 'height': h},
                        {'format': 'raw', 'physicalCamera': ids[0]},
                        {'format': 'raw', 'physicalCamera': ids[1]}]
        _, cap_raw[ids[0]], cap_raw[ids[1]] = cam.do_capture(req, out_surfaces)

    size_raw = {}
    k = {}
    reference = {}
    rotation = {}
    trans = {}
    circle = {}
    fl = {}
    sensor_diag = {}
    point = {}
    for i in ids:
        print 'Starting camera %s' % i
        # process image
        img_raw = its.image.convert_capture_to_rgb_image(
                cap_raw[i], props=props)
        size_raw[i] = (cap_raw[i]['width'], cap_raw[i]['height'])

        # save images if debug
        if debug:
            its.image.write_image(img_raw, '%s_raw_%s.jpg' % (NAME, i))

        # convert to [0, 255] images
        img_raw *= 255

        # scale to match calibration data
        img = cv2.resize(img_raw.astype(np.uint8), None, fx=2, fy=2)

        # load parameters for each physical camera
        ical = props_physical[i]['android.lens.intrinsicCalibration']
        assert len(ical) == 5, 'android.lens.instrisicCalibration incorrect.'
        k[i] = np.array([[ical[0], ical[4], ical[2]],
                         [0, ical[1], ical[3]],
                         [0, 0, 1]])
        print ' k:', k[i]

        rotation[i] = np.array(props_physical[i]['android.lens.poseRotation'])
        print ' rotation:', rotation[i]
        assert len(rotation[i]) == 4, 'poseRotation has wrong # of params.'
        trans[i] = np.array(
                props_physical[i]['android.lens.poseTranslation'])
        print ' translation:', trans[i]
        assert len(trans[i]) == 3, 'poseTranslation has wrong # of params.'
        if ((rotation[i] == ROTATE_REF_MATRIX).all() and
                    (trans[i] == TRANS_REF_MATRIX).all()):
            reference[i] = True
        else:
            reference[i] = False

        # Apply correction to image (if available)
        if its.caps.distortion_correction(props):
            distort = np.array(props_physical[i]['android.lens.distortion'])
            assert len(distort) == 5, 'radialDistortion has wrong # of params.'
            cv2_distort = np.array([distort[0], distort[1],
                                    distort[3], distort[4],
                                    distort[2]])
            img = cv2.undistort(img, k[i], cv2_distort)
            its.image.write_image(img/255.0, '%s_correct_%s.jpg' % (
                    NAME, i))

        # Find the circles in grayscale image
        circle[i] = find_circle(cv2.cvtColor(img, cv2.COLOR_BGR2GRAY),
                                '%s_gray%s.jpg' % (NAME, i))

        # Find focal length & sensor size
        fl[i] = props_physical[i]['android.lens.info.availableFocalLengths'][0]
        sensor_diag[i] = math.sqrt(size_raw[i][0] ** 2 + size_raw[i][1] ** 2)

        # Find 3D location of circle centers
        point[i] = np.dot(np.linalg.inv(k[i]),
                          np.array([circle[i][0],
                                    circle[i][1], 1])) * chart_distance * 1.0E-2

    ref_index = (e for e in reference if e).next()
    print 'reference camera id:', ref_index
    ref_rotation = rotation[ref_index]
    ref_rotation = ref_rotation.astype(np.float32)
    print 'rotation reference:', ref_rotation
    r = rotation_matrix(ref_rotation)
    if debug:
        print 'r:', r
    t = -1 * trans[ref_index]
    print 't:', t

    # Estimate ids[0] circle center from ids[1] & params
    estimated_0 = cv2.projectPoints(point[ids[1]].reshape(1, 3),
                                    r, t, k[ids[0]], None)[0][0][0]
    err_0 = np.linalg.norm(estimated_0 - circle[ids[0]][:2])
    print 'Circle centers [%s]' % ids[0]
    print 'Measured:      %.1f, %.1f' % (circle[ids[0]][1], circle[ids[0]][0])
    print 'Calculated:    %.1f, %.1f' % (estimated_0[1],
                                         estimated_0[0])
    print 'Error(pixels): %.1f' % err_0

    # Estimate ids[0] circle center from ids[1] & params
    estimated_1 = cv2.projectPoints(point[ids[0]].reshape(1, 3),
                                    r.T, -np.dot(r, t), k[ids[1]],
                                    None)[0][0][0]
    err_1 = np.linalg.norm(estimated_1 - circle[ids[1]][:2])
    print 'Circle centers [%s]' % ids[1]
    print 'Measured:      %.1f, %.1f' % (circle[ids[1]][1], circle[ids[1]][0])
    print 'Calculated:    %.1f, %.1f' % (estimated_1[1], estimated_1[0])
    print 'Error(pixels): %.1f' % err_1

    err_0 /= math.sqrt(size_raw[ids[0]][0]**2 + size_raw[ids[0]][1]**2)
    err_1 /= math.sqrt(size_raw[ids[1]][0]**2 + size_raw[ids[1]][1]**2)
    msg = '%s -> %s center error too large! val=%.1f%%, THRESH=%.f%%' % (
            ids[1], ids[0], err_0*100, ALIGN_TOL_PERCENT)
    assert err_0*100 < ALIGN_TOL_PERCENT, msg
    msg = '%s -> %s center error too large! val=%.1f%%, THRESH=%.f%%' % (
            ids[0], ids[1], err_1*100, ALIGN_TOL_PERCENT)
    assert err_1*100 < ALIGN_TOL_PERCENT, msg

    # Check focal length and circle size if more than 1 focal length
    if len(avail_fls) > 1:
        print 'circle_0: %.2f, circle_1: %.2f' % (
                circle[ids[0]][2], circle[ids[1]][2])
        print 'fl_0: %.2f, fl_1: %.2f' % (fl[ids[0]], fl[ids[1]])
        print 'diag_0: %.2f, diag_1: %.2f' % (
                sensor_diag[ids[0]], sensor_diag[ids[1]])
        msg = 'Circle size does not scale properly.'
        assert np.isclose(circle[ids[0]][2]/fl[ids[0]]*sensor_diag[ids[0]],
                          circle[ids[1]][2]/fl[ids[1]]*sensor_diag[ids[1]],
                          rtol=CIRCLE_TOL_PERCENT/100.0), msg


if __name__ == '__main__':
    main()
