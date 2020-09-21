# Copyright 2016 The Android Open Source Project
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
import unittest

import cv2
import its.caps
import its.device
import its.error
import its.image
import numpy

VGA_HEIGHT = 480
VGA_WIDTH = 640


def scale_img(img, scale=1.0):
    """Scale and image based on a real number scale factor."""
    dim = (int(img.shape[1]*scale), int(img.shape[0]*scale))
    return cv2.resize(img.copy(), dim, interpolation=cv2.INTER_AREA)


def gray_scale_img(img):
    """Return gray scale version of image."""
    if len(img.shape) == 2:
        img_gray = img.copy()
    elif len(img.shape) == 3:
        if img.shape[2] == 1:
            img_gray = img[:, :, 0].copy()
        else:
            img_gray = cv2.cvtColor(img, cv2.COLOR_RGB2GRAY)
    return img_gray


class Chart(object):
    """Definition for chart object.

    Defines PNG reference file, chart size and distance, and scaling range.
    """

    def __init__(self, chart_file, height, distance, scale_start, scale_stop,
                 scale_step):
        """Initial constructor for class.

        Args:
            chart_file:     str; absolute path to png file of chart
            height:         float; height in cm of displayed chart
            distance:       float; distance in cm from camera of displayed chart
            scale_start:    float; start value for scaling for chart search
            scale_stop:     float; stop value for scaling for chart search
            scale_step:     float; step value for scaling for chart search
        """
        self._file = chart_file
        self._height = height
        self._distance = distance
        self._scale_start = scale_start
        self._scale_stop = scale_stop
        self._scale_step = scale_step
        self.xnorm, self.ynorm, self.wnorm, self.hnorm, self.scale = its.image.chart_located_per_argv()
        if not self.xnorm:
            with its.device.ItsSession() as cam:
                props = cam.get_camera_properties()
                if its.caps.read_3a(props):
                    self.locate(cam, props)
                else:
                    print 'Chart locator skipped.'
                    self._set_scale_factors_to_one()

    def _set_scale_factors_to_one(self):
        """Set scale factors to 1.0 for skipped tests."""
        self.wnorm = 1.0
        self.hnorm = 1.0
        self.xnorm = 0.0
        self.ynorm = 0.0
        self.scale = 1.0

    def _calc_scale_factors(self, cam, props, fmt, s, e, fd):
        """Take an image with s, e, & fd to find the chart location.

        Args:
            cam:            An open device session.
            props:          Properties of cam
            fmt:            Image format for the capture
            s:              Sensitivity for the AF request as defined in
                            android.sensor.sensitivity
            e:              Exposure time for the AF request as defined in
                            android.sensor.exposureTime
            fd:             float; autofocus lens position
        Returns:
            template:       numpy array; chart template for locator
            img_3a:         numpy array; RGB image for chart location
            scale_factor:   float; scaling factor for chart search
        """
        req = its.objects.manual_capture_request(s, e)
        req['android.lens.focusDistance'] = fd
        cap_chart = its.image.stationary_lens_cap(cam, req, fmt)
        img_3a = its.image.convert_capture_to_rgb_image(cap_chart, props)
        img_3a = its.image.rotate_img_per_argv(img_3a)
        its.image.write_image(img_3a, 'af_scene.jpg')
        template = cv2.imread(self._file, cv2.IMREAD_ANYDEPTH)
        focal_l = cap_chart['metadata']['android.lens.focalLength']
        pixel_pitch = (props['android.sensor.info.physicalSize']['height'] /
                       img_3a.shape[0])
        print ' Chart distance: %.2fcm' % self._distance
        print ' Chart height: %.2fcm' % self._height
        print ' Focal length: %.2fmm' % focal_l
        print ' Pixel pitch: %.2fum' % (pixel_pitch*1E3)
        print ' Template height: %dpixels' % template.shape[0]
        chart_pixel_h = self._height * focal_l / (self._distance * pixel_pitch)
        scale_factor = template.shape[0] / chart_pixel_h
        print 'Chart/image scale factor = %.2f' % scale_factor
        return template, img_3a, scale_factor

    def locate(self, cam, props):
        """Find the chart in the image, and append location to chart object.

        The values appended are:
            xnorm:          float; [0, 1] left loc of chart in scene
            ynorm:          float; [0, 1] top loc of chart in scene
            wnorm:          float; [0, 1] width of chart in scene
            hnorm:          float; [0, 1] height of chart in scene
            scale:          float; scale factor to extract chart

        Args:
            cam:            An open device session
            props:          Camera properties
        """
        if its.caps.read_3a(props):
            s, e, _, _, fd = cam.do_3a(get_results=True)
            fmt = {'format': 'yuv', 'width': VGA_WIDTH, 'height': VGA_HEIGHT}
            chart, scene, s_factor = self._calc_scale_factors(cam, props, fmt,
                                                              s, e, fd)
        else:
            print 'Chart locator skipped.'
            self._set_scale_factors_to_one()
            return
        scale_start = self._scale_start * s_factor
        scale_stop = self._scale_stop * s_factor
        scale_step = self._scale_step * s_factor
        self.scale = s_factor
        max_match = []
        # check for normalized image
        if numpy.amax(scene) <= 1.0:
            scene = (scene * 255.0).astype(numpy.uint8)
        scene_gray = gray_scale_img(scene)
        print 'Finding chart in scene...'
        for scale in numpy.arange(scale_start, scale_stop, scale_step):
            scene_scaled = scale_img(scene_gray, scale)
            result = cv2.matchTemplate(scene_scaled, chart, cv2.TM_CCOEFF)
            _, opt_val, _, top_left_scaled = cv2.minMaxLoc(result)
            # print out scale and match
            print ' scale factor: %.3f, opt val: %.f' % (scale, opt_val)
            max_match.append((opt_val, top_left_scaled))

        # determine if optimization results are valid
        opt_values = [x[0] for x in max_match]
        if 2.0*min(opt_values) > max(opt_values):
            estring = ('Warning: unable to find chart in scene!\n'
                       'Check camera distance and self-reported '
                       'pixel pitch, focal length and hyperfocal distance.')
            print estring
            self._set_scale_factors_to_one()
        else:
            if (max(opt_values) == opt_values[0] or
                        max(opt_values) == opt_values[len(opt_values)-1]):
                estring = ('Warning: chart is at extreme range of locator '
                           'check.\n')
                print estring
            # find max and draw bbox
            match_index = max_match.index(max(max_match, key=lambda x: x[0]))
            self.scale = scale_start + scale_step * match_index
            print 'Optimum scale factor: %.3f' %  self.scale
            top_left_scaled = max_match[match_index][1]
            h, w = chart.shape
            bottom_right_scaled = (top_left_scaled[0] + w,
                                   top_left_scaled[1] + h)
            top_left = (int(top_left_scaled[0]/self.scale),
                        int(top_left_scaled[1]/self.scale))
            bottom_right = (int(bottom_right_scaled[0]/self.scale),
                            int(bottom_right_scaled[1]/self.scale))
            self.wnorm = float((bottom_right[0]) - top_left[0]) / scene.shape[1]
            self.hnorm = float((bottom_right[1]) - top_left[1]) / scene.shape[0]
            self.xnorm = float(top_left[0]) / scene.shape[1]
            self.ynorm = float(top_left[1]) / scene.shape[0]


