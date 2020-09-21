# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This is a utility to build an html page based on the directory summaries
collected during the test.
"""

import os
import re

import common
from autotest_lib.client.bin.result_tools import utils_lib
from autotest_lib.client.common_lib import global_config


CONFIG = global_config.global_config
# Base url to open a file from Google Storage
GS_FILE_BASE_URL = CONFIG.get_config_value('CROS', 'gs_file_base_url')

# Default width of `size_trimmed_width`. If throttle is not applied, the block
# of `size_trimmed_width` will be set to minimum to make the view more compact.
DEFAULT_SIZE_TRIMMED_WIDTH = 50

DEFAULT_RESULT_SUMMARY_NAME = 'result_summary.html'

DIR_SUMMARY_PATTERN = 'dir_summary_\d+.json'

# ==================================================
# Following are key names used in the html templates:

CSS = 'css'
DIRS = 'dirs'
GS_FILE_BASE_URL_KEY = 'gs_file_base_url'
INDENTATION_KEY = 'indentation'
JAVASCRIPT = 'javascript'
JOB_DIR = 'job_dir'
NAME = 'name'
PATH = 'path'

SIZE_CLIENT_COLLECTED = 'size_client_collected'

SIZE_INFO = 'size_info'
SIZE_ORIGINAL = 'size_original'
SIZE_PERCENT = 'size_percent'
SIZE_PERCENT_CLASS = 'size_percent_class'
SIZE_PERCENT_CLASS_REGULAR = 'size_percent'
SIZE_PERCENT_CLASS_TOP = 'top_size_percent'
SIZE_SUMMARY = 'size_summary'
SIZE_TRIMMED = 'size_trimmed'

# Width of `size_trimmed` block`
SIZE_TRIMMED_WIDTH = 'size_trimmed_width'

SUBDIRS = 'subdirs'
SUMMARY_TREE = 'summary_tree'
# ==================================================

# Text to show when test result is not throttled.
NOT_THROTTLED = '(Not throttled)'


PAGE_TEMPLATE = """
<!DOCTYPE html>
  <html>
    <body onload="init()">
      <h3>Summary of test results</h3>
%(size_summary)s
      <p>
      <b>
        Display format of a file or directory:
      </b>
      </p>
      <p>
        <span class="size_percent" style="width:auto">
          [percentage of size in the parent directory]
        </span>
        <span class="size_original" style="width:auto">
          [original size]
        </span>
        <span class="size_trimmed" style="width:auto">
          [size after throttling (empty if not throttled)]
        </span>
        [file name (<strike>strikethrough</strike> if file was deleted due to
            throttling)]
      </p>

      <button onclick="expandAll();">Expand All</button>
      <button onclick="collapseAll();">Collapse All</button>

%(summary_tree)s

%(css)s
%(javascript)s

    </body>
</html>
"""

CSS_TEMPLATE = """
<style>
  body {
      font-family: Arial;
  }

  td.table_header {
      font-weight: normal;
  }

  span.size_percent {
      color: #e8773e;
      display: inline-block;
      font-size: 75%%;
      text-align: right;
      width: 35px;
  }

  span.top_size_percent {
      color: #e8773e;
      background-color: yellow;
      display: inline-block;
      font-size: 75%%;
      fount-weight: bold;
      text-align: right;
      width: 35px;
  }

  span.size_original {
      color: sienna;
      display: inline-block;
      font-size: 75%%;
      text-align: right;
      width: 50px;
  }

  span.size_trimmed {
      color: green;
      display: inline-block;
      font-size: 75%%;
      text-align: right;
      width: %(size_trimmed_width)dpx;
  }

  ul.tree li {
      list-style-type: none;
      position: relative;
  }

  ul.tree li ul {
      display: none;
  }

  ul.tree li.open > ul {
      display: block;
  }

  ul.tree li a {
    color: black;
    text-decoration: none;
  }

  ul.tree li a.file {
    color: blue;
    text-decoration: underline;
  }

  ul.tree li a:before {
      height: 1em;
      padding:0 .1em;
      font-size: .8em;
      display: block;
      position: absolute;
      left: -1.3em;
      top: .2em;
  }

  ul.tree li > a:not(:last-child):before {
      content: '+';
  }

  ul.tree li.open > a:not(:last-child):before {
      content: '-';
  }
