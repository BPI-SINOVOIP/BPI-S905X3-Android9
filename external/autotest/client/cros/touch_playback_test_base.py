# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import shutil
import subprocess
import tempfile
import time
import urllib
import urllib2

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.cros.input_playback import input_playback


class touch_playback_test_base(test.test):
    """Base class for touch tests involving playback."""
    version = 1

    _INPUTCONTROL = '/opt/google/input/inputcontrol'


    @property
    def _has_touchpad(self):
        """True if device under test has a touchpad; else False."""
        return self.player.has('touchpad')


    @property
    def _has_touchscreen(self):
        """True if device under test has a touchscreen; else False."""
        return self.player.has('touchscreen')


    @property
    def _has_mouse(self):
        """True if device under test has or emulates a USB mouse; else False."""
        return self.player.has('mouse')


    def warmup(self, mouse_props=None):
        """Test setup.

        Instantiate player object to find touch devices, if any.
        These devices can be used for playback later.
        Emulate a USB mouse if a property file is provided.
        Check if the inputcontrol script is avaiable on the disk.

        @param mouse_props: optional property file for a mouse to emulate.
                            Created using 'evemu-describe /dev/input/X'.

        """
        self.player = input_playback.InputPlayback()
        if mouse_props:
            self.player.emulate(input_type='mouse', property_file=mouse_props)
        self.player.find_connected_inputs()

        self._autotest_ext = None
        self._has_inputcontrol = os.path.isfile(self._INPUTCONTROL)
        self._platform = utils.get_board()
        if 'cheets' in self._platform:
            self._platform = self._platform[:-len('-cheets')]


    def _find_test_files(self, input_type, gestures):
        """Determine where the playback gesture files for this test are.

        Expected file format is: <boardname>_<input type>_<hwid>_<gesture name>
            e.g. samus_touchpad_164.17_scroll_down

        @param input_type: device type, e.g. 'touchpad'
        @param gestures: list of gesture name strings used in filename

        @returns: None if not all files are found.  Dictionary of filepaths if
                  they are found, indexed by gesture names as given.
        @raises: error.TestError if no device is found or if device should have
                 a hw_id but does not.

        """
        if type(gestures) is not list:
            raise error.TestError('find_test_files() takes a LIST, not a '
                                   '%s!' % type(gestures))

        if not self.player.has(input_type):
            raise error.TestError('Device does not have a %s!' % input_type)

        if input_type in ['touchpad', 'touchscreen', 'stylus']:
            hw_id = self.player.devices[input_type].hw_id
            if not hw_id:
                raise error.TestError('No valid hw_id for %s!' % input_type)
            filename_fmt = '%s_%s_%s' % (self._platform, input_type, hw_id)

        else:
            device_name = self.player.devices[input_type].name
            filename_fmt = '%s_%s' % (device_name, input_type)

        filepaths = {}
        for gesture in gestures:
            filename = '%s_%s' % (filename_fmt, gesture)
            filepath = self._download_remote_test_file(filename, input_type)
            if not filepath:
                logging.info('Did not find files for this device!')
                return None

            filepaths[gesture] = filepath

        return filepaths


    def _find_test_files_from_directions(self, input_type, fmt_str, directions):
        """Find gesture files given a list of directions and name format.

        @param input_type: device type, e.g. 'touchpad'
        @param fmt_str: format string for filename, e.g. 'scroll-%s'
        @param directions: list of directions for fmt_string

        @returns: None if not all files are found.  Dictionary of filepaths if
                  they are found, indexed by directions as given.
        @raises: error.TestError if no hw_id is found.

        """
        gestures = [fmt_str % d for d in directions]
        temp_filepaths = self._find_test_files(input_type, gestures)

        filepaths = {}
        if temp_filepaths:
            filepaths = {d: temp_filepaths[fmt_str % d] for d in directions}

        return filepaths


    def _download_remote_test_file(self, filename, input_type):
        """Download a file from the remote touch playback folder.

        @param filename: string of filename
        @param input_type: device type, e.g. 'touchpad'

        @returns: Path to local file or None if file is not found.

        """
        REMOTE_STORAGE_URL = ('https://storage.googleapis.com/'
                              'chromiumos-test-assets-public/touch_playback')
        filename = urllib.quote(filename)

        if input_type in ['touchpad', 'touchscreen', 'stylus']:
            url = '%s/%s/%s' % (REMOTE_STORAGE_URL, self._platform, filename)
        else:
            url = '%s/TYPE-%s/%s' % (REMOTE_STORAGE_URL, input_type, filename)
        local_file = os.path.join(self.bindir, filename)

        logging.info('Looking for %s', url)
        try:
            file_utils.download_file(url, local_file)
        except urllib2.URLError as e:
            logging.info('File download failed!')
            logging.debug(e.msg)
            return None

        return local_file


    def _emulate_mouse(self, property_file=None):
        """Emulate a mouse with the given property file.

        player will use default mouse if no file is provided.

        """
        self.player.emulate(input_type='mouse', property_file=property_file)
        self.player.find_connected_inputs()
        if not self._has_mouse:
            raise error.TestError('Mouse emulation failed!')


    def _playback(self, filepath, touch_type='touchpad'):
        """Playback a given input file on the given input."""
        self.player.playback(filepath, touch_type)


    def _blocking_playback(self, filepath, touch_type='touchpad'):
        """Playback a given input file on the given input; block until done."""
        self.player.blocking_playback(filepath, touch_type)


    def _set_touch_setting_by_inputcontrol(self, setting, value):
        """Set a given touch setting the given value by inputcontrol.

        @param setting: Name of touch setting, e.g. 'tapclick'.
        @param value: True for enabled, False for disabled.

        """
        cmd_value = 1 if value else 0
        utils.run('%s --%s %d' % (self._INPUTCONTROL, setting, cmd_value))
        logging.info('%s turned %s.', setting, 'on' if value else 'off')


    def _set_touch_setting(self, inputcontrol_setting, autotest_ext_setting,
                           value):
        """Set a given touch setting the given value.

        @param inputcontrol_setting: Name of touch setting for the inputcontrol
                                     script, e.g. 'tapclick'.
        @param autotest_ext_setting: Name of touch setting for the autotest
                                     extension, e.g. 'TapToClick'.
        @param value: True for enabled, False for disabled.

        """
        if self._has_inputcontrol:
            self._set_touch_setting_by_inputcontrol(inputcontrol_setting, value)
        elif self._autotest_ext is not None:
            self._autotest_ext.EvaluateJavaScript(
                    'chrome.autotestPrivate.set%s(%s);'
                    % (autotest_ext_setting, ("%s" % value).lower()))
            # TODO: remove this sleep once checking for value is available.
            time.sleep(1)
        else:
            raise error.TestFail('Both inputcontrol and the autotest '
                                 'extension are not availble.')


    def _set_australian_scrolling(self, value):
        """Set australian scrolling to the given value.

        @param value: True for enabled, False for disabled.

        """
        self._set_touch_setting('australian_scrolling', 'NaturalScroll', value)


    def _set_tap_to_click(self, value):
        """Set tap-to-click to the given value.

        @param value: True for enabled, False for disabled.

        """
        self._set_touch_setting('tapclick', 'TapToClick', value)


    def _set_tap_dragging(self, value):
        """Set tap dragging to the given value.

        @param value: True for enabled, False for disabled.

        """
        self._set_touch_setting('tapdrag', 'TapDragging', value)


    def _set_autotest_ext(self, ext):
        """Set the autotest extension.

        @ext: the autotest extension object.

        """
        self._autotest_ext = ext


    def _open_test_page(self, cr, filename='test_page.html'):
        """Prepare test page for testing.  Set self._tab with page.

        @param cr: chrome.Chrome() object
        @param filename: name of file in self.bindir to open

        """
        self._test_page = TestPage(cr, self.bindir, filename)
        self._tab = self._test_page._tab


    def _open_events_page(self, cr):
        """Open the test events page.  Set self._events with EventsPage class.

        Also set self._tab as this page and self.bindir as the http server dir.

        @param cr: chrome.Chrome() object

        """
        self._events = EventsPage(cr, self.bindir)
        self._tab = self._events._tab


    def _center_cursor(self):
        """Playback mouse movement to center cursor.

        Requres that self._emulate_mouse() has been called.

        """
        self.player.blocking_playback_of_default_file(
                'mouse_center_cursor_gesture', input_type='mouse')


    def _get_kernel_events_recorder(self, input_type):
        """Return a kernel event recording object for the given input type.

        @param input_type: device type, e.g. 'touchpad'

        @returns: KernelEventsRecorder instance.

        """
        node = self.player.devices[input_type].node
        return KernelEventsRecorder(node)


    def cleanup(self):
        self.player.close()


