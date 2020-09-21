# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import urllib2

# HWID info types to request.
HWID_INFO_LABEL = 'dutlabel'
HWID_INFO_BOM = 'bom'
HWID_INFO_SKU = 'sku'
HWID_INFO_TYPES = [HWID_INFO_BOM, HWID_INFO_SKU, HWID_INFO_LABEL]

# HWID url vars.
HWID_VERSION = 'v1'
HWID_BASE_URL = 'https://www.googleapis.com/chromeoshwid'
URL_FORMAT_STRING='%(base_url)s/%(version)s/%(info_type)s/%(hwid)s/?key=%(key)s'

# Key file name to use when we don't want hwid labels.
KEY_FILENAME_NO_HWID = 'no_hwid_labels'

class HwIdException(Exception):
    """Raised whenever anything fails in the hwid info request."""


def get_hwid_info(hwid, info_type, key_file):
    """Given a hwid and info type, return a dict of the requested info.

    @param hwid: hwid to use for the query.
    @param info_type: String of info type requested.
    @param key_file: Filename that holds the key for authentication.

    @return: A dict of the info.

    @raises HwIdException: If hwid/info_type/key_file is invalid or there's an
                           error anywhere related to getting the raw hwid info
                           or decoding it.
    """
    # There are situations we don't want to call out to the hwid service, we use
    # the key_file name as the indicator for that.
    if key_file == KEY_FILENAME_NO_HWID:
        return {}

    if not isinstance(hwid, str):
        raise ValueError('hwid is not a string.')

    if info_type not in HWID_INFO_TYPES:
        raise ValueError('invalid info type: "%s".' % info_type)

    key = None
    with open(key_file) as f:
        key = f.read().strip()

    url_format_dict = {'base_url': HWID_BASE_URL,
                       'version': HWID_VERSION,
                       'info_type': info_type,
                       'hwid': urllib2.quote(hwid),
                       'key': key}

    url_request = URL_FORMAT_STRING % url_format_dict
    try:
        page_contents = urllib2.urlopen(url_request)
    except (urllib2.URLError, urllib2.HTTPError) as e:
        # TODO(kevcheng): Might need to scrub out key from exception message.
        raise HwIdException('error retrieving raw hwid info: %s' % e)

    try:
        hwid_info_dict = json.load(page_contents)
    except ValueError as e:
        raise HwIdException('error decoding hwid info: %s - "%s"' %
                            (e, page_contents.getvalue()))
    finally:
        page_contents.close()

    return hwid_info_dict


def get_all_possible_dut_labels(key_file):
    """Return all possible labels that can be supplied by dutlabels.

    We can send a dummy key to the service to retrieve all the possible
    labels the service will provide under the dutlabel api call.  We need
    this in order to track which labels we can remove if they're not detected
    on a dut anymore.

    @param key_file: Filename that holds the key for authentication.

    @return: A list of all possible labels.
    """
    return get_hwid_info('dummy_hwid', HWID_INFO_LABEL, key_file).get(
            'possible_labels', [])
