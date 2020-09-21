# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Provides graphics related utils, like capturing screenshots or checking on
the state of the graphics driver.
"""

import collections
import contextlib
import glob
import logging
import os
import re
import sys
import time
#import traceback
# Please limit the use of the uinput library to this file. Try not to spread
# dependencies and abstract as much as possible to make switching to a different
# input library in the future easier.
import uinput

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import test as test_utils
from autotest_lib.client.cros.graphics import gbm
from autotest_lib.client.cros.input_playback import input_playback
from autotest_lib.client.cros.power import power_utils
from functools import wraps


class GraphicsTest(test.test):
    """Base class for graphics test.

    GraphicsTest is the base class for graphics tests.
    Every subclass of GraphicsTest should call GraphicsTests initialize/cleanup
    method as they will do GraphicsStateChecker as well as report states to
    Chrome Perf dashboard.

    Attributes:
        _test_failure_description(str): Failure name reported to chrome perf
                                        dashboard. (Default: Failures)
        _test_failure_report_enable(bool): Enable/Disable reporting
                                            failures to chrome perf dashboard
                                            automatically. (Default: True)
        _test_failure_report_subtest(bool): Enable/Disable reporting
                                            subtests failure to chrome perf
                                            dashboard automatically.
                                            (Default: False)
    """
    version = 1
    _GSC = None

    _test_failure_description = "Failures"
    _test_failure_report_enable = True
    _test_failure_report_subtest = False

    def __init__(self, *args, **kwargs):
        """Initialize flag setting."""
        super(GraphicsTest, self).__init__(*args, **kwargs)
        self._failures = []
        self._player = None

    def initialize(self, raise_error_on_hang=False, *args, **kwargs):
        """Initial state checker and report initial value to perf dashboard."""
        self._GSC = GraphicsStateChecker(
            raise_error_on_hang=raise_error_on_hang,
            run_on_sw_rasterizer=utils.is_virtual_machine())

        self.output_perf_value(
            description='Timeout_Reboot',
            value=1,
            units='count',
            higher_is_better=False,
            replace_existing_values=True
        )

        # Enable the graphics tests to use keyboard interaction.
        self._player = input_playback.InputPlayback()
        self._player.emulate(input_type='keyboard')
        self._player.find_connected_inputs()

        if hasattr(super(GraphicsTest, self), "initialize"):
            test_utils._cherry_pick_call(super(GraphicsTest, self).initialize,
                                         *args, **kwargs)

    def cleanup(self, *args, **kwargs):
        """Finalize state checker and report values to perf dashboard."""
        if self._GSC:
            self._GSC.finalize()

        self._output_perf()
        self._player.close()

        if hasattr(super(GraphicsTest, self), "cleanup"):
            test_utils._cherry_pick_call(super(GraphicsTest, self).cleanup,
                                         *args, **kwargs)

    @contextlib.contextmanager
    def failure_report(self, name, subtest=None):
        """Record the failure of an operation to the self._failures.

        Records if the operation taken inside executed normally or not.
        If the operation taken inside raise unexpected failure, failure named
        |name|, will be added to the self._failures list and reported to the
        chrome perf dashboard in the cleanup stage.

        Usage:
            # Record failure of doSomething
            with failure_report('doSomething'):
                doSomething()
        """
        # Assume failed at the beginning
        self.add_failures(name, subtest=subtest)
        yield {}
        self.remove_failures(name, subtest=subtest)

    @classmethod
    def failure_report_decorator(cls, name, subtest=None):
        """Record the failure if the function failed to finish.
        This method should only decorate to functions of GraphicsTest.
        In addition, functions with this decorator should be called with no
        unnamed arguments.
        Usage:
            @GraphicsTest.test_run_decorator('graphics_test')
            def Foo(self, bar='test'):
                return doStuff()

            is equivalent to

            def Foo(self, bar):
                with failure_reporter('graphics_test'):
                    return doStuff()

            # Incorrect usage.
            @GraphicsTest.test_run_decorator('graphics_test')
            def Foo(self, bar='test'):
                pass
            self.Foo('test_name', bar='test_name') # call Foo with named args

            # Incorrect usage.
            @GraphicsTest.test_run_decorator('graphics_test')
            def Foo(self, bar='test'):
                pass
            self.Foo('test_name') # call Foo with unnamed args
         """
        def decorator(fn):
            @wraps(fn)
            def wrapper(*args, **kwargs):
                if len(args) > 1:
                    raise error.TestError('Unnamed arguments is not accepted. '
                                          'Please apply this decorator to '
                                          'function without unnamed args.')
                # A member function of GraphicsTest is decorated. The first
                # argument is the instance itself.
                instance = args[0]
                with instance.failure_report(name, subtest):
                    # Cherry pick the arguments for the wrapped function.
                    d_args, d_kwargs = test_utils._cherry_pick_args(fn, args,
                                                                    kwargs)
                    return fn(instance, *d_args, **d_kwargs)
            return wrapper
        return decorator

    def add_failures(self, name, subtest=None):
        """
        Add a record to failures list which will report back to chrome perf
        dashboard at cleanup stage.
        Args:
            name: failure name.
            subtest: subtest which will appears in cros-perf. If None is
                     specified, use name instead.
        """
        target = self._get_failure(name, subtest=subtest)
        if target:
            target['names'].append(name)
        else:
            target = {
                'description': self._get_failure_description(name, subtest),
                'unit': 'count',
                'higher_is_better': False,
                'graph': self._get_failure_graph_name(),
                'names': [name],
            }
            self._failures.append(target)
        return target

    def remove_failures(self, name, subtest=None):
        """
        Remove a record from failures list which will report back to chrome perf
        dashboard at cleanup stage.
        Args:
            name: failure name.
            subtest: subtest which will appears in cros-perf. If None is
                     specified, use name instead.
        """
        target = self._get_failure(name, subtest=subtest)
        if name in target['names']:
            target['names'].remove(name)

    def _output_perf(self):
        """Report recorded failures back to chrome perf."""
        self.output_perf_value(
            description='Timeout_Reboot',
            value=0,
            units='count',
            higher_is_better=False,
            replace_existing_values=True
        )

        if not self._test_failure_report_enable:
            return

        total_failures = 0
        # Report subtests failures
        for failure in self._failures:
            logging.debug('GraphicsTest failure: %s' % failure['names'])
            total_failures += len(failure['names'])

            if not self._test_failure_report_subtest:
                continue

            self.output_perf_value(
                description=failure['description'],
                value=len(failure['names']),
                units=failure['unit'],
                higher_is_better=failure['higher_is_better'],
                graph=failure['graph']
            )

        # Report the count of all failures
        self.output_perf_value(
            description=self._get_failure_graph_name(),
            value=total_failures,
            units='count',
            higher_is_better=False,
        )

    def _get_failure_graph_name(self):
        return self._test_failure_description

    def _get_failure_description(self, name, subtest):
        return subtest or name

    def _get_failure(self, name, subtest):
        """Get specific failures."""
        description = self._get_failure_description(name, subtest=subtest)
        for failure in self._failures:
            if failure['description'] == description:
                return failure
        return None

    def get_failures(self):
        """
        Get currently recorded failures list.
        """
        return [name for failure in self._failures
                for name in failure['names']]

    def open_vt1(self):
        """Switch to VT1 with keyboard."""
        self._player.blocking_playback_of_default_file(
            input_type='keyboard', filename='keyboard_ctrl+alt+f1')
        time.sleep(5)

    def open_vt2(self):
        """Switch to VT2 with keyboard."""
        self._player.blocking_playback_of_default_file(
            input_type='keyboard', filename='keyboard_ctrl+alt+f2')
        time.sleep(5)

    def wake_screen_with_keyboard(self):
        """Use the vt1 keyboard shortcut to bring the devices screen back on.

        This is useful if you want to take screenshots of the UI. If you try
        to take them while the screen is off, it will fail.
        """
        self.open_vt1()


def screen_disable_blanking():
    """ Called from power_Backlight to disable screen blanking. """
    # We don't have to worry about unexpected screensavers or DPMS here.
    return


def screen_disable_energy_saving():
    """ Called from power_Consumption to immediately disable energy saving. """
    # All we need to do here is enable displays via Chrome.
    power_utils.set_display_power(power_utils.DISPLAY_POWER_ALL_ON)
    return


def screen_toggle_fullscreen():
    """Toggles fullscreen mode."""
    press_keys(['KEY_F11'])


def screen_toggle_mirrored():
    """Toggles the mirrored screen."""
    press_keys(['KEY_LEFTCTRL', 'KEY_F4'])


def hide_cursor():
    """Hides mouse cursor."""
    # Send a keystroke to hide the cursor.
    press_keys(['KEY_UP'])


def hide_typing_cursor():
    """Hides typing cursor."""
    # Press the tab key to move outside the typing bar.
    press_keys(['KEY_TAB'])


def screen_wakeup():
    """Wake up the screen if it is dark."""
    # Move the mouse a little bit to wake up the screen.
    device = _get_uinput_device_mouse_rel()
    _uinput_emit(device, 'REL_X', 1)
    _uinput_emit(device, 'REL_X', -1)


def switch_screen_on(on):
    """
    Turn the touch screen on/off.

    @param on: On or off.
    """
    raise error.TestFail('switch_screen_on is not implemented.')


# Don't create a device during build_packages or for tests that don't need it.
uinput_device_keyboard = None
uinput_device_touch = None
uinput_device_mouse_rel = None

# Don't add more events to this list than are used. For a complete list of
# available events check python2.7/site-packages/uinput/ev.py.
UINPUT_DEVICE_EVENTS_KEYBOARD = [
    uinput.KEY_F4,
    uinput.KEY_F11,
    uinput.KEY_KPPLUS,
    uinput.KEY_KPMINUS,
    uinput.KEY_LEFTCTRL,
    uinput.KEY_TAB,
    uinput.KEY_UP,
    uinput.KEY_DOWN,
    uinput.KEY_LEFT,
    uinput.KEY_RIGHT,
    uinput.KEY_RIGHTSHIFT,
    uinput.KEY_LEFTALT,
    uinput.KEY_A,
    uinput.KEY_M,
    uinput.KEY_V
]
# TODO(ihf): Find an ABS sequence that actually works.
UINPUT_DEVICE_EVENTS_TOUCH = [
    uinput.BTN_TOUCH,
    uinput.ABS_MT_SLOT,
    uinput.ABS_MT_POSITION_X + (0, 2560, 0, 0),
    uinput.ABS_MT_POSITION_Y + (0, 1700, 0, 0),
    uinput.ABS_MT_TRACKING_ID + (0, 10, 0, 0),
    uinput.BTN_TOUCH
]
UINPUT_DEVICE_EVENTS_MOUSE_REL = [
    uinput.REL_X,
    uinput.REL_Y,
    uinput.BTN_MOUSE,
    uinput.BTN_LEFT,
    uinput.BTN_RIGHT
]


def _get_uinput_device_keyboard():
    """
    Lazy initialize device and return it. We don't want to create a device
    during build_packages or for tests that don't need it, hence init with None.
    """
    global uinput_device_keyboard
    if uinput_device_keyboard is None:
        uinput_device_keyboard = uinput.Device(UINPUT_DEVICE_EVENTS_KEYBOARD)
    return uinput_device_keyboard


def _get_uinput_device_mouse_rel():
    """
    Lazy initialize device and return it. We don't want to create a device
    during build_packages or for tests that don't need it, hence init with None.
    """
    global uinput_device_mouse_rel
    if uinput_device_mouse_rel is None:
        uinput_device_mouse_rel = uinput.Device(UINPUT_DEVICE_EVENTS_MOUSE_REL)
    return uinput_device_mouse_rel


def _get_uinput_device_touch():
    """
    Lazy initialize device and return it. We don't want to create a device
    during build_packages or for tests that don't need it, hence init with None.
    """
    global uinput_device_touch
    if uinput_device_touch is None:
        uinput_device_touch = uinput.Device(UINPUT_DEVICE_EVENTS_TOUCH)
    return uinput_device_touch


def _uinput_translate_name(event_name):
    """
    Translates string |event_name| to uinput event.
    """
    return getattr(uinput, event_name)


def _uinput_emit(device, event_name, value, syn=True):
    """
    Wrapper for uinput.emit. Emits event with value.
    Example: ('REL_X', 20), ('BTN_RIGHT', 1)
    """
    event = _uinput_translate_name(event_name)
    device.emit(event, value, syn)


def _uinput_emit_click(device, event_name, syn=True):
    """
    Wrapper for uinput.emit_click. Emits click event. Only KEY and BTN events
    are accepted, otherwise ValueError is raised. Example: 'KEY_A'
    """
    event = _uinput_translate_name(event_name)
    device.emit_click(event, syn)


def _uinput_emit_combo(device, event_names, syn=True):
    """
    Wrapper for uinput.emit_combo. Emits sequence of events.
    Example: ['KEY_LEFTCTRL', 'KEY_LEFTALT', 'KEY_F5']
    """
    events = [_uinput_translate_name(en) for en in event_names]
    device.emit_combo(events, syn)


def press_keys(key_list):
    """Presses the given keys as one combination.

    Please do not leak uinput dependencies outside of the file.

    @param key: A list of key strings, e.g. ['LEFTCTRL', 'F4']
    """
    _uinput_emit_combo(_get_uinput_device_keyboard(), key_list)


def click_mouse():
    """Just click the mouse.
    Presumably only hacky tests use this function.
    """
    logging.info('click_mouse()')
    # Move a little to make the cursor appear.
    device = _get_uinput_device_mouse_rel()
    _uinput_emit(device, 'REL_X', 1)
    # Some sleeping is needed otherwise events disappear.
    time.sleep(0.1)
    # Move cursor back to not drift.
    _uinput_emit(device, 'REL_X', -1)
    time.sleep(0.1)
    # Click down.
    _uinput_emit(device, 'BTN_LEFT', 1)
    time.sleep(0.2)
    # Release click.
    _uinput_emit(device, 'BTN_LEFT', 0)


# TODO(ihf): this function is broken. Make it work.
def activate_focus_at(rel_x, rel_y):
    """Clicks with the mouse at screen position (x, y).

    This is a pretty hacky method. Using this will probably lead to
    flaky tests as page layout changes over time.
    @param rel_x: relative horizontal position between 0 and 1.
    @param rel_y: relattive vertical position between 0 and 1.
    """
    width, height = get_internal_resolution()
    device = _get_uinput_device_touch()
    _uinput_emit(device, 'ABS_MT_SLOT', 0, syn=False)
    _uinput_emit(device, 'ABS_MT_TRACKING_ID', 1, syn=False)
    _uinput_emit(device, 'ABS_MT_POSITION_X', int(rel_x * width), syn=False)
    _uinput_emit(device, 'ABS_MT_POSITION_Y', int(rel_y * height), syn=False)
    _uinput_emit(device, 'BTN_TOUCH', 1, syn=True)
    time.sleep(0.2)
    _uinput_emit(device, 'BTN_TOUCH', 0, syn=True)


def take_screenshot(resultsdir, fname_prefix, extension='png'):
    """Take screenshot and save to a new file in the results dir.
    Args:
      @param resultsdir:   Directory to store the output in.
      @param fname_prefix: Prefix for the output fname.
      @param extension:    String indicating file format ('png', 'jpg', etc).
    Returns:
      the path of the saved screenshot file
    """

    old_exc_type = sys.exc_info()[0]

    next_index = len(glob.glob(
        os.path.join(resultsdir, '%s-*.%s' % (fname_prefix, extension))))
    screenshot_file = os.path.join(
        resultsdir, '%s-%d.%s' % (fname_prefix, next_index, extension))
    logging.info('Saving screenshot to %s.', screenshot_file)

    try:
        image = gbm.crtcScreenshot()
        image.save(screenshot_file)
    except Exception as err:
        # Do not raise an exception if the screenshot fails while processing
        # another exception.
        if old_exc_type is None:
            raise
        logging.error(err)

    return screenshot_file


def take_screenshot_crop_by_height(fullpath, final_height, x_offset_pixels,
                                   y_offset_pixels):
    """
    Take a screenshot, crop to final height starting at given (x, y) coordinate.
    Image width will be adjusted to maintain original aspect ratio).

    @param fullpath: path, fullpath of the file that will become the image file.
    @param final_height: integer, height in pixels of resulting image.
    @param x_offset_pixels: integer, number of pixels from left margin
                            to begin cropping.
    @param y_offset_pixels: integer, number of pixels from top margin
                            to begin cropping.
    """
    image = gbm.crtcScreenshot()
    image.crop()
    width, height = image.size
    # Preserve aspect ratio: Wf / Wi == Hf / Hi
    final_width = int(width * (float(final_height) / height))
    box = (x_offset_pixels, y_offset_pixels,
           x_offset_pixels + final_width, y_offset_pixels + final_height)
    cropped = image.crop(box)
    cropped.save(fullpath)
    return fullpath


def take_screenshot_crop_x(fullpath, box=None):
    """
    Take a screenshot using import tool, crop according to dim given by the box.
    @param fullpath: path, full path to save the image to.
    @param box: 4-tuple giving the upper left and lower right pixel coordinates.
    """

    if box:
        img_w, img_h, upperx, uppery = box
        cmd = ('/usr/local/bin/import -window root -depth 8 -crop '
                      '%dx%d+%d+%d' % (img_w, img_h, upperx, uppery))
    else:
        cmd = ('/usr/local/bin/import -window root -depth 8')

    old_exc_type = sys.exc_info()[0]
    try:
        utils.system('%s %s' % (cmd, fullpath))
    except Exception as err:
        # Do not raise an exception if the screenshot fails while processing
        # another exception.
        if old_exc_type is None:
            raise
        logging.error(err)


def take_screenshot_crop(fullpath, box=None, crtc_id=None):
    """
    Take a screenshot using import tool, crop according to dim given by the box.
    @param fullpath: path, full path to save the image to.
    @param box: 4-tuple giving the upper left and lower right pixel coordinates.
    """
    if crtc_id is not None:
        image = gbm.crtcScreenshot(crtc_id)
    else:
        image = gbm.crtcScreenshot(get_internal_crtc())
    if box:
        image = image.crop(box)
    image.save(fullpath)
    return fullpath


_MODETEST_CONNECTOR_PATTERN = re.compile(
    r'^(\d+)\s+\d+\s+(connected|disconnected)\s+(\S+)\s+\d+x\d+\s+\d+\s+\d+')

_MODETEST_MODE_PATTERN = re.compile(
    r'\s+.+\d+\s+(\d+)\s+\d+\s+\d+\s+\d+\s+(\d+)\s+\d+\s+\d+\s+\d+\s+flags:.+type:'
    r' preferred')

_MODETEST_CRTCS_START_PATTERN = re.compile(r'^id\s+fb\s+pos\s+size')

_MODETEST_CRTC_PATTERN = re.compile(
    r'^(\d+)\s+(\d+)\s+\((\d+),(\d+)\)\s+\((\d+)x(\d+)\)')

Connector = collections.namedtuple(
    'Connector', [
        'cid',  # connector id (integer)
        'ctype',  # connector type, e.g. 'eDP', 'HDMI-A', 'DP'
        'connected',  # boolean
        'size',  # current screen size, e.g. (1024, 768)
        'encoder',  # encoder id (integer)
        # list of resolution tuples, e.g. [(1920,1080), (1600,900), ...]
        'modes',
    ])

CRTC = collections.namedtuple(
    'CRTC', [
        'id',  # crtc id
        'fb',  # fb id
        'pos',  # position, e.g. (0,0)
        'size',  # size, e.g. (1366,768)
    ])


def get_display_resolution():
    """
    Parses output of modetest to determine the display resolution of the dut.
    @return: tuple, (w,h) resolution of device under test.
    """
    connectors = get_modetest_connectors()
    for connector in connectors:
        if connector.connected:
            return connector.size
    return None


def _get_num_outputs_connected():
    """
    Parses output of modetest to determine the number of connected displays
    @return: The number of connected displays
    """
    connected = 0
    connectors = get_modetest_connectors()
    for connector in connectors:
        if connector.connected:
            connected = connected + 1

    return connected


def get_num_outputs_on():
    """
    Retrieves the number of connected outputs that are on.

    Return value: integer value of number of connected outputs that are on.
    """

    return _get_num_outputs_connected()


def get_modetest_connectors():
    """
    Retrieves a list of Connectors using modetest.

    Return value: List of Connectors.
    """
    connectors = []
    modetest_output = utils.system_output('modetest -c')
    for line in modetest_output.splitlines():
        # First search for a new connector.
        connector_match = re.match(_MODETEST_CONNECTOR_PATTERN, line)
        if connector_match is not None:
            cid = int(connector_match.group(1))
            connected = False
            if connector_match.group(2) == 'connected':
                connected = True
            ctype = connector_match.group(3)
            size = (-1, -1)
            encoder = -1
            modes = None
            connectors.append(
                Connector(cid, ctype, connected, size, encoder, modes))
        else:
            # See if we find corresponding line with modes, sizes etc.
            mode_match = re.match(_MODETEST_MODE_PATTERN, line)
            if mode_match is not None:
                size = (int(mode_match.group(1)), int(mode_match.group(2)))
                # Update display size of last connector in list.
                c = connectors.pop()
                connectors.append(
                    Connector(
                        c.cid, c.ctype, c.connected, size, c.encoder,
                        c.modes))
    return connectors


def get_modetest_crtcs():
    """
    Returns a list of CRTC data.

    Sample:
        [CRTC(id=19, fb=50, pos=(0, 0), size=(1366, 768)),
         CRTC(id=22, fb=54, pos=(0, 0), size=(1920, 1080))]
    """
    crtcs = []
    modetest_output = utils.system_output('modetest -p')
    found = False
    for line in modetest_output.splitlines():
        if found:
            crtc_match = re.match(_MODETEST_CRTC_PATTERN, line)
            if crtc_match is not None:
                crtc_id = int(crtc_match.group(1))
                fb = int(crtc_match.group(2))
                x = int(crtc_match.group(3))
                y = int(crtc_match.group(4))
                width = int(crtc_match.group(5))
                height = int(crtc_match.group(6))
                # CRTCs with fb=0 are disabled, but lets skip anything with
                # trivial width/height just in case.
                if not (fb == 0 or width == 0 or height == 0):
                    crtcs.append(CRTC(crtc_id, fb, (x, y), (width, height)))
            elif line and not line[0].isspace():
                return crtcs
        if re.match(_MODETEST_CRTCS_START_PATTERN, line) is not None:
            found = True
    return crtcs


def get_modetest_output_state():
    """
    Reduce the output of get_modetest_connectors to a dictionary of connector/active states.
    """
    connectors = get_modetest_connectors()
    outputs = {}
    for connector in connectors:
        # TODO(ihf): Figure out why modetest output needs filtering.
        if connector.connected:
            outputs[connector.ctype] = connector.connected
    return outputs


def get_output_rect(output):
    """Gets the size and position of the given output on the screen buffer.

    @param output: The output name as a string.

    @return A tuple of the rectangle (width, height, fb_offset_x,
            fb_offset_y) of ints.
    """
    connectors = get_modetest_connectors()
    for connector in connectors:
        if connector.ctype == output:
            # Concatenate two 2-tuples to 4-tuple.
            return connector.size + (0, 0)  # TODO(ihf): Should we use CRTC.pos?
    return (0, 0, 0, 0)


def get_internal_resolution():
    if has_internal_display():
        crtcs = get_modetest_crtcs()
        if len(crtcs) > 0:
            return crtcs[0].size
    return (-1, -1)


def has_internal_display():
    """Checks whether the DUT is equipped with an internal display.

    @return True if internal display is present; False otherwise.
    """
    return bool(get_internal_connector_name())


def get_external_resolution():
    """Gets the resolution of the external display.

    @return A tuple of (width, height) or None if no external display is
            connected.
    """
    offset = 1 if has_internal_display() else 0
    crtcs = get_modetest_crtcs()
    if len(crtcs) > offset and crtcs[offset].size != (0, 0):
        return crtcs[offset].size
    return None


def get_display_output_state():
    """
    Retrieves output status of connected display(s).

    Return value: dictionary of connected display states.
    """
    return get_modetest_output_state()


def set_modetest_output(output_name, enable):
    # TODO(ihf): figure out what to do here. Don't think this is the right command.
    # modetest -s <connector_id>[,<connector_id>][@<crtc_id>]:<mode>[-<vrefresh>][@<format>]  set a mode
    pass


def set_display_output(output_name, enable):
    """
    Sets the output given by |output_name| on or off.
    """
    set_modetest_output(output_name, enable)


# TODO(ihf): Fix this for multiple external connectors.
def get_external_crtc(index=0):
    offset = 1 if has_internal_display() else 0
    crtcs = get_modetest_crtcs()
    if len(crtcs) > offset + index:
        return crtcs[offset + index].id
    return -1


def get_internal_crtc():
    if has_internal_display():
        crtcs = get_modetest_crtcs()
        if len(crtcs) > 0:
            return crtcs[0].id
    return -1


# TODO(ihf): Fix this for multiple external connectors.
def get_external_connector_name():
    """Gets the name of the external output connector.

    @return The external output connector name as a string, if any.
            Otherwise, return False.
    """
    outputs = get_display_output_state()
    for output in outputs.iterkeys():
        if outputs[output] and (output.startswith('HDMI')
                or output.startswith('DP')
                or output.startswith('DVI')
                or output.startswith('VGA')):
            return output
    return False


def get_internal_connector_name():
    """Gets the name of the internal output connector.

    @return The internal output connector name as a string, if any.
            Otherwise, return False.
    """
    outputs = get_display_output_state()
    for output in outputs.iterkeys():
        # reference: chromium_org/chromeos/display/output_util.cc
        if (output.startswith('eDP')
                or output.startswith('LVDS')
                or output.startswith('DSI')):
            return output
    return False


def wait_output_connected(output):
    """Wait for output to connect.

    @param output: The output name as a string.

    @return: True if output is connected; False otherwise.
    """
    def _is_connected(output):
        """Helper function."""
        outputs = get_display_output_state()
        if output not in outputs:
            return False
        return outputs[output]

    return utils.wait_for_value(lambda: _is_connected(output),
                                expected_value=True)


def set_content_protection(output_name, state):
    """
    Sets the content protection to the given state.

    @param output_name: The output name as a string.
    @param state: One of the states 'Undesired', 'Desired', or 'Enabled'

    """
    raise error.TestFail('freon: set_content_protection not implemented')


def get_content_protection(output_name):
    """
    Gets the state of the content protection.

    @param output_name: The output name as a string.
    @return: A string of the state, like 'Undesired', 'Desired', or 'Enabled'.
             False if not supported.

    """
    raise error.TestFail('freon: get_content_protection not implemented')


def is_sw_rasterizer():
    """Return true if OpenGL is using a software rendering."""
    cmd = utils.wflinfo_cmd() + ' | grep "OpenGL renderer string"'
    output = utils.run(cmd)
    result = output.stdout.splitlines()[0]
    logging.info('wflinfo: %s', result)
    # TODO(ihf): Find exhaustive error conditions (especially ARM).
    return 'llvmpipe' in result.lower() or 'soft' in result.lower()


def get_gles_version():
    cmd = utils.wflinfo_cmd()
    wflinfo = utils.system_output(cmd, retain_output=False, ignore_status=False)
    # OpenGL version string: OpenGL ES 3.0 Mesa 10.5.0-devel
    version = re.findall(r'OpenGL version string: '
                         r'OpenGL ES ([0-9]+).([0-9]+)', wflinfo)
    if version:
        version_major = int(version[0][0])
        version_minor = int(version[0][1])
        return (version_major, version_minor)
    return (None, None)


def get_egl_version():
    cmd = 'eglinfo'
    eglinfo = utils.system_output(cmd, retain_output=False, ignore_status=False)
    # EGL version string: 1.4 (DRI2)
    version = re.findall(r'EGL version string: ([0-9]+).([0-9]+)', eglinfo)
    if version:
        version_major = int(version[0][0])
        version_minor = int(version[0][1])
        return (version_major, version_minor)
    return (None, None)


class GraphicsKernelMemory(object):
    """
    Reads from sysfs to determine kernel gem objects and memory info.
    """
    # These are sysfs fields that will be read by this test.  For different
    # architectures, the sysfs field paths are different.  The "paths" are given
    # as lists of strings because the actual path may vary depending on the
    # system.  This test will read from the first sysfs path in the list that is
    # present.
    # e.g. ".../memory" vs ".../gpu_memory" -- if the system has either one of
    # these, the test will read from that path.
    amdgpu_fields = {
        'gem_objects': ['/sys/kernel/debug/dri/0/amdgpu_gem_info'],
        'memory': ['/sys/kernel/debug/dri/0/amdgpu_gtt_mm'],
    }
    arm_fields = {}
    exynos_fields = {
        'gem_objects': ['/sys/kernel/debug/dri/?/exynos_gem_objects'],
        'memory': ['/sys/class/misc/mali0/device/memory',
                   '/sys/class/misc/mali0/device/gpu_memory'],
    }
    mediatek_fields = {}  # TODO(crosbug.com/p/58189) add nodes
    # TODO Add memory nodes once the GPU patches landed.
    rockchip_fields = {}
    tegra_fields = {
        'memory': ['/sys/kernel/debug/memblock/memory'],
    }
    i915_fields = {
        'gem_objects': ['/sys/kernel/debug/dri/0/i915_gem_objects'],
        'memory': ['/sys/kernel/debug/dri/0/i915_gem_gtt'],
    }
    cirrus_fields = {}
    virtio_fields = {}

    arch_fields = {
        'amdgpu': amdgpu_fields,
        'arm': arm_fields,
        'cirrus': cirrus_fields,
        'exynos5': exynos_fields,
        'i915': i915_fields,
        'mediatek': mediatek_fields,
        'rockchip': rockchip_fields,
        'tegra': tegra_fields,
        'virtio': virtio_fields,
    }


    num_errors = 0

    def __init__(self):
        self._initial_memory = self.get_memory_keyvals()

    def get_memory_difference_keyvals(self):
        """
        Reads the graphics memory values and return the difference between now
        and the memory usage at initialization stage as keyvals.
        """
        current_memory = self.get_memory_keyvals()
        return {key: self._initial_memory[key] - current_memory[key]
                for key in self._initial_memory}

    def get_memory_keyvals(self):
        """
        Reads the graphics memory values and returns them as keyvals.
        """
        keyvals = {}

        # Get architecture type and list of sysfs fields to read.
        soc = utils.get_cpu_soc_family()

        arch = utils.get_cpu_arch()
        if arch == 'x86_64' or arch == 'i386':
            pci_vga_device = utils.run("lspci | grep VGA").stdout.rstrip('\n')
            if "Advanced Micro Devices" in pci_vga_device:
                soc = 'amdgpu'
            elif "Intel Corporation" in pci_vga_device:
                soc = 'i915'
            elif "Cirrus Logic" in pci_vga_device:
                # Used on qemu with kernels 3.18 and lower. Limited to 800x600
                # resolution.
                soc = 'cirrus'
            else:
                pci_vga_device = utils.run('lshw -c video').stdout.rstrip()
                groups = re.search('configuration:.*driver=(\S*)',
                                   pci_vga_device)
                if groups and 'virtio' in groups.group(1):
                    soc = 'virtio'

        if not soc in self.arch_fields:
            raise error.TestFail('Error: Architecture "%s" not yet supported.' % soc)
        fields = self.arch_fields[soc]

        for field_name in fields:
            possible_field_paths = fields[field_name]
            field_value = None
            for path in possible_field_paths:
                if utils.system('ls %s' % path):
                    continue
                field_value = utils.system_output('cat %s' % path)
                break

            if not field_value:
                logging.error('Unable to find any sysfs paths for field "%s"',
                              field_name)
                self.num_errors += 1
                continue

            parsed_results = GraphicsKernelMemory._parse_sysfs(field_value)

            for key in parsed_results:
                keyvals['%s_%s' % (field_name, key)] = parsed_results[key]

            if 'bytes' in parsed_results and parsed_results['bytes'] == 0:
                logging.error('%s reported 0 bytes', field_name)
                self.num_errors += 1

        keyvals['meminfo_MemUsed'] = (utils.read_from_meminfo('MemTotal') -
                                      utils.read_from_meminfo('MemFree'))
        keyvals['meminfo_SwapUsed'] = (utils.read_from_meminfo('SwapTotal') -
                                       utils.read_from_meminfo('SwapFree'))
        return keyvals

    @staticmethod
    def _parse_sysfs(output):
        """
        Parses output of graphics memory sysfs to determine the number of
        buffer objects and bytes.

        Arguments:
            output      Unprocessed sysfs output
        Return value:
            Dictionary containing integer values of number bytes and objects.
            They may have the keys 'bytes' and 'objects', respectively.  However
            the result may not contain both of these values.
        """
        results = {}
        labels = ['bytes', 'objects']

        for line in output.split('\n'):
            # Strip any commas to make parsing easier.
            line_words = line.replace(',', '').split()

            prev_word = None
            for word in line_words:
                # When a label has been found, the previous word should be the
                # value. e.g. "3200 bytes"
                if word in labels and word not in results and prev_word:
                    logging.info(prev_word)
                    results[word] = int(prev_word)

                prev_word = word

            # Once all values has been parsed, return.
            if len(results) == len(labels):
                return results

        return results


class GraphicsStateChecker(object):
    """
    Analyzes the state of the GPU and log history. Should be instantiated at the
    beginning of each graphics_* test.
    """
    crash_blacklist = []
    dirty_writeback_centisecs = 0
    existing_hangs = {}

    _BROWSER_VERSION_COMMAND = '/opt/google/chrome/chrome --version'
    _HANGCHECK = ['drm:i915_hangcheck_elapsed', 'drm:i915_hangcheck_hung',
                  'Hangcheck timer elapsed...']
    _HANGCHECK_WARNING = ['render ring idle']
    _MESSAGES_FILE = '/var/log/messages'

    def __init__(self, raise_error_on_hang=True, run_on_sw_rasterizer=False):
        """
        Analyzes the initial state of the GPU and log history.
        """
        # Attempt flushing system logs every second instead of every 10 minutes.
        self.dirty_writeback_centisecs = utils.get_dirty_writeback_centisecs()
        utils.set_dirty_writeback_centisecs(100)
        self._raise_error_on_hang = raise_error_on_hang
        logging.info(utils.get_board_with_frequency_and_memory())
        self.graphics_kernel_memory = GraphicsKernelMemory()
        self._run_on_sw_rasterizer = run_on_sw_rasterizer

        if utils.get_cpu_arch() != 'arm':
            if not self._run_on_sw_rasterizer and is_sw_rasterizer():
                raise error.TestFail('Refusing to run on SW rasterizer.')
            logging.info('Initialize: Checking for old GPU hangs...')
            messages = open(self._MESSAGES_FILE, 'r')
            for line in messages:
                for hang in self._HANGCHECK:
                    if hang in line:
                        logging.info(line)
                        self.existing_hangs[line] = line
            messages.close()

    def finalize(self):
        """
        Analyzes the state of the GPU, log history and emits warnings or errors
        if the state changed since initialize. Also makes a note of the Chrome
        version for later usage in the perf-dashboard.
        """
        utils.set_dirty_writeback_centisecs(self.dirty_writeback_centisecs)
        new_gpu_hang = False
        new_gpu_warning = False
        if utils.get_cpu_arch() != 'arm':
            logging.info('Cleanup: Checking for new GPU hangs...')
            messages = open(self._MESSAGES_FILE, 'r')
            for line in messages:
                for hang in self._HANGCHECK:
                    if hang in line:
                        if not line in self.existing_hangs.keys():
                            logging.info(line)
                            for warn in self._HANGCHECK_WARNING:
                                if warn in line:
                                    new_gpu_warning = True
                                    logging.warning(
                                        'Saw GPU hang warning during test.')
                                else:
                                    logging.warning('Saw GPU hang during test.')
                                    new_gpu_hang = True
            messages.close()

            if not self._run_on_sw_rasterizer and is_sw_rasterizer():
                logging.warning('Finished test on SW rasterizer.')
                raise error.TestFail('Finished test on SW rasterizer.')
            if self._raise_error_on_hang and new_gpu_hang:
                raise error.TestError('Detected GPU hang during test.')
            if new_gpu_hang:
                raise error.TestWarn('Detected GPU hang during test.')
            if new_gpu_warning:
                raise error.TestWarn('Detected GPU warning during test.')

    def get_memory_access_errors(self):
        """ Returns the number of errors while reading memory stats. """
        return self.graphics_kernel_memory.num_errors

    def get_memory_difference_keyvals(self):
        return self.graphics_kernel_memory.get_memory_difference_keyvals()

    def get_memory_keyvals(self):
        """ Returns memory stats. """
        return self.graphics_kernel_memory.get_memory_keyvals()

class GraphicsApiHelper(object):
    """
    Report on the available graphics APIs.
    Ex. gles2, gles3, gles31, and vk
    """
    _supported_apis = []

    DEQP_BASEDIR = os.path.join('/usr', 'local', 'deqp')
    DEQP_EXECUTABLE = {
        'gles2': os.path.join('modules', 'gles2', 'deqp-gles2'),
        'gles3': os.path.join('modules', 'gles3', 'deqp-gles3'),
        'gles31': os.path.join('modules', 'gles31', 'deqp-gles31'),
        'vk': os.path.join('external', 'vulkancts', 'modules',
                           'vulkan', 'deqp-vk')
    }

    def __init__(self):
        # Determine which executable should be run. Right now never egl.
        major, minor = get_gles_version()
        logging.info('Found gles%d.%d.', major, minor)
        if major is None or minor is None:
            raise error.TestFail(
                'Failed: Could not get gles version information (%d, %d).' %
                (major, minor)
            )
        if major >= 2:
            self._supported_apis.append('gles2')
        if major >= 3:
            self._supported_apis.append('gles3')
            if major > 3 or minor >= 1:
                self._supported_apis.append('gles31')

        # If libvulkan is installed, then assume the board supports vulkan.
        has_libvulkan = False
        for libdir in ('/usr/lib', '/usr/lib64',
                       '/usr/local/lib', '/usr/local/lib64'):
            if os.path.exists(os.path.join(libdir, 'libvulkan.so')):
                has_libvulkan = True

        if has_libvulkan:
            executable_path = os.path.join(
                self.DEQP_BASEDIR,
                self.DEQP_EXECUTABLE['vk']
            )
            if os.path.exists(executable_path):
                self._supported_apis.append('vk')
            else:
                logging.warning('Found libvulkan.so but did not find deqp-vk '
                                'binary for testing.')

    def get_supported_apis(self):
        """Return the list of supported apis. eg. gles2, gles3, vk etc.
        @returns: a copy of the supported api list will be returned
        """
        return list(self._supported_apis)

    def get_deqp_executable(self, api):
        """Return the path to the api executable."""
        if api not in self.DEQP_EXECUTABLE:
            raise KeyError(
                "%s is not a supported api for GraphicsApiHelper." % api
            )

        executable = os.path.join(
            self.DEQP_BASEDIR,
            self.DEQP_EXECUTABLE[api]
        )
        return executable