class KernelEventsRecorder(object):
    """Object to record kernel events for a particular device."""

    def __init__(self, node):
        """Setup to record future evtest output for this node.

        @param input_type: the device which to inspect, e.g. 'mouse'

        """
        self.node = node
        self.fh = tempfile.NamedTemporaryFile()
        self.evtest_process = None


    def start(self):
        """Start recording events."""
        self.evtest_process = subprocess.Popen(
                ['evtest', self.node], stdout=self.fh)

        # Wait until the initial output has finished before returning.
        def find_exit():
            """Polling function for end of output."""
            interrupt_cmd = ('grep "interrupt to exit" %s | wc -l' %
                             self.fh.name)
            line_count = utils.run(interrupt_cmd).stdout.strip()
            return line_count != '0'
        utils.poll_for_condition(find_exit)


    def clear(self):
        """Clear previous events."""
        self.stop()
        self.fh.close()
        self.fh = tempfile.NamedTemporaryFile()


    def stop(self):
        """Stop recording events."""
        if self.evtest_process:
            self.evtest_process.kill()
            self.evtest_process = None


    def get_recorded_events(self):
        """Get the evtest output since object was created."""
        self.fh.seek(0)
        events = self.fh.read()
        return events


    def log_recorded_events(self):
        """Save recorded events into logs."""
        events = self.get_recorded_events()
        logging.info('Kernel events seen:\n%s', events)


    def get_last_event_timestamp(self, filter_str=''):
        """Return the timestamp of the last event since recording started.

        Events are in the form "Event: time <epoch time>, <info>\n"

        @param filter_str: a regex string to match to the <info> section.

        @returns: floats matching

        """
        events = self.get_recorded_events()
        findall = re.findall(r' time (.*?), [^\n]*?%s' % filter_str,
                             events, re.MULTILINE)
        re.findall(r' time (.*?), [^\n]*?%s' % filter_str, events, re.MULTILINE)
        if not findall:
            self.log_recorded_events()
            raise error.TestError('Could not find any kernel timestamps!'
                                  '  Filter: %s' % filter_str)
        return float(findall[-1])


    def close(self):
        """Clean up this class."""
        self.stop()
        self.fh.close()


