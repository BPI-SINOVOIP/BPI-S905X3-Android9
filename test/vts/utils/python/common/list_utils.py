#
# Copyright (C) 2016 The Android Open Source Project
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

import itertools
import os

DEFAULT_COMMENT_CHAR = '#'


def ItemsToStr(input_list):
    '''Convert item in a list to string.

    Args:
        input_list: list of objects, the list to convert

    Return:
        A list of string where objects were converted to string using str function.
        None if input list is None.
    '''
    if not input_list:
        return input_list
    return list(map(str, input_list))


def ExpandItemDelimiters(input_list,
                         delimiter,
                         strip=False,
                         to_str=False,
                         remove_empty=True):
    '''Expand list items that contain the given delimiter.

    Args:
        input_list: list of string, a list whose item may contain a delimiter
        delimiter: string
        strip: bool, whether to strip items after expanding. Default is False
        to_str: bool, whether to convert output items in string.
                Default is False
        remove_empty: bool, whether to remove empty string in result list.
                      Will not remove None items. Default: True

    Returns:
        The expended list, which may be the same with input list
        if no delimiter found; None if input list is None
    '''
    if input_list is None:
        return None

    do_strip = lambda s: s.strip() if strip else s
    do_str = lambda s: str(s) if to_str else s

    expended_list_generator = (item.split(delimiter) for item in input_list)
    result = [
        do_strip(do_str(s))
        for s in itertools.chain.from_iterable(expended_list_generator)
    ]
    return filter(lambda s: str(s) != '', result) if remove_empty else result


def DeduplicateKeepOrder(input):
    '''Remove duplicate items from a sequence while keeping the item order.

    Args:
        input: a sequence that might have duplicated items.

    Returns:
        A deduplicated list where item order is kept.
    '''
    return MergeUniqueKeepOrder(input)


def MergeUniqueKeepOrder(*lists):
    '''Merge two list, remove duplicate items, and order.

    Args:
        lists: any number of lists

    Returns:
        A merged list where items are unique and original order is kept.
    '''
    seen = set()
    return [
        x for x in itertools.chain(*lists) if not (x in seen or seen.add(x))
    ]


def LoadListFromCommentedTextFile(file_path,
                                  to_str=True,
                                  to_strip=True,
                                  exclude_empty_line=True,
                                  exclude_comment_line=True,
                                  exclude_trailing_comment=True,
                                  remove_duplicates=False,
                                  remove_line_breaks=True,
                                  comment_char=DEFAULT_COMMENT_CHAR):
    '''Read commented text file into a list of lines.

        Comments or empty lines will be excluded by default.

        Args:
            file_path: string, path to file
            to_str: bool, whether to convert lines to string in result list.
                    Default value is True.
            to_strip: bool, whether to strip lines in result list.
                      Default value is True.
            exclude_empty_line: bool, whether to exclude empty items in result list
                                Default value is True.
            exclude_comment_line: bool, whether to exclude lines that only contains comments.
                                  If a line starts with spaces and ends with comments it
                                  will still be excluded even if to_trim is False.
                                  Default value is True.
            exclude_trailing_comment: bool, whether to remove trailing comments
                                      from result items.
                                      Default value is True.
            remove_duplicates: bool, whether to remove duplicate items in output list.
                               Default value is False.
            remove_line_breaks: bool, whether to remove trailing trailing
                                new line characters from result items.
                                Default value is True.
            comment_char: string, character to denote comment.
                          Default value is pound (#).

        Returns:
            a list of string. None if file does not exist.
        '''
    if not os.path.isfile(file_path):
        logging.error('The path provided is not a file or does not exist: %s',
                      file_path)
        return None

    with open(file_path, 'r') as f:
        return LoadListFromCommentedText(
            f.read(),
            to_str,
            to_strip,
            exclude_empty_line,
            exclude_comment_line,
            exclude_trailing_comment,
            remove_duplicates,
            remove_line_breaks,
            comment_char=DEFAULT_COMMENT_CHAR)


def LoadListFromCommentedText(text,
                              to_str=True,
                              to_strip=True,
                              exclude_empty_line=True,
                              exclude_comment_line=True,
                              exclude_trailing_comment=True,
                              remove_duplicates=False,
                              remove_line_breaks=True,
                              comment_char=DEFAULT_COMMENT_CHAR):
    '''Read commented text into a list of lines.

        Comments or empty lines will be excluded by default.

        Args:
            text: string, text to parse
            to_str: bool, whether to convert lines to string in result list.
                    Default value is True.
            to_strip: bool, whether to strip lines in result list.
                      Default value is True.
            exclude_empty_line: bool, whether to exclude empty items in result list
                                Default value is True.
            exclude_comment_line: bool, whether to exclude lines that only contains comments.
                                  If a line starts with spaces and ends with comments it
                                  will still be excluded even if to_trim is False.
                                  Default value is True.
            exclude_trailing_comment: bool, whether to remove trailing comments
                                      from result items.
                                      Default value is True.
            remove_duplicates: bool, whether to remove duplicate items in output list.
                               Default value is False.
            remove_line_breaks: bool, whether to remove trailing trailing
                                new line characters from result items.
                                Default value is True.
            comment_char: string, character to denote comment.
                          Default value is pound (#).

        Returns:
            a list of string.
        '''
    lines = text.splitlines(not remove_line_breaks)

    if to_str:
        lines = map(str, lines)

    if exclude_trailing_comment:

        def RemoveComment(line):
            idx = line.find(comment_char)
            if idx < 0:
                return line
            else:
                return line[:idx]

        lines = map(RemoveComment, lines)

    if to_strip:
        lines = map(lambda line: line.strip(), lines)

    if exclude_comment_line:
        lines = filter(lambda line: not line.strip().startswith(comment_char),
                       lines)

    if exclude_empty_line:
        lines = filter(bool, lines)

    if remove_duplicates:
        lines = DeduplicateKeepOrder(lines)

    return lines
