# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import tempfile


HTML_TEMPLATE = """<!DOCTYPE html>
    <html>
        <body id="body">
            <h1>{title}</h1>
            <p>Status: <span id="status">not-started</span></p>
            {scripts}
        </body>
    </html>
    """


def generate_test_html(title, *scripts):
    """
    Generates HTML contents for WebRTC tests.

    @param title: The title of the page.
    @param scripts: Paths to the javascript files to include.
    @returns HTML contents.
    """
    script_tag_list = [
        '<script src="{}"></script>'.format(script)
        for script in scripts
    ]
    script_tags = '\n'.join(script_tag_list)
    return HTML_TEMPLATE.format(title=title, scripts=script_tags)


def get_common_script_path(script):
    """
    Gets the file path to a common script.

    @param script: The file name of the script, e.g. 'foo.js'
    @returns The absolute path to the script.
    """
    return os.path.join(
            os.path.dirname(__file__), 'webrtc_scripts', script)


def create_temp_html_file(title, tmpdir, *scripts):
    """
    Creates a temporary file with HTML contents for WebRTC tests.

    @param title: The title of the page.
    @param tmpdir: Directory to put the temporary file.
    @param scripts: Paths to the javascript files to load.
    @returns The file object containing the HTML.
    """
    html = generate_test_html(
            title, *scripts)
    html_file = tempfile.NamedTemporaryFile(
            suffix = '.html', dir = tmpdir, delete = False)
    html_file.write(html)
    html_file.close()
    return html_file