class TestPage(object):
    """Wrapper around a Telemtry tab for utility functions.

    Provides functions such as reload and setting scroll height on page.

    """
    _DEFAULT_SCROLL = 5000

    def __init__(self, cr, httpdir, filename):
        """Open a given test page in the given httpdir.

        @param cr: chrome.Chrome() object
        @param httpdir: the directory to use for SetHTTPServerDirectories
        @param filename: path to the file to open, relative to httpdir

        """
        cr.browser.platform.SetHTTPServerDirectories(httpdir)
        self._tab = cr.browser.tabs[0]
        self._tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(httpdir, filename)))
        self.wait_for_page_ready()


    def reload_page(self):
        """Reloads test page."""
        self._tab.Navigate(self._tab.url)
        self.wait_for_page_ready()


    def wait_for_page_ready(self):
        """Wait for a variable pageReady on the test page to be true.

        Presuposes that a pageReady variable exists on this page.

        @raises error.TestError if page is not ready after timeout.

        """
        self._tab.WaitForDocumentReadyStateToBeComplete()
        utils.poll_for_condition(
                lambda: self._tab.EvaluateJavaScript('pageReady'),
                exception=error.TestError('Test page is not ready!'))


    def expand_page(self):
        """Expand the page to be very large, to allow scrolling."""
        page_width = self._DEFAULT_SCROLL * 5
        cmd = 'document.body.style.%s = "%dpx"' % ('%s', page_width)
        self._tab.ExecuteJavaScript(cmd % 'width')
        self._tab.ExecuteJavaScript(cmd % 'height')


    def set_scroll_position(self, value, scroll_vertical=True):
        """Set scroll position to given value.

        @param value: integer value in pixels.
        @param scroll_vertical: True for vertical scroll,
                                False for horizontal Scroll.

        """
        cmd = 'window.scrollTo(%d, %d);'
        if scroll_vertical:
            self._tab.ExecuteJavaScript(cmd % (0, value))
        else:
            self._tab.ExecuteJavaScript(cmd % (value, 0))


    def set_default_scroll_position(self, scroll_vertical=True):
        """Set scroll position of page to default.

        @param scroll_vertical: True for vertical scroll,
                                False for horizontal Scroll.
        @raise: TestError if page is not set to default scroll position

        """
        total_tries = 2
        for i in xrange(total_tries):
            try:
                self.set_scroll_position(self._DEFAULT_SCROLL, scroll_vertical)
                self.wait_for_default_scroll_position(scroll_vertical)
            except error.TestError as e:
                if i == total_tries - 1:
                   pos = self.get_scroll_position(scroll_vertical)
                   logging.error('SCROLL POSITION: %s', pos)
                   raise e
                else:
                   self.expand_page()
            else:
                 break


    def get_scroll_position(self, scroll_vertical=True):
        """Return current scroll position of page.

        @param scroll_vertical: True for vertical scroll,
                                False for horizontal Scroll.

        """
        if scroll_vertical:
            return int(self._tab.EvaluateJavaScript('window.scrollY'))
        else:
            return int(self._tab.EvaluateJavaScript('window.scrollX'))


    def wait_for_default_scroll_position(self, scroll_vertical=True):
        """Wait for page to be at the default scroll position.

        @param scroll_vertical: True for vertical scroll,
                                False for horizontal scroll.

        @raise: TestError if page either does not move or does not stop moving.

        """
        utils.poll_for_condition(
                lambda: self.get_scroll_position(
                        scroll_vertical) == self._DEFAULT_SCROLL,
                exception=error.TestError('Page not set to default scroll!'))


    def wait_for_scroll_position_to_settle(self, scroll_vertical=True):
        """Wait for page to move and then stop moving.

        @param scroll_vertical: True for Vertical scroll and
                                False for horizontal scroll.

        @raise: TestError if page either does not move or does not stop moving.

        """
        # Wait until page starts moving.
        utils.poll_for_condition(
                lambda: self.get_scroll_position(
                        scroll_vertical) != self._DEFAULT_SCROLL,
                exception=error.TestError('No scrolling occurred!'), timeout=30)

        # Wait until page has stopped moving.
        self._previous = self._DEFAULT_SCROLL
        def _movement_stopped():
            current = self.get_scroll_position()
            result = current == self._previous
            self._previous = current
            return result

        utils.poll_for_condition(
                lambda: _movement_stopped(), sleep_interval=1,
                exception=error.TestError('Page did not stop moving!'),
                timeout=30)


    def get_page_width(self):
        """Return window.innerWidth for this page."""
        return int(self._tab.EvaluateJavaScript('window.innerWidth'))


