"""
This library provides a bunch of miscellaneous parameter parsing,
sql generating and list cleanup library functions that are used
by both the reporting cli and web interface.
"""

import os
import re
import sys

import common
from autotest_lib.tko import display
from autotest_lib.tko import frontend

def parse_scrub_and_gen_condition(condition, valid_field_dict):
    me = parse_scrub_and_gen_condition   # shorten the name
    compare_ops = {'=':'=', '<>':'<>', '==':'=', '!=':'<>', '>':'>',
                   '<':'<', '>=':'>=', '<=':'<=', '~':'LIKE', '#':'REGEXP'}

    # strip white space
    condition = condition.strip()

    # OR
    match = re.match(r'^(.+)[|](.+)$', condition)
    if match:
        (a_sql, a_values) = me(match.group(1), valid_field_dict)
        (b_sql, b_values) = me(match.group(2), valid_field_dict)
        return (" (%s) OR (%s) " % (a_sql, b_sql),
                a_values + b_values)

    # AND
    match = re.match(r'^(.+)[&](.+)$', condition)
    if match:
        (a_sql, a_values) = me(match.group(1), valid_field_dict)
        (b_sql, b_values) = me(match.group(2), valid_field_dict)
        return (" (%s) AND (%s) " % (a_sql, b_sql),
                a_values + b_values)

    # '<field> <op> <value>' where value can be quoted
    # double quotes are escaped....i.e.  '''' is the same as "'"
    regex = r'^(%s)[ \t]*(%s)[ \t]*' + \
            r'(\'((\'\'|[^\'])*)\'|"((""|[^"])*)"|([^\'"].*))$'
    regex = regex % ('|'.join(valid_field_dict.keys()),
                     '|'.join(compare_ops.keys()))
    match = re.match(regex, condition)
    if match:
        field = valid_field_dict[match.group(1)]
        op = compare_ops[match.group(2)]
        if match.group(5):
            val = match.group(4).replace("''", "'")
        elif match.group(7):
            val = match.group(6).replace('""', '"')
        elif match.group(8):
            val = match.group(8)
        else:
            raise "Internal error"
        return ("%s %s %%s" % (field, op), [val])

    raise "Could not parse '%s' (%s)" % (condition, regex)