</style>
"""

JAVASCRIPT_TEMPLATE = """
<script>
function init() {
    var tree = document.querySelectorAll('ul.tree a:not(:last-child)');
    for(var i = 0; i < tree.length; i++){
        tree[i].addEventListener('click', function(e) {
            var parent = e.target.parentElement;
            var classList = parent.classList;
            if(classList.contains("open")) {
                classList.remove('open');
                var opensubs = parent.querySelectorAll(':scope .open');
                for(var i = 0; i < opensubs.length; i++){
                    opensubs[i].classList.remove('open');
                }
            } else {
                classList.add('open');
            }
        });
    }
}

function expandAll() {
    var tree = document.querySelectorAll('ul.tree a:not(:last-child)');
    for(var i = 0; i < tree.length; i++){
        var classList = tree[i].parentElement.classList;
        if(classList.contains("close")) {
            classList.remove('close');
        }
        classList.add('open');
    }
}

function collapseAll() {
    var tree = document.querySelectorAll('ul.tree a:not(:last-child)');
    for(var i = 0; i < tree.length; i++){
        var classList = tree[i].parentElement.classList;
        if(classList.contains("open")) {
            classList.remove('open');
        }
        classList.add('close');
    }
}

// If the current url has `gs_url`, it means the file is opened from Google
// Storage.
var gs_url = 'apidata.googleusercontent.com';
// Base url to open a file from Google Storage
var gs_file_base_url = '%(gs_file_base_url)s'
// Path to the result.
var job_dir = '%(job_dir)s'

function openFile(path) {
    if(window.location.href.includes(gs_url)) {
        url = gs_file_base_url + job_dir + '/' + path.substring(3);
    } else {
        url = window.location.href + '/' + path;
    }
    window.open(url, '_blank');
}
</script>
"""

SIZE_SUMMARY_TEMPLATE = """
<table>
  <tr>
    <td class="table_header">Results collected from test device: </td>
    <td><span>%(size_client_collected)s</span> </td>
  </tr>
  <tr>
    <td class="table_header">Original size of test results:</td>
    <td>
      <span class="size_original" style="font-size:100%%;width:auto">
        %(size_original)s
      </span>
    </td>
  </tr>
  <tr>
    <td class="table_header">Size of test results after throttling:</td>
    <td>
      <span class="size_trimmed" style="font-size:100%%;width:auto">
        %(size_trimmed)s
      </span>
    </td>
  </tr>
