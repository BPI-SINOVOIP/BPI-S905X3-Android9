#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import parse
import sys

from abc import ABCMeta
from abc import abstractmethod
from ply import lex
from ply import yacc
from vts.utils.python.file import target_file_utils


def repeat_rule(to_repeat, zero_ok=False):
    '''
    From a given rule, generates a rule that allows consecutive items
    of that rule. Instances are collected in a list.
    '''

    def p_multiple(self, p):
        if len(p) == 2 and zero_ok:
            p[0] = []
        elif len(p) == 2:
            p[0] = [p[1]]
        else:
            p[0] = p[1] + [p[2]]

    func = p_multiple
    format_tuple = (to_repeat, to_repeat, to_repeat, 'empty'
                    if zero_ok else to_repeat)
    func.__doc__ = '%ss : %ss %s \n| %s' % format_tuple
    return func


def literal_token(tok):
    '''
    A compact function to specify literal string tokens when.
    they need to take precedence over a generic string,
    Among these tokens precedence is decided in alphabetic order.
    '''

    def t_token(self, t):
        return t

    func = t_token
    func.__doc__ = tok
    return func


class KernelProcFileTestBase(object):
    """
    An abstract test for the formatting of a procfs file. Individual
    files can inherit from this class.

    New parsing rules can be defined in the form of p_RULENAME, and
    similarly new tokens can be defined as t_TOKENNAME.

    Child class should also specify a `start` variable to give the starting rule.
    """

    __metaclass__ = ABCMeta

    def t_HEX_LITERAL(self, t):
        r'0x[a-f0-9]+'
        t.value = int(t.value, 0)
        return t

    def t_FLOAT(self, t):
        r'([0-9]+[.][0-9]*|[0-9]*[.][0-9]+)'
        t.value = float(t.value)
        return t

    def t_NUMBER(self, t):
        r'\d+'
        t.value = int(t.value)
        return t

    t_PATH = r'/[^\0]+'
    t_COLON = r':'
    t_EQUALS = r'='
    t_COMMA = r','
    t_STRING = r'[a-zA-Z\(\)_0-9\-@]+'

    t_TAB = r'\t'
    t_SPACE = r'[ ]'

    def t_DASH(self, t):
        r'\-'
        return t

    def t_NEWLINE(self, t):
        r'\n'
        t.lexer.lineno += len(t.value)
        return t

    t_ignore = ''

    def t_error(self, t):
        raise SyntaxError("Illegal character '%s' in line %d '%s'" % \
                (t.value[0], t.lexer.lineno, t.value.split()[0]))

    p_SPACEs = repeat_rule('SPACE', zero_ok=True)

    def p_error(self, p):
        raise SyntaxError("Parsing error at token %s in line %d" %
                          (p, p.lexer.lineno))

    def p_empty(self, p):
        'empty :'
        pass

    def __init__(self):
        self.tokens = [
            t_name[2:] for t_name in dir(self)
            if len(t_name) > 2 and t_name[:2] == 't_'
        ]
        self.tokens.remove('error')
        self.tokens.remove('ignore')
        self.lexer = lex.lex(module=self)
        # (Change logger output stream if debugging)
        self.parser = yacc.yacc(module=self, write_tables=False, \
                errorlog=yacc.PlyLogger(sys.stderr)) #open(os.devnull, 'w')))

    def parse_line(self, rule, line, custom={}):
        """Parse a line of text with the parse library.

        Args:
            line: string, a line of text
            rule: string, a format rule. See parse documentation
            custom: dict, maps to custom type conversion functions

        Returns:
            list, information parsed from the line

        Raises:
            SyntaxError: if the line could not be parsed.
        """
        parsed = parse.parse(rule, line, custom)
        if parsed is None:
            raise SyntaxError("Failed to parse line %s according to rule %s" %
                              (line, rule))
        return list(parsed)

    def parse_contents(self, file_contents):
        """Using the internal parser, parse the contents.

        Args:
            file_contents: string, entire contents of a file

        Returns:
            list, a parsed representation of the file

        Raises:
            SyntaxError: if the file could not be parsed
        """
        return self.parser.parse(file_contents, lexer=self.lexer)

    @abstractmethod
    def get_path(self):
        """Returns the full path of this proc file (string)."""
        pass

    def prepare_test(self, shell, dut):
        """Performs any actions necessary before testing the proc file.

        Args:
            shell: shell object, for preparation that requires device access

        Returns:
            boolean, True if successful.
        """
        return True

    def file_optional(self):
        """Returns: True if file is allowed to be absent (boolean)."""
        return False

    def result_correct(self, parse_result):
        """Returns: True if the parsed result meets the requirements (boolean)."""
        return True

    def test_format(self):
        """Returns:
            boolean, True if the file should be read and its format tested.
                     False if only the existence and permission should be tested.
        """
        return True

    def get_permission_checker(self):
        """Gets the function handle to use for validating file permissions.

        Return the function that will check if the permissions are correct.
        By default, return the IsReadOnly function from target_file_utils.

        Returns:
            function which takes one argument (the unix file permission bits
            in octal format) and returns True if the permissions are correct,
            False otherwise.
        """
        return target_file_utils.IsReadOnly
