#!/usr/bin/python

import datetime
import unittest

import common
from autotest_lib.site_utils.deployment import commandline


class ArgumentPairTestCase(unittest.TestCase):

    """Tests for parsing and adding argument pairs."""

    def test_missing_dest(self):
        """Test for error when missing dest argument."""
        parser = commandline._ArgumentParser()
        with self.assertRaisesRegexp(ValueError, r'\bdest\b'):
            parser.add_argument_pair('--yes', '--no', default=True)

    def test_missing_dest_and_default(self):
        """Test for error when missing dest and default arguments."""
        parser = commandline._ArgumentParser()
        with self.assertRaises(ValueError) as context:
            parser.add_argument_pair('--yes', '--no')
        message = str(context.exception)
        self.assertIn('dest', message)
        self.assertIn('default', message)

    def test_default_value(self):
        """Test the default value for an option pair."""
        parser = commandline._ArgumentParser()
        parser.add_argument_pair('--yes', '--no', dest='option',
                                 default=False)
        args = parser.parse_args([])
        self.assertIs(args.option, False)

    def test_parsing_flag(self):
        """Test parsing an option flag of an option pair."""
        parser = commandline._ArgumentParser()
        parser.add_argument_pair('--yes', '--no', dest='option',
                                 default=False)
        args = parser.parse_args(['--yes'])
        self.assertIs(args.option, True)

    def test_duplicate_flag_precedence(self):
        """Test precedence when passing multiple flags."""
        parser = commandline._ArgumentParser()
        parser.add_argument_pair('--yes', '--no', dest='option',
                                 default=False)
        args = parser.parse_args(['--no', '--yes'])
        self.assertIs(args.option, True)
        args = parser.parse_args(['--yes', '--no'])
        self.assertIs(args.option, False)


class _NamespaceStub:

    def __init__(self, **kwargs):
        for key, val in kwargs.iteritems():
            setattr(self, key, val)


class DefaultLogdirNameTestCase(unittest.TestCase):

    """Test getting default logdir name."""

    def test_happy_path(self):
        """Test everything working correctly."""
        arguments = _NamespaceStub(
            start_time=datetime.datetime(2016, 5, 4, 3),
            board='shana')
        got = commandline.get_default_logdir_name(arguments)
        self.assertEqual(got, '2016-05-04T03:00:00-shana')


if __name__ == '__main__':
    unittest.main()