</table>
"""

SIZE_INFO_TEMPLATE = """
%(indentation)s<span class="%(size_percent_class)s">%(size_percent)s</span>
%(indentation)s<span class="size_original">%(size_original)s</span>
%(indentation)s<span class="size_trimmed">%(size_trimmed)s</span> """

FILE_ENTRY_TEMPLATE = """
%(indentation)s<li>
%(indentation)s\t<div>
%(size_info)s
%(indentation)s\t\t<a class="file" href="javascript:openFile('%(path)s');" >
%(indentation)s\t\t\t%(name)s
%(indentation)s\t\t</a>
%(indentation)s\t</div>
%(indentation)s</li>"""

DELETED_FILE_ENTRY_TEMPLATE = """
%(indentation)s<li>
%(indentation)s\t<div>
%(size_info)s
%(indentation)s\t\t<strike>%(name)s</strike>
%(indentation)s\t</div>
%(indentation)s</li>"""

DIR_ENTRY_TEMPLATE = """
%(indentation)s<li><a>%(size_info)s %(name)s</a>
%(subdirs)s
%(indentation)s</li>"""

SUBDIRS_WRAPPER_TEMPLATE = """
%(indentation)s<ul class="tree">
%(dirs)s
%(indentation)s</ul>"""

INDENTATION = '\t'

def _get_size_percent(size_original, total_bytes):
    """Get the percentage of file size in the parent directory before throttled.

    @param size_original: Original size of the file, in bytes.
    @param total_bytes: Total size of all files under the parent directory, in
            bytes.
    @return: A formatted string of the percentage of file size in the parent
            directory before throttled.
    """
    if total_bytes == 0:
        return '0%'
    return '%.1f%%' % (100*float(size_original)/total_bytes)


def _get_dirs_html(dirs, parent_path, total_bytes, indentation):
    """Get the html string for the given directory.

    @param dirs: A list of ResultInfo.
    @param parent_path: Path to the parent directory.
    @param total_bytes: Total of the original size of files in the given
            directories in bytes.
    @param indentation: Indentation to be used for the html.
    """
    if not dirs:
        return ''
    summary_html = ''
    top_size_limit = max([entry.original_size for entry in dirs])
    # A map between file name to ResultInfo that contains the summary of the
    # file.
    entries = dict((entry.keys()[0], entry) for entry in dirs)
    for name in sorted(entries.keys()):
        entry = entries[name]
        if not entry.is_dir and re.match(DIR_SUMMARY_PATTERN, name):
            # Do not include directory summary json files in the html, as they
            # will be deleted.
            continue

        size_data = {SIZE_PERCENT: _get_size_percent(entry.original_size,
                                                     total_bytes),
                     SIZE_ORIGINAL:
                        utils_lib.get_size_string(entry.original_size),
                     SIZE_TRIMMED:
                        utils_lib.get_size_string(entry.trimmed_size),
                     INDENTATION_KEY: indentation + 2*INDENTATION}
        if entry.original_size < top_size_limit:
            size_data[SIZE_PERCENT_CLASS] = SIZE_PERCENT_CLASS_REGULAR
        else:
            size_data[SIZE_PERCENT_CLASS] = SIZE_PERCENT_CLASS_TOP
        if entry.trimmed_size == entry.original_size:
            size_data[SIZE_TRIMMED] = ''

        entry_path = '%s/%s' % (parent_path, name)
        if not entry.is_dir:
            # This is a file
            data = {NAME: name,
                    PATH: entry_path,
                    SIZE_INFO: SIZE_INFO_TEMPLATE % size_data,
                    INDENTATION_KEY: indentation}
            if entry.original_size > 0 and entry.trimmed_size == 0:
                summary_html += DELETED_FILE_ENTRY_TEMPLATE % data
            else:
                summary_html += FILE_ENTRY_TEMPLATE % data
        else:
            subdir_total_size = entry.original_size
            sub_indentation = indentation + INDENTATION
            subdirs_html = (
                    SUBDIRS_WRAPPER_TEMPLATE %
                    {DIRS: _get_dirs_html(
                            entry.files, entry_path, subdir_total_size,
                            sub_indentation),
                     INDENTATION_KEY: indentation})
            data = {NAME: entry.name,
                    SIZE_INFO: SIZE_INFO_TEMPLATE % size_data,
                    SUBDIRS: subdirs_html,
                    INDENTATION_KEY: indentation}
            summary_html += DIR_ENTRY_TEMPLATE % data
    return summary_html


def build(client_collected_bytes, summary, html_file):
    """Generate an HTML file to visualize the given directory summary.

    @param client_collected_bytes: The total size of results collected from
            the DUT. The number can be larger than the total file size of the
            given path, as files can be overwritten or removed.
    @param summary: A ResultInfo instance containing the directory summary.
    @param html_file: Path to save the html file to.
    """
    size_original = summary.original_size
    size_trimmed = summary.trimmed_size
    size_summary_data = {SIZE_CLIENT_COLLECTED:
                             utils_lib.get_size_string(client_collected_bytes),
                         SIZE_ORIGINAL:
                             utils_lib.get_size_string(size_original),
                         SIZE_TRIMMED:
                             utils_lib.get_size_string(size_trimmed)}
    size_trimmed_width = DEFAULT_SIZE_TRIMMED_WIDTH
    if size_original == size_trimmed:
        size_summary_data[SIZE_TRIMMED] = NOT_THROTTLED
        size_trimmed_width = 0

    size_summary = SIZE_SUMMARY_TEMPLATE % size_summary_data

    indentation = INDENTATION
    dirs_html = _get_dirs_html(
            summary.files, '..', size_original, indentation + INDENTATION)
    summary_tree = SUBDIRS_WRAPPER_TEMPLATE % {DIRS: dirs_html,
                                               INDENTATION_KEY: indentation}

    # job_dir is the path between Autotest `results` folder and the summary html
    # file, e.g., 123-debug_user/host1. Assume it always contains 2 levels.
    job_dir_sections = html_file.split(os.sep)[:-1]
    try:
        job_dir = '/'.join(job_dir_sections[
                (job_dir_sections.index('results')+1):])
    except ValueError:
        # 'results' is not in the path, default to two levels up of the summary
        # file.
        job_dir = '/'.join(job_dir_sections[-2:])

    javascript = (JAVASCRIPT_TEMPLATE %
                  {GS_FILE_BASE_URL_KEY: GS_FILE_BASE_URL,
                   JOB_DIR: job_dir})
    css = CSS_TEMPLATE % {SIZE_TRIMMED_WIDTH: size_trimmed_width}
    html = PAGE_TEMPLATE % {SIZE_SUMMARY: size_summary,
                            SUMMARY_TREE: summary_tree,
                            CSS: css,
                            JAVASCRIPT: javascript}
    with open(html_file, 'w') as f:
        f.write(html)
