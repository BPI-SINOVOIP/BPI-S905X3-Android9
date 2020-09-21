#!/usr/bin/env python
#
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script functions as a web app and wrapper for the cras_router program."""

import re
import subprocess
import logging
import cherrypy

# Node Format: [Stable_Id, ID, Vol, Plugged, L/R_swapped, Time, Type, Name]
ID_INDEX = 1
PLUGGED_INDEX = 3
TYPE_INDEX = 6
NAME_INDEX = 7


def get_plugged_nodes(plugged_nodes, lines, is_input):
  start_str = 'Input Nodes:' if is_input else 'Output Nodes:'
  end_str = 'Attached clients:' if is_input else 'Input Devices:'
  for i in range(lines.index(start_str) + 2,
                 lines.index(end_str)):
    node = filter(None, re.split(r'\s+|\*+', lines[i]))
    # check for nodes that are plugged nodes and loopback
    if node[PLUGGED_INDEX] == 'yes' and node[TYPE_INDEX][:4] != 'POST':
      key = node[TYPE_INDEX] + ' ' + node[NAME_INDEX]
      plugged_nodes[key] = node[ID_INDEX]


class CrasRouterTest(object):
  """Cherrypy class that builds and runs the HTML for audio testing tool."""

  @cherrypy.expose
  def index(self):
    """Builds up and displays the html for the audio testing tool.

    Returns:
      html that was built up based on plugged audio devices.
    """

    # Stop program if currently being run.
    if 'process' in cherrypy.session:
      print 'Existing process'
      # If return code is None process is still running
      if cherrypy.session['process'].poll() is None:
        print 'Killing existing process'
        cherrypy.session['process'].kill()
      else:
        print 'Process already finished'

    html = """<html>
              <head>
                <title>Audio Test</title>
              </head>
              <body style="background-color:lightgrey;">
                <font color="red">
                <h1>Audio Closed Loop Test</h1>
                <font style="color:rgb(100, 149, 237)">
                <h3>
                <form name="routerOptions" method="get"
                onsubmit="return validateForm()" action="start_test">
                  <h2>Input Type</h2>
              """
    dump = subprocess.check_output(['cras_test_client', '--dump_s'])
    if not dump:
      return 'Could not connect to server'
    dump_lines = dump.split('\n')
    input_plugged_nodes = {}
    get_plugged_nodes(input_plugged_nodes, dump_lines, True)
    for name, node_id in input_plugged_nodes.items():
      line = '<input type ="radio" name="input_type" value="'
      line += node_id + '">' +name + '<br>\n'
      html += line

    html += """<input type ="radio" id="i0" name="input_type"
               value="file">File<br>
                 <div id="input_file" style="display:none;">
                 Filename <input type="text" name="input_file"><br><br>
                 </div>
               <h2>Output Type</h2>"""
    output_plugged_nodes = {}
    get_plugged_nodes(output_plugged_nodes, dump_lines, False)
    for name, node_id in output_plugged_nodes.items():
      line = '<input type ="radio" name="output_type" value="'
      line = line + node_id + '">' +name + '<br>\n'
      html += line

    html += """<input type ="radio" name="output_type"
               value="file">File<br>
               <div id="output_file" style="display:none;">
               Filename <input type="text" name="output_file">
               </div><br>
               <h2>Sample Rate</h2>
               <input type="radio" name="rate" id="sample_rate1" value=48000
               checked>48,000 Hz<br>
               <input type="radio" name="rate" id="sample_rate0" value=44100>
               44,100 Hz<br><br>
               <button type="submit" onclick="onOff(this)">Test!</button>
               </h3>
               </form>
               </font>
               </body>
               </html>
               """
    javascript = """
                 <script>
                 /* Does basic error checking to make sure user doesn't
                  * give bad options to the router.
                  */
                 function validateForm(){
                    var input_type =
                        document.forms['routerOptions']['input_type'].value;
                    var output_type =
                        document.forms['routerOptions']['output_type'].value;
                    if (input_type == '' || output_type == '') {
                        alert('Please select an input and output type.');
                        return false;
                    }
                    if (input_type == 'file' && output_type == 'file') {
                        alert('Input and Output Types cannot both be files!');
                        return false;
                    }
                    //check if filename is valid
                    if (input_type == 'file') {
                        var input_file =
                          document.forms['routerOptions']['input_file'].value;
                        if (input_file == '') {
                            alert('Please enter a file name');
                            return false;
                        }
                    }
                    if (output_type == 'file') {
                        var output_file =
                          document.forms['routerOptions']['output_file'].value;
                        if (output_file == '') {
                            alert('Please enter a file name');
                            return false;
                        }
                    }
                }

                function show_filename(radio, file_elem) {
                    for(var i =0; i < radio.length; i++){
                        radio[i].onclick = function(){
                            if (this.value == 'file') {
                                file_elem.style.display = 'block';
                            } else {
                                file_elem.style.display = 'none';
                            }
                        }
                    }
                }
                /* Loops determine if filename field should be shown */
                var input_type_rad =
                    document.forms['routerOptions']['input_type'];
                var input_file_elem =
                    document.getElementById('input_file');
                var output_type_rad =
                    document.forms['routerOptions']['output_type'];
                var output_file_elem =
                    document.getElementById('output_file');
                show_filename(input_type_rad, input_file_elem);
                show_filename(output_type_rad, output_file_elem);
                </script>"""
    html += javascript
    return html

  @cherrypy.expose
  def start_test(self, input_type, output_type, input_file='',
                 output_file='', rate=48000):
    """Capture audio from the input_type and plays it back to the output_type.

    Args:
      input_type: Node id for the selected input or 'file' for files
      output_type: Node id for the selected output or 'file' for files
      input_file: Path of the input if 'file' is input type
      output_file: Path of the output if 'file' is output type
      rate: Sample rate for the test.

    Returns:
      html for the tesing in progress page.
    """
    print 'Beginning test'
    if input_type == 'file' or output_type == 'file':
        command = ['cras_test_client']
    else:
        command = ['cras_router']
    if input_type == 'file':
      command.append('--playback_file')
      command.append(str(input_file))
    else:
      set_input = ['cras_test_client', '--select_input', str(input_type)]
      if subprocess.check_call(set_input):
        print 'Error setting input'
    if output_type == 'file':
      command.append('--capture_file')
      command.append(str(output_file))
    else:
      set_output = ['cras_test_client', '--select_output', str(output_type)]
      if subprocess.check_call(set_output):
        print 'Error setting output'
    command.append('--rate')
    command.append(str(rate))
    print 'Running commmand: ' + str(command)
    p = subprocess.Popen(command)
    cherrypy.session['process'] = p
    return """
    <html>
    <head>
    <style type="text/css">
    body {
      background-color: #DC143C;
    }
    #test {
      color: white;
      text-align: center;
    }
    </style>
      <title>Running test</title>
    </head>
    <body>
      <div id="test">
        <h1>Test in progress</h1>
        <form action="index"><!--Go back to orginal page-->
          <button type="submit" id="stop">Click to stop</button>
        </form>
        <h2>Time Elapsed<br>
            <time id="elapsed_time">00:00</time>
        </h2>
        </div>
    </body>
    </html>
    <script type="text/javascript">
    var seconds = 0;
    var minutes = 0;
    var elapsed_time;
    var start_time = new Date().getTime();
    function secondPassed(){
      var time = new Date().getTime() - start_time;
      elapsed_time = Math.floor(time / 100) / 10;
      minutes = Math.floor(elapsed_time / 60);
      seconds = Math.floor(elapsed_time % 60);
      var seconds_str = (seconds < 10 ? '0' + seconds: '' + seconds);
      var minutes_str = (minutes < 10 ? '0' + minutes: '' + minutes);
      var time_passed = minutes_str + ':' + seconds_str;
      document.getElementById("elapsed_time").textContent = time_passed;
    }
    //have time tic every second
    var timer = setInterval(secondPassed, 1000);
    var stop = document.getElementById("stop");
    stop.onclick = function(){
      seconds = 0;
      minutes = 0;
      clearInterval(timer);
    }
    </script>"""

if __name__ == '__main__':
  conf = {
      '/': {
          'tools.sessions.on': True
      }
  }
  cherrypy.quickstart(CrasRouterTest(), '/', conf)
