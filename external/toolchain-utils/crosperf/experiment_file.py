# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""The experiment file module. It manages the input file of crosperf."""

from __future__ import print_function
import os.path
import re
from settings_factory import SettingsFactory


class ExperimentFile(object):
  """Class for parsing the experiment file format.

  The grammar for this format is:

  experiment = { _FIELD_VALUE_RE | settings }
  settings = _OPEN_SETTINGS_RE
             { _FIELD_VALUE_RE }
             _CLOSE_SETTINGS_RE

  Where the regexes are terminals defined below. This results in an format
  which looks something like:

  field_name: value
  settings_type: settings_name {
    field_name: value
    field_name: value
  }
  """

  # Field regex, e.g. "iterations: 3"
  _FIELD_VALUE_RE = re.compile(r'(\+)?\s*(\w+?)(?:\.(\S+))?\s*:\s*(.*)')
  # Open settings regex, e.g. "label {"
  _OPEN_SETTINGS_RE = re.compile(r'(?:([\w.-]+):)?\s*([\w.-]+)\s*{')
  # Close settings regex.
  _CLOSE_SETTINGS_RE = re.compile(r'}')

  def __init__(self, experiment_file, overrides=None):
    """Construct object from file-like experiment_file.

    Args:
      experiment_file: file-like object with text description of experiment.
      overrides: A settings object that will override fields in other settings.

    Raises:
      Exception: if invalid build type or description is invalid.
    """
    self.all_settings = []
    self.global_settings = SettingsFactory().GetSettings('global', 'global')
    self.all_settings.append(self.global_settings)

    self._Parse(experiment_file)

    for settings in self.all_settings:
      settings.Inherit()
      settings.Validate()
      if overrides:
        settings.Override(overrides)

  def GetSettings(self, settings_type):
    """Return nested fields from the experiment file."""
    res = []
    for settings in self.all_settings:
      if settings.settings_type == settings_type:
        res.append(settings)
    return res

  def GetGlobalSettings(self):
    """Return the global fields from the experiment file."""
    return self.global_settings

  def _ParseField(self, reader):
    """Parse a key/value field."""
    line = reader.CurrentLine().strip()
    match = ExperimentFile._FIELD_VALUE_RE.match(line)
    append, name, _, text_value = match.groups()
    return (name, text_value, append)

  def _ParseSettings(self, reader):
    """Parse a settings block."""
    line = reader.CurrentLine().strip()
    match = ExperimentFile._OPEN_SETTINGS_RE.match(line)
    settings_type = match.group(1)
    if settings_type is None:
      settings_type = ''
    settings_name = match.group(2)
    settings = SettingsFactory().GetSettings(settings_name, settings_type)
    settings.SetParentSettings(self.global_settings)

    while reader.NextLine():
      line = reader.CurrentLine().strip()

      if not line:
        continue
      elif ExperimentFile._FIELD_VALUE_RE.match(line):
        field = self._ParseField(reader)
        settings.SetField(field[0], field[1], field[2])
      elif ExperimentFile._CLOSE_SETTINGS_RE.match(line):
        return settings

    raise EOFError('Unexpected EOF while parsing settings block.')

  def _Parse(self, experiment_file):
    """Parse experiment file and create settings."""
    reader = ExperimentFileReader(experiment_file)
    settings_names = {}
    try:
      while reader.NextLine():
        line = reader.CurrentLine().strip()

        if not line:
          continue
        elif ExperimentFile._OPEN_SETTINGS_RE.match(line):
          new_settings = self._ParseSettings(reader)
          if new_settings.name in settings_names:
            raise SyntaxError(
                "Duplicate settings name: '%s'." % new_settings.name)
          settings_names[new_settings.name] = True
          self.all_settings.append(new_settings)
        elif ExperimentFile._FIELD_VALUE_RE.match(line):
          field = self._ParseField(reader)
          self.global_settings.SetField(field[0], field[1], field[2])
        else:
          raise IOError('Unexpected line.')
    except Exception, err:
      raise RuntimeError('Line %d: %s\n==> %s' % (reader.LineNo(), str(err),
                                                  reader.CurrentLine(False)))

  def Canonicalize(self):
    """Convert parsed experiment file back into an experiment file."""
    res = ''
    board = ''
    for field_name in self.global_settings.fields:
      field = self.global_settings.fields[field_name]
      if field.assigned:
        res += '%s: %s\n' % (field.name, field.GetString())
      if field.name == 'board':
        board = field.GetString()
    res += '\n'

    for settings in self.all_settings:
      if settings.settings_type != 'global':
        res += '%s: %s {\n' % (settings.settings_type, settings.name)
        for field_name in settings.fields:
          field = settings.fields[field_name]
          if field.assigned:
            res += '\t%s: %s\n' % (field.name, field.GetString())
            if field.name == 'chromeos_image':
              real_file = (
                  os.path.realpath(os.path.expanduser(field.GetString())))
              if real_file != field.GetString():
                res += '\t#actual_image: %s\n' % real_file
            if field.name == 'build':
              chromeos_root_field = settings.fields['chromeos_root']
              if chromeos_root_field:
                chromeos_root = chromeos_root_field.GetString()
              value = field.GetString()
              autotest_field = settings.fields['autotest_path']
              autotest_path = ''
              if autotest_field.assigned:
                autotest_path = autotest_field.GetString()
              image_path, autotest_path = settings.GetXbuddyPath(
                  value, autotest_path, board, chromeos_root, 'quiet')
              res += '\t#actual_image: %s\n' % image_path
              if not autotest_field.assigned:
                res += '\t#actual_autotest_path: %s\n' % autotest_path

        res += '}\n\n'

    return res


class ExperimentFileReader(object):
  """Handle reading lines from an experiment file."""

  def __init__(self, file_object):
    self.file_object = file_object
    self.current_line = None
    self.current_line_no = 0

  def CurrentLine(self, strip_comment=True):
    """Return the next line from the file, without advancing the iterator."""
    if strip_comment:
      return self._StripComment(self.current_line)
    return self.current_line

  def NextLine(self, strip_comment=True):
    """Advance the iterator and return the next line of the file."""
    self.current_line_no += 1
    self.current_line = self.file_object.readline()
    return self.CurrentLine(strip_comment)

  def _StripComment(self, line):
    """Strip comments starting with # from a line."""
    if '#' in line:
      line = line[:line.find('#')] + line[-1]
    return line

  def LineNo(self):
    """Return the current line number."""
    return self.current_line_no
