#!/usr/bin/python
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
A script to modify dsp.ini config files.
A dsp.ini config file is represented by an Ini object.
An Ini object contains one or more Sections.
Each Section has a name, a list of Ports, and a list of NonPorts.
"""

import argparse
import logging
import os
import re
import StringIO
import sys
from collections import namedtuple

Parameter = namedtuple('Parameter', ['value', 'comment'])


class Port(object):
  """Class for port definition in ini file.

  Properties:
    io: "input" or "output".
    index: an integer for port index.
    definition: a string for the content after "=" in port definition line.
    parameter: a Parameter namedtuple which is parsed from definition.
  """
  @staticmethod
  def ParsePortLine(line):
    """Parses a port definition line in ini file and init a Port object.

    Args:
      line: A string possibly containing port definition line like
        "input_0=1;  something".

    Returns:
      A Port object if input is a valid port definition line. Returns
      None if input is not a valid port definition line.
    """
    result = re.match(r'(input|output)_(\d+)=(.*)', line)
    if result:
      parse_values = result.groups()
      io = parse_values[0]
      index = int(parse_values[1])
      definition = parse_values[2]
      return Port(io, index, definition)
    else:
      return None

  def __init__(self, io, index, definition):
    """Initializes a port.

    Initializes a port with io, index and definition. The definition will be
    further parsed to Parameter(value, comment) if the format matches
    "<some value> ; <some comment>".

    Args:
      io: "input" or "output".
      index: an integer for port index.
      definition: a string for the content after "=" in port definition line.
    """
    self.io = io
    self.index = index
    self.definition = definition
    result = re.match(r'(\S+)\s+; (.+)', definition)
    if result:
      self.parameter = Parameter._make(result.groups())
    else:
      self.parameter = None

  def FormatLine(self):
    """Returns a port definition line which is used in ini file."""
    line = '%s_%d=' % (self.io, self.index)
    if self.parameter:
      line +="{:<8}; {:}".format(self.parameter.value, self.parameter.comment)
    else:
      line += self.definition
    return line

  def _UpdateIndex(self, index):
    """Updates index of this port.

    Args:
      index: The new index.
    """
    self.index = index


class NonPort(object):
  """Class for non-port definition in ini file.

  Properties:
    name: A string representing the non-port name.
    definition: A string representing the non-port definition.
  """
  @staticmethod
  def ParseNonPortLine(line):
    """Parses a non-port definition line in ini file and init a NonPort object.

    Args:
      line: A string possibly containing non-port definition line like
        "library=builtin".

    Returns:
      A NonPort object if input is a valid non-port definition line. Returns
      None if input is not a valid non-port definition line.
    """
    result = re.match(r'(\w+)=(.*)', line)
    if result:
      parse_values = result.groups()
      name = parse_values[0]
      definition = parse_values[1]
      return NonPort(name, definition)
    else:
      return None

  def __init__(self, name, definition):
    """Initializes a NonPort <name>=<definition>.

    Args:
      name: A string representing the non-port name.
      definition: A string representing the non-port definition.
    """
    self.name = name
    self.definition = definition

  def FormatLine(self):
    """Formats a string representation of a NonPort.

    Returns:
      A string "<name>=<definition>".
    """
    line = '%s=%s' % (self.name, self.definition)
    return line


class SectionException(Exception):
  pass


class Section(object):
  """Class for section definition in ini file.

  Properties:
   name: Section name.
   non_ports: A list containing NonPorts of this section.
   ports: A list containing Ports of this section.
  """
  @staticmethod
  def ParseSectionName(line):
    """Parses a section name.

    Args:
      line: A string possibly containing a section name like [drc].

    Returns:
      Returns parsed section name without '[' and ']' if input matches
      the syntax [<section name>]. Returns None if not.
    """
    result = re.match(r'\[(\w+)\]', line)
    return result.groups()[0] if result else None

  @staticmethod
  def ParseLine(line):
    """Parses a line that belongs to a section.

    Returns:
      A Port or NonPort object if input line matches the format. Returns None
      if input line does not match the format of Port nor NonPort.
    """
    if not line:
      return
    parse_port = Port.ParsePortLine(line)
    if parse_port:
      return parse_port
    parse_non_port = NonPort.ParseNonPortLine(line)
    if parse_non_port:
      return parse_non_port

  def __init__(self, name):
    """Initializes a Section with given name."""
    self.name = name
    self.non_ports= []
    self.ports = []

  def AddLine(self, line):
    """Adds a line to this Section.

    Args:
      line: A line to be added to this section. If it matches port or non-port
      format, a Port or NonPort will be added to this section. Otherwise,
      this line is ignored.
    """
    to_add = Section.ParseLine(line)
    if not to_add:
      return
    if isinstance(to_add, Port):
      self.AppendPort(to_add)
      return
    if isinstance(to_add, NonPort):
      self.AppendNonPort(to_add)
      return

  def AppendNonPort(self, non_port):
    """Appends a NonPort to non_ports.

    Args:
      non_port: A NonPort object to be appended.
    """
    self.non_ports.append(non_port)

  def AppendPort(self, port):
    """Appends a Port to ports.

    Args:
      port: A Port object to be appended. The port should be appended
      in the order of index, so the index of port should equal to the current
      size of ports list.

    Raises:
      SectionException if the index of port is not the current size of ports
      list.
    """
    if not port.index == len(self.ports):
      raise SectionException(
          'The port with index %r can not be appended to the end of ports'
          ' of size' % (port.index, len(self.ports)))
    else:
      self.ports.append(port)

  def InsertLine(self, line):
    """Inserts a line to this section.

    Inserts a line containing port or non-port definition to this section.
    If input line matches Port or NonPort format, the corresponding insert
    method InsertNonPort or InsertPort will be called. If input line does not
    match the format, SectionException will be raised.

    Args:
      line: A line to be inserted. The line should

    Raises:
      SectionException if input line does not match the format of Port or
      NonPort.
    """
    to_insert = Section.ParseLine(line)
    if not to_insert:
      raise SectionException(
          'The line %s does not match Port or NonPort syntax' % line)
    if isinstance(to_insert, Port):
      self.InsertPort(to_insert)
      return
    if isinstance(to_insert, NonPort):
      self.InsertNonPort(to_insert)
      return

  def InsertNonPort(self, non_port):
    """Inserts a NonPort to non_ports list.

    Currently there is no ordering for non-port definition. This method just
    appends non_port to non_ports list.

    Args:
      non_port: A NonPort object.
    """
    self.non_ports.append(non_port)

  def InsertPort(self, port):
    """Inserts a Port to ports list.

    The index of port should not be greater than the current size of ports.
    After insertion, the index of each port in ports should be updated to the
    new index of that port in the ports list.
    E.g. Before insertion:
    self.ports=[Port("input", 0, "foo0"),
                Port("input", 1, "foo1"),
                Port("output", 2, "foo2")]
    Now we insert a Port with index 1 by invoking
    InsertPort(Port("output, 1, "bar")),
    Then,
    self.ports=[Port("input", 0, "foo0"),
                Port("output, 1, "bar"),
                Port("input", 2, "foo1"),
                Port("output", 3, "foo2")].
    Note that the indices of foo1 and foo2 had been shifted by one because a
    new port was inserted at index 1.

    Args:
      port: A Port object.

    Raises:
      SectionException: If the port to be inserted does not have a valid index.
    """
    if port.index > len(self.ports):
      raise SectionException('Inserting port index %d but'
          ' currently there are only %d ports' % (port.index,
              len(self.ports)))

    self.ports.insert(port.index, port)
    self._UpdatePorts()

  def _UpdatePorts(self):
    """Updates the index property of each Port in ports.

    Updates the index property of each Port in ports so the new index property
    is the index of that Port in ports list.
    """
    for index, port in enumerate(self.ports):
      port._UpdateIndex(index)

  def Print(self, output):
    """Prints the section definition to output.

    The format is:
    [section_name]
    non_port_name_0=non_port_definition_0
    non_port_name_1=non_port_definition_1
    ...
    port_name_0=port_definition_0
    port_name_1=port_definition_1
    ...

    Args:
      output: A StringIO.StringIO object.
    """
    output.write('[%s]\n' % self.name)
    for non_port in self.non_ports:
      output.write('%s\n' % non_port.FormatLine())
    for port in self.ports:
      output.write('%s\n' % port.FormatLine())


class Ini(object):
  """Class for an ini config file.

  Properties:
    sections: A dict containing mapping from section name to Section.
    section_names: A list of section names.
    file_path: The path of this ini config file.
  """
  def __init__(self, input_file):
    """Initializes an Ini object from input config file.

    Args:
      input_file: The path to an ini config file.
    """
    self.sections = {}
    self.section_names = []
    self.file_path = input_file
    self._ParseFromFile(input_file)

  def _ParseFromFile(self, input_file):
    """Parses sections in the input config file.

    Reads in the content of the input config file and parses each sections.
    The parsed sections are stored in sections dict.
    The names of each section is stored in section_names list.

    Args:
      input_file: The path to an ini config file.
    """
    content = open(input_file, 'r').read()
    content_lines = content.splitlines()
    self.sections = {}
    self.section_names = []
    current_name = None
    for line in content_lines:
      name = Section.ParseSectionName(line)
      if name:
        self.section_names.append(name)
        self.sections[name] = Section(name)
        current_name = name
      else:
        self.sections[current_name].AddLine(line)

  def Print(self, output_file=None):
    """Prints all sections of this Ini object.

    Args:
      output_file: The path to write output. If this is not None, writes the
        output to this path. Otherwise, just print the output to console.

    Returns:
      A StringIO.StringIO object containing output.
    """
    output = StringIO.StringIO()
    for index, name in enumerate(self.section_names):
      self.sections[name].Print(output)
      if index < len(self.section_names) - 1:
        output.write('\n')
    if output_file:
      with open(output_file, 'w') as f:
        f.write(output.getvalue())
        output.close()
    else:
      print output.getvalue()
    return output

  def HasSection(self, name):
    """Checks if this Ini object has a section with certain name.

    Args:
      name: The name of the section.
    """
    return name in self.sections

  def PrintSection(self, name):
    """Prints a section to console.

    Args:
      name: The name of the section.

    Returns:
      A StringIO.StringIO object containing output.
    """
    output = StringIO.StringIO()
    self.sections[name].Print(output)
    output.write('\n')
    print output.getvalue()
    return output

  def InsertLineToSection(self, name, line):
    """Inserts a line to a section.

    Args:
      name: The name of the section.
      line: A line to be inserted.
    """
    self.sections[name].InsertLine(line)


def prompt(question, binary_answer=True):
  """Displays the question to the user and wait for input.

  Args:
    question: The question to be displayed to user.
    binary_answer: True to expect an yes/no answer from user.
  Returns:
    True/False if binary_answer is True. Otherwise, returns a string
    containing user input to the question.
  """

  sys.stdout.write(question)
  answer = raw_input()
  if binary_answer:
    answer = answer.lower()
    if answer in ['y', 'yes']:
      return True
    elif answer in ['n', 'no']:
      return False
    else:
      return prompt(question)
  else:
    return answer


class IniEditorException(Exception):
  pass


class IniEditor(object):
  """The class for ini file editing command line interface.

  Properties:
    input_files: The files to be edited. Note that the same editing command
      can be applied on many config files.
    args: The result of ArgumentParser.parse_args method. It is an object
      containing args as attributes.
  """
  def __init__(self):
    self.input_files = []
    self.args = None

  def Main(self):
    """The main method of IniEditor.

    Parses the arguments and processes files according to the arguments.
    """
    self.ParseArgs()
    self.ProcessFiles()

  def ParseArgs(self):
    """Parses the arguments from command line.

    Parses the arguments from command line to determine input_files.
    Also, checks the arguments are valid.

    Raises:
      IniEditorException if arguments are not valid.
    """
    parser = argparse.ArgumentParser(
        description=('Edit or show the config files'))
    parser.add_argument('--input_file', '-i', default=None,
                        help='Use the specified file as input file. If this '
                             'is not given, the editor will try to find config '
                             'files using config_dirs and board.')
    parser.add_argument('--config_dirs', '-c',
                        default='~/trunk/src/third_party/adhd/cras-config',
                        help='Config directory. By default it is '
                             '~/trunk/src/third_party/adhd/cras-config.')
    parser.add_argument('--board', '-b', default=None, nargs='*',
                        help='The boards to apply the changes. Use "all" '
                             'to apply on all boards. '
                             'Use --board <board_1> <board_2> to specify more '
                             'than one boards')
    parser.add_argument('--section', '-s', default=None,
                        help='The section to be shown/edited in the ini file.')
    parser.add_argument('--insert', '-n', default=None,
                        help='The line to be inserted into the ini file. '
                             'Must be used with --section.')
    parser.add_argument('--output-suffix', '-o', default='.new',
                        help='The output file suffix. Set it to "None" if you '
                             'want to apply the changes in-place.')
    self.args = parser.parse_args()

    # If input file is given, just edit this file.
    if self.args.input_file:
      self.input_files.append(self.args.input_file)
    # Otherwise, try to find config files in board directories of config
    # directory.
    else:
      if self.args.config_dirs.startswith('~'):
        self.args.config_dirs = os.path.join(
            os.path.expanduser('~'),
            self.args.config_dirs.split('~/')[1])
      all_boards = os.walk(self.args.config_dirs).next()[1]
      # "board" argument must be a valid board name or "all".
      if (not self.args.board or
          (self.args.board != ['all'] and
           not set(self.args.board).issubset(set(all_boards)))):
        logging.error('Please select a board from %s or use "all".' % (
            ', '.join(all_boards)))
        raise IniEditorException('User must specify board if input_file '
                                 'is not given.')
      if self.args.board == ['all']:
        logging.info('Applying on all boards.')
        boards = all_boards
      else:
        boards = self.args.board

      self.input_files = []
      # Finds dsp.ini files in candidate boards directories.
      for board in boards:
        ini_file = os.path.join(self.args.config_dirs, board, 'dsp.ini')
        if os.path.exists(ini_file):
          self.input_files.append(ini_file)

    if self.args.insert and not self.args.section:
      raise IniEditorException('--insert must be used with --section')

  def ProcessFiles(self):
    """Processes the config files in input_files.

    Showes or edits every selected config file.
    """
    for input_file in self.input_files:
      logging.info('Looking at dsp.ini file at %s', input_file)
      ini = Ini(input_file)
      if self.args.insert:
        self.InsertCommand(ini)
      else:
        self.PrintCommand(ini)

  def PrintCommand(self, ini):
    """Prints this Ini object.

    Prints all sections or a section in input Ini object if
    args.section is specified and there is such section in this Ini object.

    Args:
      ini: An Ini object.
    """
    if self.args.section:
      if ini.HasSection(self.args.section):
        logging.info('Printing section %s.', self.args.section)
        ini.PrintSection(self.args.section)
      else:
        logging.info('There is no section %s in %s',
                     self.args.section, ini.file_path)
    else:
      logging.info('Printing ini content.')
      ini.Print()

  def InsertCommand(self, ini):
    """Processes insertion editing on Ini object.

    Inserts args.insert to section named args.section in input Ini object.
    If input Ini object does not have a section named args.section, this method
    does not do anything. If the editing is valid, prints the changed section
    to console. Writes the editied config file to the same path as input path
    plus a suffix speficied in args.output_suffix. If that suffix is "None",
    prompts and waits for user to confirm editing in-place.

    Args:
      ini: An Ini object.
    """
    if not ini.HasSection(self.args.section):
      logging.info('There is no section %s in %s',
                   self.args.section, ini.file_path)
      return

    ini.InsertLineToSection(self.args.section, self.args.insert)
    logging.info('Changed section:')
    ini.PrintSection(self.args.section)

    if self.args.output_suffix == 'None':
      answer = prompt(
          'Writing output file in-place at %s ? [y/n]' % ini.file_path)
      if not answer:
        sys.exit('Abort!')
      output_file = ini.file_path
    else:
      output_file = ini.file_path + self.args.output_suffix
      logging.info('Writing output file to : %s.', output_file)
    ini.Print(output_file)


if __name__ == '__main__':
  logging.basicConfig(
     format='%(asctime)s:%(levelname)s:%(filename)s:%(lineno)d:%(message)s',
     level=logging.DEBUG)
  IniEditor().Main()
