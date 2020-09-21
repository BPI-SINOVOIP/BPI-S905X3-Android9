#!/usr/bin/python

from __future__ import print_function

import argparse
import logging
import multiprocessing
import subprocess
import sys

import common
from autotest_lib.server import frontend
from autotest_lib.site_utils.lib import infra

DEPLOY_SERVER_LOCAL = ('/usr/local/autotest/site_utils/deploy_server_local.py')
POOL_SIZE = 124


def _filter_servers(servers):
    """Filter a set of servers to those that should be deployed to."""
    non_push_roles = {'devserver', 'crash_server', 'reserve'}
    for s in servers:
        if s['status'] == 'repair_required':
            continue
        if s['status'] == 'backup':
            continue
        if set(s['roles']) & non_push_roles:
            continue
        yield s


def discover_servers(afe):
    """Discover the in-production servers to update.

    Returns the set of servers from serverdb that are in production and should
    be updated. This filters out servers in need of repair, or servers of roles
    that are not yet supported by deploy_server / deploy_server_local.

    @param afe: Server to contact with RPC requests.

    @returns: A set of server hostnames.
    """
    # Example server details....
    # {
    #     'hostname': 'server1',
    #     'status': 'backup',
    #     'roles': ['drone', 'scheduler'],
    #     'attributes': {'max_processes': 300}
    # }
    rpc = frontend.AFE(server=afe)
    servers = rpc.run('get_servers')

    return {s['hostname'] for s in _filter_servers(servers)}


def _parse_arguments(args):
    """Parse command line arguments.

    @param args: The command line arguments to parse. (usually sys.argv[1:])

    @returns A tuple of (argparse.Namespace populated with argument values,
                         list of extra args to pass to deploy_server_local).
    """
    parser = argparse.ArgumentParser(
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description='Run deploy_server_local on a bunch of servers. Extra '
                        'arguments will be passed through.',
            epilog=('Update all servers:\n'
                    '  deploy_server.py -x --afe cautotest\n'
                    '\n'
                    'Update one server:\n'
                    '  deploy_server.py <server> -x\n'
                    ))

    parser.add_argument('-x', action='store_true',
                        help='Actually perform actions. If not supplied, '
                             'script does nothing.')
    parser.add_argument('--afe',
            help='The AFE server used to get servers from server_db,'
                 'e.g, cautotest. Used only if no SERVER specified.')
    parser.add_argument('servers', action='store', nargs='*', metavar='SERVER')

    return parser.parse_known_args()


def _update_server(server, extra_args=[]):
    """Run deploy_server_local for given server.

    @param server: hostname to update.
    @param extra_args: args to be passed in to deploy_server_local.

    @return: A tuple of (server, success, output), where:
             server: Name of the server.
             sucess: True if update succeeds, False otherwise.
             output: A string of the deploy_server_local script output
                     including any errors.
    """
    cmd = ('%s %s' %
           (DEPLOY_SERVER_LOCAL, ' '.join(extra_args)))
    success = False
    try:
        output = infra.execute_command(server, cmd)
        success = True
    except subprocess.CalledProcessError as e:
        output = e.output

    return server, success, output

def _update_in_parallel(servers, extra_args=[]):
    """Update a group of servers in parallel.

    @param servers: A list of servers to update.
    @param options: Options for the push.

    @returns A dictionary from server names that failed to the output
             of the update script.
    """
    # Create a list to record all the finished servers.
    manager = multiprocessing.Manager()
    finished_servers = manager.list()

    do_server = lambda s: _update_server(s, extra_args)

    # The update actions run in parallel. If any update failed, we should wait
    # for other running updates being finished. Abort in the middle of an update
    # may leave the server in a bad state.
    pool = multiprocessing.pool.ThreadPool(POOL_SIZE)
    try:
        results = pool.map_async(do_server, servers)
        pool.close()

        # Track the updating progress for current group of servers.
        incomplete_servers = set()
        server_names = set([s[0] for s in servers])
        while not results.ready():
            incomplete_servers = sorted(set(servers) - set(finished_servers))
            print('Not finished yet. %d servers in this group. '
                '%d servers are still running:\n%s\n' %
                (len(servers), len(incomplete_servers), incomplete_servers))
            # Check the progress every 20s
            results.wait(20)

        # After update finished, parse the result.
        failures = {}
        for server, success, output in results.get():
            if not success:
                failures[server] = output

        return failures

    finally:
        pool.terminate()
        pool.join()


def main(args):
    """Entry point to deploy_server.py

    @param args: The command line arguments to parse. (usually sys.argv)

    @returns The system exit code.
    """
    options, extra_args = _parse_arguments(args[1:])
    # Remove all the handlers from the root logger to get rid of the handlers
    # introduced by the import packages.
    logging.getLogger().handlers = []
    logging.basicConfig(level=logging.DEBUG)

    servers = options.servers
    if not servers:
        if not options.afe:
            print('No servers or afe specified. Aborting')
            return 1
        print('Retrieving servers from %s..' % options.afe)
        servers = discover_servers(options.afe)
        print('Retrieved servers were: %s' % servers)

    if not options.x:
        print('Doing nothing because -x was not supplied.')
        print('servers: %s' % options.servers)
        print('extra args for deploy_server_local: %s' % extra_args)
        return 0

    failures = _update_in_parallel(servers, extra_args)

    if not failures:
        print('Completed all updates successfully.')
        return 0

    print('The following servers failed, with the following output:')
    for s, o in failures.iteritems():
        print('======== %s ========' % s)
        print(o)

    print('The servers that failed were:')
    print('\n'.join(failures.keys()))
    print('\n\nTo retry on failed servers, run the following command:')
    retry_cmd = [args[0], '-x'] + failures.keys() + extra_args
    print(' '.join(retry_cmd))
    return 1



if __name__ == '__main__':
    sys.exit(main(sys.argv))
