#!/usr/bin/python
# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for server_db_sync."""

from __future__ import print_function

import mock
import unittest

from skylab_venv.scripts import server_db_sync as sds

class ServerDbSyncTest(unittest.TestCase):
  """Test server_db_sync."""

  INVENTORY_RESPONSE = {
      "servers": [
          {
              "attributes": {"ip": "",
                             "max_processes": "0",
                             "mysql_server_id": "2"},
              "cname": "",
              "environment": "PROD",
              "hostname": "foo",
              "notes": "autotest database slave",
              "roles": ["DATABASE_SLAVE"],
              "status": "PRIMARY"
          },
          {
              "attributes": {
                  "ip": "1.2.3.4",
                  "max_processes": "0",
                  "mysql_server_id": ""
              },
              "cname": "",
              "environment": "STAGING",
              "hostname": "bar",
              "notes": "",
              "roles": ["CRASH_SERVER"],
              "status": "BACKUP"
          }
      ]
  }


  def testServerDbDumpWithoutNone(self):
    """Test server_db_dump without None values."""
    cursor_mock = mock.MagicMock()
    cursor_mock.fetchall.side_effect = [
        (('hostname_1', None, 'primary', '',),),
        (('hostname_1', 'attr_1', 'value_1',),),
        (('hostname_1', 'role_1',),)
    ]

    expect_returns = {
        'servers': [sds.Server('hostname_1', None, 'primary', '')],
        'server_attributes': [sds.ServerAttribute('hostname_1',
                                                   'attr_1',
                                                   'value_1')],
        'server_roles': [sds.ServerRole('hostname_1', 'role_1')]
    }
    results = sds.server_db_dump(cursor_mock)
    self.assertEqual(expect_returns, results)


  def testInventoryApiResponseParseProdServer(self):
    """Test inventory_api_response_parse."""
    results = sds.inventory_api_response_parse(self.INVENTORY_RESPONSE, 'prod')

    expect_servers = [sds.Server('foo', None, 'primary',
                                 'autotest database slave')]
    expect_server_attrs = [
        sds.ServerAttribute('foo', 'mysql_server_id', '2'),
        sds.ServerAttribute('foo', 'max_processes', '0'),
    ]
    expect_server_roles = [
        sds.ServerRole('foo', 'database_slave'),
    ]

    self.assertEqual(expect_servers, results['servers'])
    self.assertEqual(expect_server_attrs, results['server_attributes'])
    self.assertEqual(expect_server_roles, results['server_roles'])


  def testInventoryApiResponseParseStagingServer(self):
    """Test inventory_api_response_parse."""
    results = sds.inventory_api_response_parse(self.INVENTORY_RESPONSE,
                                               'staging')

    expect_servers = [sds.Server('bar', None, 'backup', '')]
    expect_server_attrs = [
        sds.ServerAttribute('bar', 'ip', '1.2.3.4'),
        sds.ServerAttribute('bar', 'max_processes', '0'),
    ]
    expect_server_roles = [
        sds.ServerRole('bar', 'crash_server')
    ]

    self.assertEqual(expect_servers, results['servers'])
    self.assertEqual(expect_server_attrs, results['server_attributes'])
    self.assertEqual(expect_server_roles, results['server_roles'])


  def testCreateMysqlUpdatesServersTableWhenNothingToUpdate(self):
    """Test create_mysql_updates with servers table."""
    api_output = {'servers': [sds.Server('foo', None, 'primary', '')],
                  'server_attributes': [], 'server_roles': []}
    db_output = {'servers': [sds.Server('foo', None, 'primary', '')],
                 'server_attributes': [], 'server_roles': []}
    table = 'servers'
    server_id_map = {'foo': 1}

    result = sds.create_mysql_updates(api_output,
                                      db_output,
                                      table,
                                      server_id_map,
                                      False)
    expect_returns = []
    self.assertEqual(expect_returns, result)


  def testCreateMysqlUpdatesServersTableWhenDbIsInconsistent(self):
    """Test create_mysql_updates with servers table."""
    api_output = {'servers': [sds.Server('foo', None, 'backup', '')],
                  'server_attributes': [], 'server_roles': []}
    db_output = {'servers': [sds.Server('foo', None, 'primary', '')],
                 'server_attributes': [], 'server_roles': []}
    table = 'servers'
    server_id_map = {'foo': 1}

    result = sds.create_mysql_updates(api_output,
                                      db_output,
                                      table,
                                      server_id_map,
                                      False)
    delete_cmd = "DELETE FROM servers WHERE hostname='foo'"
    insert_cmd = ("INSERT INTO servers (hostname, cname, status, note) "
                  "VALUES('foo', NULL, 'backup', '')")
    self.assertEqual([delete_cmd, insert_cmd], result)


  def testCreateMysqlUpdatesServersAttrsTableWhenNothingToUpdate(self):
    """Test create_mysql_updates with servers table."""
    api_output = {
        'server_attributes': [sds.ServerAttribute('foo', 'ip', '1')],
        'servers': [], 'server_roles': []}
    db_output = {
        'server_attributes': [sds.ServerAttribute('foo', 'ip', '1')],
        'servers': [], 'server_roles': []}
    table = 'server_attributes'
    server_id_map = {'foo': 1, 'bar': 2}

    result = sds.create_mysql_updates(api_output,
                                      db_output,
                                      table,
                                      server_id_map,
                                      False)
    expect_returns = []
    self.assertEqual(expect_returns, result)


  def testCreateMysqlUpdatesServersAttrsTableWhenDbIsInconsistent(self):
    """Test create_mysql_updates with servers table."""
    api_output = {
        'server_attributes': [sds.ServerAttribute('foo', 'ip', '1'),
                               sds.ServerAttribute('bar', 'ip', '2')],
        'servers': [], 'server_roles': []}
    db_output = {
        'server_attributes': [sds.ServerAttribute('foo', 'ip', '1')],
        'servers': [], 'server_roles': []}
    table = 'server_attributes'
    server_id_map = {'foo': 1, 'bar': 2}

    result = sds.create_mysql_updates(api_output,
                                      db_output,
                                      table,
                                      server_id_map,
                                      False)
    insert_cmd = ("INSERT INTO server_attributes (server_id, attribute, value)"
                  " VALUES(2, 'ip', '2')")
    self.assertEqual([insert_cmd], result)


  def testCreateMysqlUpdatesServerRolesTableWhenNothingToUpdate(self):
    """Test create_mysql_updates with servers table."""
    api_output = {'servers': [], 'server_attributes': [],
                  'server_roles': [sds.ServerRole('foo', 'afe')]}
    db_output = {'servers': [], 'server_attributes': [],
                 'server_roles': [sds.ServerRole('foo', 'afe')]}
    table = 'server_roles'
    server_id_map = {'foo': 1}

    result = sds.create_mysql_updates(api_output,
                                      db_output,
                                      table,
                                      server_id_map,
                                      False)
    expect_returns = []
    self.assertEqual(expect_returns, result)


  def testCreateMysqlUpdatesServerRolesTableWhenDbIsInconsistent(self):
    """Test create_mysql_updates with servers table."""
    api_output = {'servers': [], 'server_attributes': [],
                  'server_roles': [sds.ServerRole('foo', 'afe')]}
    db_output = {'servers': [], 'server_attributes': [],
                 'server_roles': [sds.ServerRole('foo', 'afe'),
                                  sds.ServerRole('bar', 'scheduler')]}
    table = 'server_roles'
    server_id_map = {'foo': 1, 'bar': 2}

    result = sds.create_mysql_updates(api_output,
                                      db_output,
                                      table,
                                      server_id_map,
                                      False)
    delete_cmd = ("DELETE FROM server_roles WHERE server_id=2 "
                  "and role='scheduler'")
    self.assertEqual([delete_cmd], result)


  def testModifyTableRaiseUpdateDataBaseException(self):
    """Test _modify_table raise UpdateDatabaseException."""
    cursor_mock = mock.MagicMock()
    cursor_mock.execute.side_effect = Exception
    with self.assertRaises(sds.UpdateDatabaseException):
      sds._modify_table(cursor_mock, ['DELETE A FROM B'], 'x')



if __name__ == '__main__':
    unittest.main()
