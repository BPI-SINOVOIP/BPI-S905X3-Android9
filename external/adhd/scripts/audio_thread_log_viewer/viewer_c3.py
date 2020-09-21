#!/usr/bin/python
#
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""Generates an HTML file with plot of buffer level in the audio thread log."""

import argparse
import collections
import logging
import string

page_content = string.Template("""
<html meta charset="UTF8">
<head>
  <!-- Load c3.css -->
  <link href="https://rawgit.com/masayuki0812/c3/master/c3.css" rel="stylesheet" type="text/css">
  <!-- Load d3.js and c3.js -->
  <script src="https://d3js.org/d3.v3.min.js" charset="utf-8"></script>
  <script src="https://rawgit.com/masayuki0812/c3/master/c3.js" charset="utf-8"></script>
  <style type="text/css">
    .c3-grid text {
        fill: grey;
    }
    .event_log_box {
      font-family: 'Courier New', Courier, 'Lucida Sans Typewriter', 'Lucida Typewriter', monospace;
      font-size: 20px;
      font-style: normal;
      font-variant: normal;
      font-weight: 300;
      line-height: 26.4px;
      white-space: pre;
      height:50%;
      width:48%;
      border:1px solid #ccc;
      overflow:auto;
    }
    .checkbox {
      font-size: 30px;
      border: 2px;
    }
    .device {
      font-size: 15px;
    }
    .stream{
      font-size: 15px;
    }
    .fetch{
    }
    .wake{
    }
  </style>
  <script type="text/javascript">
    draw_chart = function() {
      var chart = c3.generate({
        data: {
          x: 'time',
          columns: [
              ['time',                   $times],
              ['buffer_level',           $buffer_levels],
          ],
          type: 'bar',
          types: {
              buffer_level: 'line',
          },
        },
        zoom: {
          enabled: true,
        },

        grid: {
          x: {
            lines: [
              $grids,
            ],
          },
        },

        axis: {
          y: {min: 0, max: $max_y},
        },
      });
    };

    logs = `$logs`;
    put_logs = function () {
      document.getElementById('logs').innerHTML = logs;
    };

    set_initial_checkbox_value = function () {
      document.getElementById('device').checked = true;
      document.getElementById('stream').checked = true;
      document.getElementById('fetch').checked = true;
      document.getElementById('wake').checked = true;
    }

    window.onload = function() {
      draw_chart();
      put_logs();
      set_initial_checkbox_value();
    };

    function handleClick(checkbox) {
      var class_name = checkbox.id;
      var elements = document.getElementsByClassName(class_name);
      var i;

      if (checkbox.checked) {
        display_value = "block";
      } else {
        display_value = "none"
      }

      console.log("change " + class_name + " to " + display_value);
      for (i = 0; i < elements.length; i++) {
        elements[i].style.display = display_value;
      }
    }

  </script>
</head>

<body>
  <div id="chart" style="height:50%; width:100%" ></div>
  <div style="margin:0 auto"; class="checkbox">
      <label><input type="checkbox" onclick="handleClick(this);" id="device">Show device removed/added event</label>
      <label><input type="checkbox" onclick="handleClick(this);" id="stream">Show stream removed/added event</label>
      <label><input type="checkbox" onclick="handleClick(this);" id="fetch">Show fetch event</label>
      <label><input type="checkbox" onclick="handleClick(this);" id="wake">Show wake by num_fds=1 event</label>
  </div>
  <div class="event_log_box", id="logs", style="float:left;"></div>
  <textarea class="event_log_box", id="text", style="float:right;"></textarea>
