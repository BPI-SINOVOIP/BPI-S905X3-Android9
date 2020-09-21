"""Tests for mysql_stats."""

import common

import collections
import mock
import unittest

import mysql_stats


class MysqlStatsTest(unittest.TestCase):
    """Unittest for mysql_stats."""

    def testQueryAndEmit(self):
        """Test for QueryAndEmit."""
        connection = mock.Mock()
        connection.Fetchall.return_value = [(
            'Column-name', 0)]

        # This shouldn't raise an exception.
        mysql_stats.QueryAndEmit(collections.defaultdict(lambda: 0), connection)


if __name__ == '__main__':
    unittest.main()
