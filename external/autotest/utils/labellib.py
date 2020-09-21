# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides standard functions for working with Autotest labels.

There are two types of labels, plain ("webcam") or keyval
("pool:suites").  Most of this module's functions work with keyval
labels.

Most users should use LabelsMapping, which provides a dict-like
interface for working with keyval labels.

This module also provides functions for working with cros version
strings, which are common keyval label values.
"""

import collections
import re


class Key(object):
    """Enum for keyval label keys."""
    CROS_VERSION = 'cros-version'
    CROS_ANDROID_VERSION = 'cheets-version'
    ANDROID_BUILD_VERSION = 'ab-version'
    TESTBED_VERSION = 'testbed-version'
    FIRMWARE_RW_VERSION = 'fwrw-version'
    FIRMWARE_RO_VERSION = 'fwro-version'


class LabelsMapping(collections.MutableMapping):
    """dict-like interface for working with labels.

    The constructor takes an iterable of labels, either plain or keyval.
    Plain labels are saved internally and ignored except for converting
    back to string labels.  Keyval labels are exposed through a
    dict-like interface (pop(), keys(), items(), etc. are all
    supported).

    When multiple keyval labels share the same key, the first one wins.

    The one difference from a dict is that setting a key to None will
    delete the corresponding keyval label, since it does not make sense
    for a keyval label to have a None value.  Prefer using del or pop()
    instead of setting a key to None.

    LabelsMapping has one method getlabels() for converting back to
    string labels.
    """

    def __init__(self, str_labels=()):
        self._plain_labels = []
        self._keyval_map = collections.OrderedDict()
        for str_label in str_labels:
            self._add_label(str_label)

    @classmethod
    def from_host(cls, host):
        """Create instance using a frontend.afe.models.Host object."""
        return cls(l.name for l in host.labels.all())

    def _add_label(self, str_label):
        """Add a label string to the internal map or plain labels list."""
        try:
            keyval_label = parse_keyval_label(str_label)
        except ValueError:
            self._plain_labels.append(str_label)
        else:
            if keyval_label.key not in self._keyval_map:
                self._keyval_map[keyval_label.key] = keyval_label.value

    def __getitem__(self, key):
        return self._keyval_map[key]

    def __setitem__(self, key, val):
        if val is None:
            self.pop(key, None)
        else:
            self._keyval_map[key] = val

    def __delitem__(self, key):
        del self._keyval_map[key]

    def __iter__(self):
        return iter(self._keyval_map)

    def __len__(self):
        return len(self._keyval_map)

    def getlabels(self):
        """Return labels as a list of strings."""
        str_labels = self._plain_labels[:]
        keyval_labels = (KeyvalLabel(key, value)
                         for key, value in self.iteritems())
        str_labels.extend(format_keyval_label(label)
                          for label in keyval_labels)
        return str_labels


_KEYVAL_LABEL_SEP = ':'


KeyvalLabel = collections.namedtuple('KeyvalLabel', 'key, value')


def parse_keyval_label(str_label):
    """Parse a string as a KeyvalLabel.

    If the argument is not a valid keyval label, ValueError is raised.
    """
    key, value = str_label.split(_KEYVAL_LABEL_SEP, 1)
    return KeyvalLabel(key, value)


def format_keyval_label(keyval_label):
    """Format a KeyvalLabel as a string."""
    return _KEYVAL_LABEL_SEP.join(keyval_label)


CrosVersion = collections.namedtuple(
        'CrosVersion', 'group, board, milestone, version, rc')


_CROS_VERSION_REGEX = (
        r'^'
        r'(?P<group>[a-z0-9_-]+)'
        r'/'
        r'(?P<milestone>R[0-9]+)'
        r'-'
        r'(?P<version>[0-9.]+)'
        r'(-(?P<rc>rc[0-9]+))?'
        r'$'
)

_CROS_BOARD_FROM_VERSION_REGEX = (
        r'^'
        r'(trybot-)?'
        r'(?P<board>[a-z_-]+)-(release|paladin|pre-cq|test-ap|toolchain)'
        r'/R.*'
        r'$'
)


def parse_cros_version(version_string):
    """Parse a string as a CrosVersion.

    If the argument is not a valid cros version, ValueError is raised.
    Example cros version string: 'lumpy-release/R27-3773.0.0-rc1'
    """
    match = re.search(_CROS_VERSION_REGEX, version_string)
    if match is None:
        raise ValueError('Invalid cros version string: %r' % version_string)
    parts = match.groupdict()
    match = re.search(_CROS_BOARD_FROM_VERSION_REGEX, version_string)
    if match is None:
        raise ValueError('Invalid cros version string: %r. Failed to parse '
                         'board.' % version_string)
    parts['board'] = match.group('board')
    return CrosVersion(**parts)


def format_cros_version(cros_version):
    """Format a CrosVersion as a string."""
    if cros_version.rc is not None:
        return '{group}/{milestone}-{version}-{rc}'.format(
                **cros_version._asdict())
    else:
        return '{group}/{milestone}-{version}'.format(**cros_version._asdict())
