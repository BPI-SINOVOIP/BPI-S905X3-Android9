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

import its.caps
from its.cv2image import get_angle
import its.device
import its.image
import its.objects
import its.target

import cv2
import matplotlib
from matplotlib import pylab
import numpy
import os

ANGLE_MASK = 10  # degrees
ANGULAR_DIFF_THRESHOLD = 5  # degrees
ANGULAR_MOVEMENT_THRESHOLD = 45  # degrees
NAME = os.path.basename(__file__).split(".")[0]
NUM_CAPTURES = 100
W = 640
H = 480


def _check_available_capabilities(props):
    """Returns True if all required test capabilities are present."""
    return all([
            its.caps.compute_target_exposure(props),
            its.caps.per_frame_control(props),
            its.caps.logical_multi_camera(props),
            its.caps.raw16(props),
            its.caps.manual_sensor(props),
            its.caps.sensor_fusion(props)])


def _assert_camera_movement(frame_pairs_angles):
    """Assert the angles between each frame pair are sufficiently different.

    Different angles is an indication of camera movement.
    """
    angles = [i for i, j in frame_pairs_angles]
    max_angle = numpy.amax(angles)
    min_angle = numpy.amin(angles)
    emsg = "Not enough phone movement!\n"
    emsg += "min angle: %.2f, max angle: %.2f deg, THRESH: %d deg" % (
            min_angle, max_angle, ANGULAR_MOVEMENT_THRESHOLD)
    assert max_angle - min_angle > ANGULAR_MOVEMENT_THRESHOLD, emsg


def _assert_angular_difference(angle_1, angle_2):
    """Assert angular difference is within threshold."""
    diff = abs(angle_2 - angle_1)

    # Assert difference is less than threshold
    emsg = "Diff between frame pair: %.1f. Threshold: %d deg." % (
            diff, ANGULAR_DIFF_THRESHOLD)
    assert diff < ANGULAR_DIFF_THRESHOLD, emsg


def _mask_angles_near_extremes(frame_pairs_angles):
    """Mask out the data near the top and bottom of angle range."""
    masked_pairs_angles = [[i, j] for i, j in frame_pairs_angles
                           if ANGLE_MASK <= abs(i) <= 90-ANGLE_MASK]
    return masked_pairs_angles


def _plot_frame_pairs_angles(frame_pairs_angles, ids):
    """Plot the extracted angles."""
    matplotlib.pyplot.figure("Camera Rotation Angle")
    cam0_angles = [i for i, j in frame_pairs_angles]
    cam1_angles = [j for i, j in frame_pairs_angles]
    pylab.plot(range(len(cam0_angles)), cam0_angles, "r", label="%s" % ids[0])
    pylab.plot(range(len(cam1_angles)), cam1_angles, "g", label="%s" % ids[1])
    pylab.legend()
    pylab.xlabel("Camera frame number")
    pylab.ylabel("Rotation angle (degrees)")
    matplotlib.pyplot.savefig("%s_angles_plot.png" % (NAME))

    matplotlib.pyplot.figure("Angle Diffs")
    angle_diffs = [j-i for i, j in frame_pairs_angles]
    pylab.plot(range(len(angle_diffs)), angle_diffs, "b",
               label="cam%s-%s" % (ids[1], ids[0]))
    pylab.legend()
    pylab.xlabel("Camera frame number")
    pylab.ylabel("Rotation angle difference (degrees)")
    matplotlib.pyplot.savefig("%s_angle_diffs_plot.png" % (NAME))

def _collect_data():
    """Returns list of pair of gray frames and camera ids used for captures."""
    yuv_sizes = {}
    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()

        # If capabilities not present, skip.
        its.caps.skip_unless(_check_available_capabilities(props))

        # Determine return parameters
        debug = its.caps.debug_mode()
        ids = its.caps.logical_multi_camera_physical_ids(props)

        # Define capture request
        s, e, _, _, f = cam.do_3a(get_results=True)
        req = its.objects.manual_capture_request(s, e, f)

        # capture YUVs
        out_surfaces = [{"format": "yuv", "width": W, "height": H,
                         "physicalCamera": ids[0]},
                        {"format": "yuv", "width": W, "height": H,
                         "physicalCamera": ids[1]}]

        capture_1_list, capture_2_list = cam.do_capture(
            [req]*NUM_CAPTURES, out_surfaces)

        # Create list of capture pairs. [[cap1A, cap1B], [cap2A, cap2B], ...]
        frame_pairs = zip(capture_1_list, capture_2_list)

        # Convert captures to grayscale
        frame_pairs_gray = [
            [
                cv2.cvtColor(its.image.convert_capture_to_rgb_image(f, props=props), cv2.COLOR_RGB2GRAY) for f in pair
            ] for pair in frame_pairs]

        # Save images for debugging
        if debug:
            for i, imgs in enumerate(frame_pairs_gray):
                for j in [0, 1]:
                    file_name = "%s_%s_%03d.png" % (NAME, ids[j], i)
                    cv2.imwrite(file_name, imgs[j]*255)

        return frame_pairs_gray, ids

def main():
    """Test frame timestamps captured by logical camera are within 10ms."""
    frame_pairs_gray, ids = _collect_data()

    # Compute angles in frame pairs
    frame_pairs_angles = [
            [get_angle(p[0]), get_angle(p[1])] for p in frame_pairs_gray]

    # Remove frames where not enough squares were detected.
    filtered_pairs_angles = []
    for angle_1, angle_2 in frame_pairs_angles:
        if angle_1 == None or angle_2 == None:
            continue
        filtered_pairs_angles.append([angle_1, angle_2])

    print 'Using {} image pairs to compute angular difference.'.format(
        len(filtered_pairs_angles))

    assert len(filtered_pairs_angles) > 20, (
        "Unable to identify enough frames with detected squares.")

    # Mask out data near 90 degrees.
    # The chessboard angles we compute go from 0 to 89. Meaning,
    # 90 degrees equals to 0 degrees.
    # In order to avoid this jump, we ignore any frames at these extremeties.
    masked_pairs_angles = _mask_angles_near_extremes(filtered_pairs_angles)

    # Plot angles and differences
    _plot_frame_pairs_angles(filtered_pairs_angles, ids)

    # Ensure camera moved
    _assert_camera_movement(filtered_pairs_angles)

    # Ensure angle between images from each camera does not change appreciably
    for cam_1_angle, cam_2_angle in masked_pairs_angles:
        _assert_angular_difference(cam_1_angle, cam_2_angle)

if __name__ == "__main__":
    main()