def get_angle(input_img):
    """Computes anglular inclination of chessboard in input_img.

    Angle estimation algoritm description:
        Input: 2D grayscale image of chessboard.
        Output: Angle of rotation of chessboard perpendicular to
            chessboard. Assumes chessboard and camera are parallel to
            each other.

        1) Use adaptive threshold to make image binary
        2) Find countours
        3) Filter out small contours
        4) Filter out all non-square contours
        5) Compute most common square shape.
            The assumption here is that the most common square instances
            are the chessboard squares. We've shown that with our current
            tuning, we can robustly identify the squares on the sensor fusion
            chessboard.
        6) Return median angle of most common square shape.

    USAGE NOTE: This function has been tuned to work for the chessboard used in
    the sensor_fusion tests. See images in test_images/rotated_chessboard/ for
    sample captures. If this function is used with other chessboards, it may not
    work as expected.

    TODO: Make algorithm more robust so it works on any type of
    chessboard.

    Args:
        input_img (2D numpy.ndarray): Grayscale image stored as a 2D
            numpy array.

    Returns:
        Median angle of squares in degrees identified in the image.
    """
    # Tuning parameters
    min_square_area = (float)(input_img.shape[1] * 0.05)

    # Creates copy of image to avoid modifying original.
    img = numpy.array(input_img, copy=True)

    # Scale pixel values from 0-1 to 0-255
    img *= 255
    img = img.astype(numpy.uint8)

    thresh = cv2.adaptiveThreshold(
            img, 255, cv2.ADAPTIVE_THRESH_MEAN_C, cv2.THRESH_BINARY, 201, 2)

    # Find all contours
    contours = []
    cv2_version = cv2.__version__
    if cv2_version.startswith('2.4.'):
        contours, _ = cv2.findContours(
                thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    elif cv2_version.startswith('3.2.'):
        _, contours, _ = cv2.findContours(
                thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)

    # Filter contours to squares only.
    square_contours = []

    for contour in contours:
        rect = cv2.minAreaRect(contour)
        _, (width, height), angle = rect

        # Skip non-squares (with 0.1 tolerance)
        tolerance = 0.1
        if width < height * (1 - tolerance) or width > height * (1 + tolerance):
            continue

        # Remove very small contours.
        # These are usually just tiny dots due to noise.
        area = cv2.contourArea(contour)
        if area < min_square_area:
            continue

        if cv2_version.startswith('2.4.'):
            box = numpy.int0(cv2.cv.BoxPoints(rect))
        elif cv2_version.startswith('3.2.'):
            box = numpy.int0(cv2.boxPoints(rect))
        square_contours.append(contour)

    areas = []
    for contour in square_contours:
        area = cv2.contourArea(contour)
        areas.append(area)

    median_area = numpy.median(areas)

    filtered_squares = []
    filtered_angles = []
    for square in square_contours:
        area = cv2.contourArea(square)
        if area < median_area * 0.90 or area > median_area * 1.10:
            continue

        filtered_squares.append(square)
        _, (width, height), angle = cv2.minAreaRect(square)
        filtered_angles.append(angle)

    if len(filtered_angles) < 10:
        return None

    return numpy.median(filtered_angles)


class __UnitTest(unittest.TestCase):
    """Run a suite of unit tests on this module.
    """

    def test_compute_image_sharpness(self):
        """Unit test for compute_img_sharpness.

        Test by using PNG of ISO12233 chart and blurring intentionally.
        'sharpness' should drop off by sqrt(2) for 2x blur of image.

        We do one level of blur as PNG image is not perfect.
        """
        yuv_full_scale = 1023.0
        chart_file = os.path.join(os.environ['CAMERA_ITS_TOP'], 'pymodules',
                                  'its', 'test_images', 'ISO12233.png')
        chart = cv2.imread(chart_file, cv2.IMREAD_ANYDEPTH)
        white_level = numpy.amax(chart).astype(float)
        sharpness = {}
        for j in [2, 4, 8]:
            blur = cv2.blur(chart, (j, j))
            blur = blur[:, :, numpy.newaxis]
            sharpness[j] = (yuv_full_scale *
                            its.image.compute_image_sharpness(blur /
                                                              white_level))
        self.assertTrue(numpy.isclose(sharpness[2]/sharpness[4],
                                      numpy.sqrt(2), atol=0.1))
        self.assertTrue(numpy.isclose(sharpness[4]/sharpness[8],
                                      numpy.sqrt(2), atol=0.1))

    def test_get_angle_identify_unrotated_chessboard_angle(self):
        basedir = os.path.join(
                os.path.dirname(__file__), 'test_images/rotated_chessboards/')

        normal_img_path = os.path.join(basedir, 'normal.jpg')
        wide_img_path = os.path.join(basedir, 'wide.jpg')

        normal_img = cv2.cvtColor(
                cv2.imread(normal_img_path), cv2.COLOR_BGR2GRAY)
        wide_img = cv2.cvtColor(
                cv2.imread(wide_img_path), cv2.COLOR_BGR2GRAY)

        assert get_angle(normal_img) == 0
        assert get_angle(wide_img) == 0

    def test_get_angle_identify_rotated_chessboard_angle(self):
        basedir = os.path.join(
                os.path.dirname(__file__), 'test_images/rotated_chessboards/')

        # Array of the image files and angles containing rotated chessboards.
        test_cases = [
                ('_15_ccw', 15),
                ('_30_ccw', 30),
                ('_45_ccw', 45),
                ('_60_ccw', 60),
                ('_75_ccw', 75),
                ('_90_ccw', 90)
        ]

        # For each rotated image pair (normal, wide). Check if angle is
        # identified as expected.
        for suffix, angle in test_cases:
            # Define image paths
            normal_img_path = os.path.join(
                    basedir, 'normal{}.jpg'.format(suffix))
            wide_img_path = os.path.join(
                    basedir, 'wide{}.jpg'.format(suffix))

            # Load and color convert images
            normal_img = cv2.cvtColor(
                    cv2.imread(normal_img_path), cv2.COLOR_BGR2GRAY)
            wide_img = cv2.cvtColor(
                    cv2.imread(wide_img_path), cv2.COLOR_BGR2GRAY)

            # Assert angle is as expected up to 2.0 degrees of accuracy.
            assert numpy.isclose(
                    abs(get_angle(normal_img)), angle, 2.0)
            assert numpy.isclose(
                    abs(get_angle(wide_img)), angle, 2.0)


if __name__ == '__main__':
    unittest.main()