class EventsPage(TestPage):
    """Functions to monitor input events on the DUT, as seen by a webpage.

    A subclass of TestPage which uses and interacts with a specific page.

    """
    def __init__(self, cr, httpdir):
        """Open the website and save the tab in self._tab.

        @param cr: chrome.Chrome() object
        @param httpdir: the directory to use for SetHTTPServerDirectories

        """
        filename = 'touch_events_test_page.html'
        current_dir = os.path.dirname(os.path.realpath(__file__))
        shutil.copyfile(os.path.join(current_dir, filename),
                        os.path.join(httpdir, filename))

        super(EventsPage, self).__init__(cr, httpdir, filename)


    def clear_previous_events(self):
        """Wipe the test page back to its original state."""
        self._tab.ExecuteJavaScript('pageReady = false')
        self._tab.ExecuteJavaScript('clearPreviousEvents()')
        self.wait_for_page_ready()


    def get_events_log(self):
        """Return the event log from the test page."""
        return self._tab.EvaluateJavaScript('eventLog')


    def log_events(self):
        """Put the test page's event log into logging.info."""
        logging.info('EVENTS LOG:')
        logging.info(self.get_events_log())


    def get_time_of_last_event(self):
        """Return the timestamp of the last seen event (if any)."""
        return self._tab.EvaluateJavaScript('timeOfLastEvent')


    def get_event_count(self):
        """Return the number of events that the test page has seen."""
        return self._tab.EvaluateJavaScript('eventCount')


    def get_scroll_delta(self, is_vertical):
        """Return the net scrolling the test page has seen.

        @param is_vertical: True for vertical scrolling; False for horizontal.

        """
        axis = 'y' if is_vertical else 'x'
        return self._tab.EvaluateJavaScript('netScrollDelta.%s' % axis)


    def get_click_count(self):
        """Return the number of clicks the test page has seen."""
        return self._tab.EvaluateJavaScript('clickCount')


    def wait_for_events_to_complete(self, delay_secs=1, timeout=60):
        """Wait until test page stops seeing events for delay_secs seconds.

        @param delay_secs: the polling frequency in seconds.
        @param timeout: the number of seconds to wait for events to complete.
        @raises: error.TestError if no events occurred.
        @raises: error.TestError if events did not stop after timeout seconds.

        """
        self._tmp_previous_event_count = -1
        def _events_stopped_coming():
            most_recent_event_count = self.get_event_count()
            delta = most_recent_event_count - self._tmp_previous_event_count
            self._tmp_previous_event_count = most_recent_event_count
            return most_recent_event_count != 0 and delta == 0

        try:
            utils.poll_for_condition(
                    _events_stopped_coming, exception=error.TestError(),
                    sleep_interval=delay_secs, timeout=timeout)
        except error.TestError:
            if self._tmp_previous_event_count == 0:
                raise error.TestError('No touch event was seen!')
            else:
                self.log_events()
                raise error.TestError('Touch events did not stop!')


    def set_prevent_defaults(self, value):
        """Set whether to allow default event actions to go through.

        E.g. if this is True, a two finger horizontal scroll will not actually
        produce history navigation on the browser.

        @param value: True for prevent defaults; False to allow them.

        """
        js_value = str(value).lower()
        self._tab.ExecuteJavaScript('preventDefaults = %s;' % js_value)