</body>
</html>
""")


Tag = collections.namedtuple('Tag', {'time', 'text', 'position', 'class_name'})
"""
The tuple for tags shown on the plot on certain time.
text is the tag to show, position is the tag position, which is one of
'start', 'middle', 'end', class_name is one of 'device', 'stream', 'fetch',
and 'wake' which will be their CSS class name.
"""

class EventData(object):
    """The base class of an event."""
    def __init__(self, time, name):
        """Initializes an EventData.

        @param time: A string for event time.
        @param name: A string for event name.

        """
        self.time = time
        self.name = name
        self._text = None
        self._position = None
        self._class_name = None

    def GetTag(self):
        """Gets the tag for this event.

        @returns: A Tag object. Returns None if no need to show tag.

        """
        if self._text:
            return Tag(
                    time=self.time, text=self._text, position=self._position,
                    class_name=self._class_name)
        return None


class DeviceEvent(EventData):
    """Class for device event."""
    def __init__(self, time, name, device):
        """Initializes a DeviceEvent.

        @param time: A string for event time.
        @param name: A string for event name.
        @param device: A string for device index.

        """
        super(DeviceEvent, self).__init__(time, name)
        self.device = device
        self._position = 'start'
        self._class_name = 'device'


class DeviceRemovedEvent(DeviceEvent):
    """Class for device removed event."""
    def __init__(self, time, name, device):
        """Initializes a DeviceRemovedEvent.

        @param time: A string for event time.
        @param name: A string for event name.
        @param device: A string for device index.

        """
        super(DeviceRemovedEvent, self).__init__(time, name, device)
        self._text = 'Removed Device %s' % self.device


class DeviceAddedEvent(DeviceEvent):
    """Class for device added event."""
    def __init__(self, time, name, device):
        """Initializes a DeviceAddedEvent.

        @param time: A string for event time.
        @param name: A string for event name.
        @param device: A string for device index.

        """
        super(DeviceAddedEvent, self).__init__(time, name, device)
        self._text = 'Added Device %s' % self.device


class LevelEvent(DeviceEvent):
    """Class for device event with buffer level."""
    def __init__(self, time, name, device, level):
        """Initializes a LevelEvent.

        @param time: A string for event time.
        @param name: A string for event name.
        @param device: A string for device index.
        @param level: An int for buffer level.

        """
        super(LevelEvent, self).__init__(time, name, device)
        self.level = level


class StreamEvent(EventData):
    """Class for event with stream."""
    def __init__(self, time, name, stream):
        """Initializes a StreamEvent.

        @param time: A string for event time.
        @param name: A string for event name.
        @param stream: A string for stream id.

        """
        super(StreamEvent, self).__init__(time, name)
        self.stream = stream
        self._class_name = 'stream'


class FetchStreamEvent(StreamEvent):
    """Class for stream fetch event."""
    def __init__(self, time, name, stream):
        """Initializes a FetchStreamEvent.

        @param time: A string for event time.
        @param name: A string for event name.
        @param stream: A string for stream id.

        """
        super(FetchStreamEvent, self).__init__(time, name, stream)
        self._text = 'Fetch %s' % self.stream
        self._position = 'end'
        self._class_name = 'fetch'


class StreamAddedEvent(StreamEvent):
    """Class for stream added event."""
    def __init__(self, time, name, stream):
        """Initializes a StreamAddedEvent.

        @param time: A string for event time.
        @param name: A string for event name.
        @param stream: A string for stream id.

        """
        super(StreamAddedEvent, self).__init__(time, name, stream)
        self._text = 'Add stream %s' % self.stream
        self._position = 'middle'


class StreamRemovedEvent(StreamEvent):
    """Class for stream removed event."""
    def __init__(self, time, name, stream):
        """Initializes a StreamRemovedEvent.

        @param time: A string for event time.
        @param name: A string for event name.
        @param stream: A string for stream id.

        """
        super(StreamRemovedEvent, self).__init__(time, name, stream)
        self._text = 'Remove stream %s' % self.stream
        self._position = 'middle'


class WakeEvent(EventData):
    """Class for wake event."""
    def __init__(self, time, name, num_fds):
        """Initializes a WakeEvent.

        @param time: A string for event time.
        @param name: A string for event name.
        @param num_fds: A string for number of fd that wakes audio thread up.

        """
        super(WakeEvent, self).__init__(time, name)
        self._position = 'middle'
        self._class_name = 'wake'
        if num_fds != '0':
            self._text = 'num_fds %s' % num_fds


class C3LogWriter(object):
    """Class to handle event data and fill an HTML page using c3.js library"""
    def __init__(self):
        """Initializes a C3LogWriter."""
        self.times = []
        self.buffer_levels = []
        self.tags = []
        self.max_y = 0

    def AddEvent(self, event):
        """Digests an event.

        Add a tag if this event needs to be shown on grid.
        Add a buffer level data into buffer_levels if this event has buffer
        level.

        @param event: An EventData object.

        """
        tag = event.GetTag()
        if tag:
            self.tags.append(tag)

        if isinstance(event, LevelEvent):
            self.times.append(event.time)
            self.buffer_levels.append(str(event.level))
            if event.level > self.max_y:
                self.max_y = event.level
            logging.debug('add data for a level event %s: %s',
                          event.time, event.level)

        if (isinstance(event, DeviceAddedEvent) or
            isinstance(event, DeviceRemovedEvent)):
            self.times.append(event.time)
            self.buffer_levels.append('null')

    def _GetGrids(self):
        """Gets the content to be filled for grids.

        @returns: A str for grid with format:
           '{value: time1, text: "tag1", position: "position1"},
            {value: time1, text: "tag1"},...'

        """
        grids = []
        for tag in self.tags:
            content = ('{value: %s, text: "%s", position: "%s", '
                       'class: "%s"}') % (
                              tag.time, tag.text, tag.position, tag.class_name)
            grids.append(content)
        grids_joined = ', '.join(grids)
        return grids_joined

    def FillPage(self, page_template):
        """Fills in the page template with content.

        @param page_template: A string for HTML page content with variables
                              to be filled.

        @returns: A string for filled page.

        """
        times = ', '.join(self.times)
        buffer_levels = ', '.join(self.buffer_levels)
        grids = self._GetGrids()
        filled = page_template.safe_substitute(
                times=times,
                buffer_levels=buffer_levels,
                grids=grids,
                max_y=str(self.max_y))
        return filled


class EventLogParser(object):
    """Class for event log parser."""
    def __init__(self):
        """Initializes an EventLogParse."""
        self.parsed_events = []

    def AddEventLog(self, event_log):
        """Digests a line of event log.

        @param event_log: A line for event log.

        """
        event = self._ParseOneLine(event_log)
        if event:
            self.parsed_events.append(event)

    def GetParsedEvents(self):
        """Gets the list of parsed events.

        @returns: A list of parsed EventData.

        """
        return self.parsed_events

    def _ParseOneLine(self, line):
        """Parses one line of event log.

        Split a line like
        169536.504763588  WRITE_STREAMS_FETCH_STREAM     id:0 cbth:512 delay:1136
        into time, name, and props where
        time = '169536.504763588'
        name = 'WRITE_STREAMS_FETCH_STREAM'
        props = {
            'id': 0,
            'cb_th': 512,
            'delay': 1136
        }

        @param line: A line of event log.

        @returns: A EventData object.

        """
        line_split = line.split()
        time, name = line_split[0], line_split[1]
        logging.debug('time: %s, name: %s', time, name)
        props = {}
        for index in xrange(2, len(line_split)):
            key, value = line_split[index].split(':')
            props[key] = value
        logging.debug('props: %s', props)
        return self._CreateEventData(time, name, props)

    def _CreateEventData(self, time, name, props):
        """Creates an EventData based on event name.

        @param time: A string for event time.
        @param name: A string for event name.
        @param props: A dict for event properties.

        @returns: A EventData object.

        """
        if name == 'WRITE_STREAMS_FETCH_STREAM':
            return FetchStreamEvent(time, name, stream=props['id'])
        if name == 'STREAM_ADDED':
            return StreamAddedEvent(time, name, stream=props['id'])
        if name == 'STREAM_REMOVED':
            return StreamRemovedEvent(time, name, stream=props['id'])
        if name in ['FILL_AUDIO', 'SET_DEV_WAKE']:
            return LevelEvent(
                    time, name, device=props['dev'],
                    level=int(props['hw_level']))
        if name == 'DEV_ADDED':
            return DeviceAddedEvent(time, name, device=props['dev'])
        if name == 'DEV_REMOVED':
            return DeviceRemovedEvent(time, name, device=props['dev'])
        if name == 'WAKE':
            return WakeEvent(time, name, num_fds=props['num_fds'])
        return None


class AudioThreadLogParser(object):
    """Class for audio thread log parser."""
    def __init__(self, path):
        """Initializes an AudioThreadLogParser.

        @param path: The audio thread log file path.

        """
        self.path = path
        self.content = None

    def Parse(self):
        """Prases the audio thread logs.

        @returns: A list of event log lines.

        """
        logging.debug('Using file: %s', self.path)
        with open(self.path, 'r') as f:
            self.content = f.read().splitlines()

        # Event logs starting at two lines after 'Audio Thread Event Log'.
        index_start = self.content.index('Audio Thread Event Log:') + 2
        # If input is from audio_diagnostic result, use aplay -l line to find
        # the end of audio thread event logs.
        try:
            index_end = self.content.index('=== aplay -l ===')
        except ValueError:
            logging.debug(
                    'Can not find aplay line. This is not from diagnostic')
            index_end = len(self.content)
        event_logs = self.content[index_start:index_end]
        logging.info('Parsed %s log events', len(event_logs))
        return event_logs

    def FillLogs(self, page_template):
        """Fills the HTML page template with contents for audio thread logs.

        @param page_template: A string for HTML page content with log variable
                              to be filled.

        @returns: A string for filled page.

        """
        logs = '\n<br>'.join(self.content)
        return page_template.substitute(logs=logs)


def ParseArgs():
    """Parses the arguments.

    @returns: The namespace containing parsed arguments.

    """
    parser = argparse.ArgumentParser(
            description='Draw time chart from audio thread log',
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('FILE', type=str, help='The audio thread log file')
    parser.add_argument('-o', type=str, dest='output',
                        default='view.html', help='The output HTML file')
    parser.add_argument('-d', dest='debug', action='store_true',
                        default=False, help='Show debug message')
    return parser.parse_args()


def Main():
    """The Main program."""
    options = ParseArgs()
    logging.basicConfig(
            format='%(asctime)s:%(levelname)s:%(message)s',
            level=logging.DEBUG if options.debug else logging.INFO)

    # Gets lines of event logs.
    audio_thread_log_parser = AudioThreadLogParser(options.FILE)
    event_logs = audio_thread_log_parser.Parse()

    # Parses event logs into events.
    event_log_parser = EventLogParser()
    for event_log in event_logs:
        event_log_parser.AddEventLog(event_log)
    events = event_log_parser.GetParsedEvents()

    # Reads in events in preparation of filling HTML template.
    c3_writer = C3LogWriter()
    for event in events:
        c3_writer.AddEvent(event)

    # Fills in buffer level chart.
    page_content_with_chart = c3_writer.FillPage(page_content)

    # Fills in audio thread log into text box.
    page_content_with_chart_and_logs = audio_thread_log_parser.FillLogs(
            string.Template(page_content_with_chart))

    with open(options.output, 'w') as f:
        f.write(page_content_with_chart_and_logs)


if __name__ == '__main__':
    Main()
