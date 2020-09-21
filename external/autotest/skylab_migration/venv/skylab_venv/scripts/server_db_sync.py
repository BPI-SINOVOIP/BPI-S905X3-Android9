#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to sync server_db with Skylab inventory service.

This script is part of migration of autotest infrastructure services to skylab.
It is intended to run regularly in the autotest lab and:
 - obtain the server_db information exposed by the skylab inventory service
 - (for now) compare with the server_db information in the main AFE database
   and create monarch metrics if any discrepancies are found.
 - (in the future) inject this information into the server_db in master AFE
   database.
"""

import collections
import contextlib
import logging
import optparse
import signal
import sys
import time

from skylab_venv import sso_discovery

import common
import MySQLdb
from autotest_lib.client.common_lib import global_config

from chromite.lib import metrics
from chromite.lib import ts_mon_config


# Database connection variables.
DEFAULT_USER = global_config.global_config.get_config_value(
        'CROS', 'db_backup_user', type=str, default='')
DEFAULT_PASSWD = global_config.global_config.get_config_value(
        'CROS', 'db_backup_password', type=str, default='')
DB = 'chromeos_lab_servers'

Server = collections.namedtuple('Server',
                                ['hostname', 'cname', 'status', 'note'])
ServerAttribute = collections.namedtuple('ServerAttribute',
                                          ['hostname', 'attribute', 'value'])
ServerRole = collections.namedtuple('ServerRole',
                                     ['hostname', 'role'])

# Metrics
_METRICS_PREFIX = 'chromeos/autotest/skylab_migration/server_db'

# API
API_ROOT = 'https://inventory-dot-chromeos-skylab.googleplex.com/_ah/api'
API = 'infrastructure'
VERSION = 'v1'
DISCOVERY_URL = '%s/discovery/v1/apis/%s/%s/rest' % (API_ROOT, API, VERSION)

_shutdown = False

class SyncUpExpection(Exception):
  """Raised when failed to sync up server db."""
  pass


class UpdateDatabaseException(Exception):
  """Raised when failed to execute any database update mysql command."""
  pass


def server_db_dump(cursor):
  """Dump the server db in to dict.

  @param cursor: A mysql cursor object to the server db.

  @returns: a dict of servers, server_attributes, server_roles namedtuples list
  """
  cursor.execute('SELECT hostname, cname, status, note FROM servers')
  servers = map(Server._make, cursor.fetchall())

  cursor.execute('SELECT hostname, attribute, value FROM servers t1 JOIN '
                 'server_attributes t2 ON t1.id=t2.server_id')
  server_attrs = map(ServerAttribute._make, cursor.fetchall())

  cursor.execute('SELECT hostname, role FROM servers t1 JOIN '
                 'server_roles t2 ON t1.id=t2.server_id')
  server_roles = map(ServerRole._make, cursor.fetchall())

  db_output = {'servers': servers,
               'server_attributes': server_attrs,
               'server_roles': server_roles}
  return db_output


def inventory_server_list():
  """Get the response from inventory server list API.

  @returns: the response in dict format
  """
  service = sso_discovery.build_service(
      service_name=API, version=VERSION, discovery_service_url=DISCOVERY_URL)
  result = service.list_servers().execute()
  return result


def inventory_api_response_parse(response, environment):
  """Parse the response from inventory API to namedtuples.

  @param response: the response in dict format.
  @environment: the lab environment of the servers, prod or staging.

  @returns: a dict of servers, server_attrs, server_roles namedtuples list.
  """
  summaries = response.get('servers', [])
  # Parse server tuples, replace notes with note in summaries
  summaries = [s for s in summaries
               if s['environment'].lower() == environment.lower()]
  servers = []
  for d in summaries:
    d['note'] = d['notes']
    del d['notes']
    sub_dict_for_server = {k:d[k].lower() for k in Server._fields}
    # cname has unique constraint in DB, empty value should be set to None.
    if not sub_dict_for_server['cname']:
      sub_dict_for_server['cname'] = None
    if not sub_dict_for_server['note']:
      sub_dict_for_server['note'] = None
    servers.append(Server(**sub_dict_for_server))

  # Parse server_attrs tuples
  server_attrs = []
  for nest_dict in summaries:
    hostname = nest_dict['hostname']
    for k, v in nest_dict.get('attributes', {}).iteritems():
      # Skip the entry whose attribute's value is null
      if v:
        flat_dict = {'hostname':hostname, 'attribute':k, 'value':v.lower()}
        server_attrs.append(ServerAttribute(**flat_dict))

  # Parse server_roles tuples
  server_roles = []
  for nest_dict in summaries:
    hostname = nest_dict['hostname']
    for role in nest_dict['roles']:
      flat_dict = {'hostname': hostname, 'role':role.lower()}
      server_roles.append(ServerRole(**flat_dict))

  api_output = {'servers': servers,
                'server_attributes': server_attrs,
                'server_roles': server_roles}
  return api_output


def create_mysql_updates(api_output, db_output, table,
                         server_id_map, warn_only):
  """Sync up servers table in server db with the inventory service.

  First step, entries in server_db but not in inventory services will be deleted
  from db. Then, entries in inventory service but not in server_db will be
  inserted into server_db.

  @param api_output: a dict mapping table name to list of corresponding
                     namedtuples parsed from inventory. This is the only
                     source of truth.
  @param db_output: a dict mapping table name to list of corresponding
                    namedtuples parsed from server db.
  @param table: name of the targeted server_db table.
  @param server_id_map: server hostname to id mapping dict.
  @param warn_only: whether it is warn_only. If yes, there will be no server id
                    for server_attributes and server_roles.

  @returns a list of mysql update commands, e.g.
  ['DELETE FROM a WHERE xx', 'INSERT ...']
  """
  logging.info('Checking table %s with inventory service...', table)

  mysql_cmds = []
  delete_entries = set(db_output[table]) - set(api_output[table])
  insert_entries = set(api_output[table]) - set(db_output[table])

  if delete_entries:
    logging.info('\nTable %s is not synced up! Below is a list of entries '
                 'that exist only in server db. These invalid entries will be '
                 'deleted from server db:\n%s',  table, delete_entries)

    for entry in delete_entries:
      if table == 'servers':
        cmd = 'DELETE FROM servers WHERE hostname=%r' % entry.hostname
      elif table == 'server_attributes':
        cmd = ('DELETE FROM server_attrs WHERE server_id=%d and attribute=%r' %
               (server_id_map[entry.hostname], entry.attribute))
      else:
        cmd = ('DELETE FROM server_roles WHERE server_id=%d and role=%r' %
               (server_id_map[entry.hostname], entry.role))
      mysql_cmds.append(cmd)

  if insert_entries:
    logging.info('\nTable %s is not synced up! Below is a list of entries '
                 'that exist only in inventory service. These new entries will'
                 ' be inserted in to server db:\n%s', table, insert_entries)

    for entry in insert_entries:
      # If this is warn_only, it is very likely that the server id for new
      # entry does not exsit since the server has not been inserted into servers
      # table yet. For this case, fake it as 0.
      if warn_only and not server_id_map.get(entry.hostname):
        server_id = 0
      else:
        server_id = server_id_map[entry.hostname]

      if table == 'servers':
        cname = entry.cname.__repr__() if entry.cname else 'NULL'
        cmd = ('INSERT INTO servers (hostname, cname, status, note) '
               'VALUES(%r, %s, %r, %r)' % (entry.hostname,
                                           cname,
                                           entry.status,
                                           entry.note))
      elif table == 'server_attributes':
        cmd = ('INSERT INTO server_attributes (server_id, attribute, value) '
               'VALUES(%d, %r, %r)' % (server_id,
                                       entry.attribute,
                                       entry.value))
      else:
        cmd = ('INSERT INTO server_roles (server_id, role) VALUES(%d, %r)' %
               (server_id, entry.role))
      mysql_cmds.append(cmd)

  metrics.Gauge(_METRICS_PREFIX + '/inconsistency_found').set(
      len(delete_entries), fields={'table': table, 'action': 'to_delete'})
  metrics.Gauge(_METRICS_PREFIX + '/inconsistency_found').set(
      len(insert_entries), fields={'table': table, 'action': 'to_add'})

  return mysql_cmds


def parse_options():
  """Parse the command line arguments."""
  usage = 'usage: %prog [options]'
  parser = optparse.OptionParser(usage=usage)
  parser.add_option('-o', '--host', default='localhost',
                    help='host of the Autotest DB. Default is localhost.')
  parser.add_option('-u', '--user', default=DEFAULT_USER,
                    help='User to login to the Autotest DB. Default is the '
                         'one defined in config file.')
  parser.add_option('-p', '--password', default=DEFAULT_PASSWD,
                    help='Password to login to the Autotest DB. Default is '
                         'the one defined in config file.')
  parser.add_option('-w', '--warn_only', action='store_true', default=False,
                    help='Only raise warnings if server_db is inconsistent '
                         'with inventory service. Do not attempt to update '
                         'server_db to match inventory service.')
  parser.add_option('-e', '--environment', default='prod',
                    help='Environment of the server_db, prod or staging. '
                         'Default is prod')
  parser.add_option('-s', '--sleep', type=int, default=300,
                    help='Time to sleep between two server db sync. '
                         'Default is 300s')
  options, args = parser.parse_args()
  return parser, options, args


def verify_options_and_args(options, args):
  """Verify the validity of options and args.

  @param options: The parsed options to verify.
  @param args: The parsed args to verify.

  @returns: True if verification passes, False otherwise.
  """
  if args:
    logging.error('Unknown arguments: ' + str(args))
    return False

  if not (options.user and options.password):
    logging.error('Failed to get the default user of password for Autotest'
                  ' DB. Please specify them through the command line.')
    return False
  return True


def _modify_table(cursor, mysql_cmds, table):
  """Helper method to commit a list of sql_cmds.

  @param cursor: mysql cursor instance.
  @param mysql_cmds: the list of sql cmd to be executed.
  @param table: the name of the table modified.
  """
  try:
    succeed = False
    for cmd in mysql_cmds:
      logging.info('running command: %s', cmd)
      cursor.execute(cmd)
    succeed = True
  except Exception as e:
    msg = ('Fail to run the following sql command:\n%s\nError:\n%s\n'
           'All changes made to server db will be rollback.' %
           (cmd, e))
    logging.error(msg)
    raise UpdateDatabaseException(msg)
  finally:
    num_deletes = len([cmd.startswith('DELETE') for cmd in mysql_cmds])
    num_inserts = len([cmd.startswith('INSERT') for cmd in mysql_cmds])
    metrics.Gauge(_METRICS_PREFIX + '/inconsistency_fixed').set(
        num_deletes,
        fields={'table': table, 'action': 'delete', 'succeed': succeed})
    metrics.Gauge(_METRICS_PREFIX + '/inconsistency_fixed').set(
        num_inserts,
        fields={'table': table, 'action': 'insert', 'succeed': succeed})


@contextlib.contextmanager
def _msql_connection_with_transaction(options):
  """mysql connection helper.

  @param options: parsed command line options.
  """
  conn =  MySQLdb.connect(options.host, options.user, options.password, DB)
  try:
    yield conn
  except:
    conn.rollback()
    raise
  else:
    conn.commit()
  finally:
    conn.close()


@contextlib.contextmanager
def _cursor(conn):
  """mysql connect cursor helper.

  @param conn: a Mysql db connection instance.
  """
  cursor = conn.cursor()
  try:
    yield cursor
  finally:
    cursor.close()


def handle_signal(signum, frame):
  """Register signal handler."""
  global _shutdown
  _shutdown = True
  logging.info("Shutdown request received.")


def _main(options):
  """Main entry.

  @param args: parsed command line arguments.
  """

  response = inventory_server_list()
  skylab_server_data = inventory_api_response_parse(response,
                                                    options.environment)
  with _msql_connection_with_transaction(options) as conn:
    with _cursor(conn) as cursor:
      db_output = server_db_dump(cursor)

      # Update servers table first, since it will cause server_id change. It
      # also delete entries in server_attributes and server_roles
      # associated with that deleted server.
      for table in ['servers', 'server_attributes', 'server_roles']:
        cursor.execute('SELECT id, hostname FROM servers')
        server_id_map = {row[1]:row[0] for row in cursor.fetchall()}
        mysql_cmds = create_mysql_updates(skylab_server_data,
                                          db_output,
                                          table,
                                          server_id_map,
                                          options.warn_only)
        if not options.warn_only:
          logging.info('Start updating table %s', table)
          _modify_table(cursor, mysql_cmds, table)
          logging.info('Successfully synced table %s with inventory service',
                       table)

        # Since there exist cascade deletion in servers table, dump db_output
        # again after syncing up servers table.
        if table == 'servers':
          db_output = server_db_dump(cursor)

def main(argv):
  """Entry point."""
  logging.basicConfig(level=logging.INFO,
                      format="%(asctime)s - %(name)s - " +
                      "%(levelname)s - %(message)s")
  parser, options, args = parse_options()
  if not verify_options_and_args(options, args):
    parser.print_help()
    sys.exit(1)

  with ts_mon_config.SetupTsMonGlobalState(service_name='sync_server_db',
                                           indirect=True):
    try:
      metrics.Counter(_METRICS_PREFIX + '/start').increment()
      logging.info("Setting signal handler")
      signal.signal(signal.SIGINT, handle_signal)
      signal.signal(signal.SIGTERM, handle_signal)

      while not _shutdown:
        _main(options)
        metrics.Counter(_METRICS_PREFIX + '/tick').increment(
          fields={'success': True})
        time.sleep(options.sleep)
    except:
      metrics.Counter(_METRICS_PREFIX + '/tick').increment(
          fields={'success': False})
      raise


if __name__ == '__main__':
  main(sys.argv)
