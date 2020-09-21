# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides utility functions to help managing servers in server
database (defined in global config section AUTOTEST_SERVER_DB).

"""

import collections
import json
import socket
import subprocess
import sys

import common

import django.core.exceptions
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.global_config import global_config
from autotest_lib.frontend.server import models as server_models
from autotest_lib.site_utils.lib import infra


class ServerActionError(Exception):
    """Exception raised when action on server failed.
    """


def use_server_db():
    """Check if use_server_db is enabled in configuration.

    @return: True if use_server_db is set to True in global config.
    """
    return global_config.get_config_value(
            'SERVER', 'use_server_db', default=False, type=bool)


def warn_missing_role(role, exclude_server):
    """Post a warning if Autotest instance has no other primary server with
    given role.

    @param role: Name of the role.
    @param exclude_server: Server to be excluded from search for role.
    """
    servers = server_models.Server.objects.filter(
            roles__role=role,
            status=server_models.Server.STATUS.PRIMARY).exclude(
                    hostname=exclude_server.hostname)
    if not servers:
        message = ('WARNING! There will be no server with role %s after it\'s '
                   'removed from server %s. Autotest will not function '
                   'normally without any server in role %s.' %
                   (role, exclude_server.hostname, role))
        print >> sys.stderr, message


def get_servers(hostname=None, role=None, status=None):
    """Find servers with given role and status.

    @param hostname: hostname of the server.
    @param role: Role of server, default to None.
    @param status: Status of server, default to None.

    @return: A list of server objects with given role and status.
    """
    filters = {}
    if hostname:
        filters['hostname'] = hostname
    if role:
        filters['roles__role'] = role
    if status:
        filters['status'] = status
    return list(server_models.Server.objects.filter(**filters))


def format_servers(servers):
    """Format servers for printing.

    Example output:

        Hostname     : server2
        Status       : primary
        Roles        : drone
        Attributes   : {'max_processes':300}
        Date Created : 2014-11-25 12:00:00
        Date Modified: None
        Note         : Drone in lab1

    @param servers: Sequence of Server instances.
    @returns: Formatted output as string.
    """
    return '\n'.join(str(server) for server in servers)


def format_servers_json(servers):
    """Format servers for printing as JSON.

    @param servers: Sequence of Server instances.
    @returns: String.
    """
    server_dicts = []
    for server in servers:
        if server.date_modified is None:
            date_modified = None
        else:
            date_modified = str(server.date_modified)
        attributes = {k: v for k, v in server.attributes.values_list(
                'attribute', 'value')}
        server_dicts.append({'hostname': server.hostname,
                             'status': server.status,
                             'roles': server.get_role_names(),
                             'date_created': str(server.date_created),
                             'date_modified': date_modified,
                             'note': server.note,
                             'attributes': attributes})
    return json.dumps(server_dicts)


_SERVER_TABLE_FORMAT = ('%(hostname)-30s | %(status)-7s | %(roles)-20s |'
                        ' %(date_created)-19s | %(date_modified)-19s |'
                        ' %(note)s')


def format_servers_table(servers):
    """format servers for printing as a table.

    Example output:

        Hostname | Status  | Roles     | Date Created    | Date Modified | Note
        server1  | backup  | scheduler | 2014-11-25 23:45:19 |           |
        server2  | primary | drone     | 2014-11-25 12:00:00 |           | Drone

    @param servers: Sequence of Server instances.
    @returns: Formatted output as string.
    """
    result_lines = [(_SERVER_TABLE_FORMAT %
                     {'hostname': 'Hostname',
                      'status': 'Status',
                      'roles': 'Roles',
                      'date_created': 'Date Created',
                      'date_modified': 'Date Modified',
                      'note': 'Note'})]
    for server in servers:
        roles = ','.join(server.get_role_names())
        result_lines.append(_SERVER_TABLE_FORMAT %
                            {'hostname':server.hostname,
                             'status': server.status or '',
                             'roles': roles,
                             'date_created': server.date_created,
                             'date_modified': server.date_modified or '',
                             'note': server.note or ''})
    return '\n'.join(result_lines)


def format_servers_summary(servers):
    """format servers for printing a summary.

    Example output:

        scheduler      : server1(backup), server3(primary),
        host_scheduler :
        drone          : server2(primary),
        devserver      :
        database       :
        crash_server   :
        No Role        :

    @param servers: Sequence of Server instances.
    @returns: Formatted output as string.
    """
    servers_by_role = _get_servers_by_role(servers)
    servers_with_roles = {server for role_servers in servers_by_role.itervalues()
                          for server in role_servers}
    servers_without_roles = [server for server in servers
                             if server not in servers_with_roles]
    result_lines = ['Roles and status of servers:', '']
    for role, role_servers in servers_by_role.iteritems():
        result_lines.append(_format_role_servers_summary(role, role_servers))
    if servers_without_roles:
        result_lines.append(
                _format_role_servers_summary('No Role', servers_without_roles))
    return '\n'.join(result_lines)


def format_servers_nameonly(servers):
    """format servers for printing names only

    @param servers: Sequence of Server instances.
    @returns: Formatted output as string.
    """
    return '\n'.join(s.hostname for s in servers)


def _get_servers_by_role(servers):
    """Return a mapping from roles to servers.

    @param servers: Iterable of servers.
    @returns: Mapping of role strings to lists of servers.
    """
    roles = [role for role, _ in server_models.ServerRole.ROLE.choices()]
    servers_by_role = collections.defaultdict(list)
    for server in servers:
        for role in server.get_role_names():
            servers_by_role[role].append(server)
    return servers_by_role


def _format_role_servers_summary(role, servers):
    """Format one line of servers for a role in a server list summary.

    @param role: Role string.
    @param servers: Iterable of Server instances.
    @returns: String.
    """
    servers_part = ', '.join(
            '%s(%s)' % (server.hostname, server.status)
            for server in servers)
    return '%-15s: %s' % (role, servers_part)


def check_server(hostname, role):
    """Confirm server with given hostname is ready to be primary of given role.

    If the server is a backup and failed to be verified for the role, remove
    the role from its roles list. If it has no other role, set its status to
    repair_required.

    @param hostname: hostname of the server.
    @param role: Role to be checked.
    @return: True if server can be verified for the given role, otherwise
             return False.
    """
    # TODO(dshi): Add more logic to confirm server is ready for the role.
    # For now, the function just checks if server is ssh-able.
    try:
        infra.execute_command(hostname, 'true')
        return True
    except subprocess.CalledProcessError as e:
        print >> sys.stderr, ('Failed to check server %s, error: %s' %
                              (hostname, e))
        return False


def verify_server(exist=True):
    """Decorator to check if server with given hostname exists in the database.

    @param exist: Set to True to confirm server exists in the database, raise
                  exception if not. If it's set to False, raise exception if
                  server exists in database. Default is True.

    @raise ServerActionError: If `exist` is True and server does not exist in
                              the database, or `exist` is False and server exists
                              in the database.
    """
    def deco_verify(func):
        """Wrapper for the decorator.

        @param func: Function to be called.
        """
        def func_verify(*args, **kwargs):
            """Decorator to check if server exists.

            If exist is set to True, raise ServerActionError is server with
            given hostname is not found in server database.
            If exist is set to False, raise ServerActionError is server with
            given hostname is found in server database.

            @param func: function to be called.
            @param args: arguments for function to be called.
            @param kwargs: keyword arguments for function to be called.
            """
            hostname = kwargs['hostname']
            try:
                server = server_models.Server.objects.get(hostname=hostname)
            except django.core.exceptions.ObjectDoesNotExist:
                server = None

            if not exist and server:
                raise ServerActionError('Server %s already exists.' %
                                        hostname)
            if exist and not server:
                raise ServerActionError('Server %s does not exist in the '
                                        'database.' % hostname)
            if server:
                kwargs['server'] = server
            return func(*args, **kwargs)
        return func_verify
    return deco_verify


def get_drones():
    """Get a list of drones in status primary.

    @return: A list of drones in status primary.
    """
    servers = get_servers(role=server_models.ServerRole.ROLE.DRONE,
                          status=server_models.Server.STATUS.PRIMARY)
    return [s.hostname for s in servers]


def delete_attribute(server, attribute):
    """Delete the attribute from the host.

    @param server: An object of server_models.Server.
    @param attribute: Name of an attribute of the server.
    """
    attributes = server.attributes.filter(attribute=attribute)
    if not attributes:
        raise ServerActionError('Server %s does not have attribute %s' %
                                (server.hostname, attribute))
    attributes[0].delete()
    print 'Attribute %s is deleted from server %s.' % (attribute,
                                                       server.hostname)


def change_attribute(server, attribute, value):
    """Change the value of an attribute of the server.

    @param server: An object of server_models.Server.
    @param attribute: Name of an attribute of the server.
    @param value: Value of the attribute of the server.

    @raise ServerActionError: If the attribute already exists and has the
                              given value.
    """
    attributes = server_models.ServerAttribute.objects.filter(
            server=server, attribute=attribute)
    if attributes and attributes[0].value == value:
        raise ServerActionError('Attribute %s for Server %s already has '
                                'value of %s.' %
                                (attribute, server.hostname, value))
    if attributes:
        old_value = attributes[0].value
        attributes[0].value = value
        attributes[0].save()
        print ('Attribute `%s` of server %s is changed from %s to %s.' %
                     (attribute, server.hostname, old_value, value))
    else:
        server_models.ServerAttribute.objects.create(
                server=server, attribute=attribute, value=value)
        print ('Attribute `%s` of server %s is set to %s.' %
               (attribute, server.hostname, value))


def get_shards():
    """Get a list of shards in status primary.

    @return: A list of shards in status primary.
    """
    servers = get_servers(role=server_models.ServerRole.ROLE.SHARD,
                          status=server_models.Server.STATUS.PRIMARY)
    return [s.hostname for s in servers]


def confirm_server_has_role(hostname, role):
    """Confirm a given server has the given role, and its status is primary.

    @param hostname: hostname of the server.
    @param role: Name of the role to be checked.
    @raise ServerActionError: If localhost does not have given role or it's
                              not in primary status.
    """
    if hostname.lower() in ['localhost', '127.0.0.1']:
        hostname = socket.gethostname()
    hostname = utils.normalize_hostname(hostname)

    servers = get_servers(role=role, status=server_models.Server.STATUS.PRIMARY)
    for server in servers:
        if hostname == utils.normalize_hostname(server.hostname):
            return True
    raise ServerActionError('Server %s does not have role of %s running in '
                            'status primary.' % (hostname, role))
