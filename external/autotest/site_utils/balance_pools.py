#!/usr/bin/env python
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Adjust pool balances to cover DUT shortfalls.

This command takes all broken DUTs in a specific pool for specific
models and swaps them with working DUTs taken from a selected pool
of spares.  The command is meant primarily for replacing broken DUTs
in critical pools like BVT or CQ, but it can also be used to adjust
pool sizes, or to create or remove pools.

usage:  balance_pool.py [ options ] POOL MODEL [ MODEL ... ]

positional arguments:
  POOL                  Name of the pool to balance
  MODEL                 Names of models to balance

optional arguments:
  -h, --help            show this help message and exit
  -t COUNT, --total COUNT
                        Set the number of DUTs in the pool to the specified
                        count for every MODEL
  -a COUNT, --grow COUNT
                        Add the specified number of DUTs to the pool for every
                        MODEL
  -d COUNT, --shrink COUNT
                        Remove the specified number of DUTs from the pool for
                        every MODEL
  -s POOL, --spare POOL
                        Pool from which to draw replacement spares (default:
                        pool:suites)
  --sku SKU             The specific SKU we intend to swap with
  -n, --dry-run         Report actions to take in the form of shell commands


The command attempts to remove all broken DUTs from the target POOL
for every MODEL, and replace them with enough working DUTs taken
from the spare pool to bring the strength of POOL to the requested
total COUNT.

If no COUNT options are supplied (i.e. there are no --total, --grow,
or --shrink options), the command will maintain the current totals of
DUTs for every MODEL in the target POOL.

If not enough working spares are available, broken DUTs may be left
in the pool to keep the pool at the target COUNT.

When reducing pool size, working DUTs will be returned after broken
DUTs, if it's necessary to achieve the target COUNT.

"""


import argparse
import sys
import time

import common
from autotest_lib.server import constants
from autotest_lib.server import frontend
from autotest_lib.server import site_utils
from autotest_lib.server.lib import status_history
from autotest_lib.site_utils import lab_inventory
from autotest_lib.utils import labellib
from chromite.lib import metrics
from chromite.lib import parallel

#This must be imported after chromite.lib.metrics
from infra_libs import ts_mon

_POOL_PREFIX = constants.Labels.POOL_PREFIX
# This is the ratio of all models we should calculate the default max
# number of broken models against.  It seemed like the best choice that
# was neither too strict nor lax.
_MAX_BROKEN_DEFAULT_RATIO = 3.0 / 8.0

_ALL_CRITICAL_POOLS = 'all_critical_pools'
_SPARE_DEFAULT = lab_inventory.SPARE_POOL


def _log_message(message, *args):
    """Log a message with optional format arguments to stdout.

    This function logs a single line to stdout, with formatting
    if necessary, and without adornments.

    If `*args` are supplied, the message will be formatted using
    the arguments.

    @param message  Message to be logged, possibly after formatting.
    @param args     Format arguments.  If empty, the message is logged
                    without formatting.

    """
    if args:
        message = message % args
    sys.stdout.write('%s\n' % message)


def _log_info(dry_run, message, *args):
    """Log information in a dry-run dependent fashion.

    This function logs a single line to stdout, with formatting
    if necessary.  When logging for a dry run, the message is
    printed as a shell comment, rather than as unadorned text.

    If `*args` are supplied, the message will be formatted using
    the arguments.

    @param message  Message to be logged, possibly after formatting.
    @param args     Format arguments.  If empty, the message is logged
                    without formatting.

    """
    if dry_run:
        message = '# ' + message
    _log_message(message, *args)


def _log_error(message, *args):
    """Log an error to stderr, with optional format arguments.

    This function logs a single line to stderr, prefixed to indicate
    that it is an error message.

    If `*args` are supplied, the message will be formatted using
    the arguments.

    @param message  Message to be logged, possibly after formatting.
    @param args     Format arguments.  If empty, the message is logged
                    without formatting.

    """
    if args:
        message = message % args
    sys.stderr.write('ERROR: %s\n' % message)


class _DUTPool(object):
    """Information about a pool of DUTs matching given labels.

    This class collects information about all DUTs for a given pool and matching
    the given labels, and divides them into three categories:
      + Working - the DUT is working for testing, and not locked.
      + Broken - the DUT is unable to run tests, or it is locked.
      + Ineligible - the DUT is not available to be removed from this pool.  The
            DUT may be either working or broken.

    DUTs with more than one pool: label are ineligible for exchange
    during balancing.  This is done for the sake of chameleon hosts,
    which must always be assigned to pool:suites.  These DUTs are
    always marked with pool:chameleon to prevent their reassignment.

    TODO(jrbarnette):  The use of `pool:chamelon` (instead of just
    the `chameleon` label is a hack that should be eliminated.

    _DUTPool instances are used to track both main pools that need
    to be resupplied with working DUTs and spare pools that supply
    those DUTs.

    @property pool                Name of the pool associated with
                                  this pool of DUTs.
    @property labels              Labels that constrain the DUTs to consider.
    @property working_hosts       The list of this pool's working DUTs.
    @property broken_hosts        The list of this pool's broken DUTs.
    @property ineligible_hosts    The list of this pool's ineligible DUTs.
    @property pool_labels         A list of labels that identify a DUT as part
                                  of this pool.
    @property total_hosts         The total number of hosts in pool.

    """

    def __init__(self, afe, pool, labels, start_time, end_time):
        self.pool = pool
        self.labels = labellib.LabelsMapping(labels)
        self.labels['pool'] = pool
        self._pool_labels = [_POOL_PREFIX + self.pool]

        self.working_hosts = []
        self.broken_hosts = []
        self.ineligible_hosts = []
        self.total_hosts = self._get_hosts(afe, start_time, end_time)


    def _get_hosts(self, afe, start_time, end_time):
        all_histories = status_history.HostJobHistory.get_multiple_histories(
                afe, start_time, end_time, self.labels.getlabels())
        for h in all_histories:
            host = h.host
            host_pools = [l for l in host.labels
                          if l.startswith(_POOL_PREFIX)]
            if len(host_pools) != 1:
                self.ineligible_hosts.append(host)
            else:
                diag = h.last_diagnosis()[0]
                if (diag == status_history.WORKING and
                        not host.locked):
                    self.working_hosts.append(host)
                else:
                    self.broken_hosts.append(host)
        return len(all_histories)


    @property
    def pool_labels(self):
        """Return the AFE labels that identify this pool.

        The returned labels are the labels that must be removed
        to remove a DUT from the pool, or added to add a DUT.

        @return A list of AFE labels suitable for AFE.add_labels()
                or AFE.remove_labels().

        """
        return self._pool_labels

    def calculate_spares_needed(self, target_total):
        """Calculate and log the spares needed to achieve a target.

        Return how many working spares are needed to achieve the
        given `target_total` with all DUTs working.

        The spares count may be positive or negative.  Positive
        values indicate spares are needed to replace broken DUTs in
        order to reach the target; negative numbers indicate that
        no spares are needed, and that a corresponding number of
        working devices can be returned.

        If the new target total would require returning ineligible
        DUTs, an error is logged, and the target total is adjusted
        so that those DUTs are not exchanged.

        @param target_total  The new target pool size.

        @return The number of spares needed.

        """
        num_ineligible = len(self.ineligible_hosts)
        spares_needed = target_total >= num_ineligible
        metrics.Boolean(
            'chromeos/autotest/balance_pools/exhausted_pools',
            'True for each pool/model which requests more DUTs than supplied',
            # TODO(jrbarnette) The 'board' field is a legacy.  We need
            # to leave it here until we do the extra work Monarch
            # requires to delete a field.
            field_spec=[
                    ts_mon.StringField('pool'),
                    ts_mon.StringField('board'),
                    ts_mon.StringField('model'),
            ]).set(
                    not spares_needed,
                    fields={
                            'pool': self.pool,
                            'board': self.labels.get('model', ''),
                            'model': self.labels.get('model', ''),
                    },
        )
        if not spares_needed:
            _log_error(
                    '%s pool (%s): Target of %d is below minimum of %d DUTs.',
                    self.pool, self.labels, target_total, num_ineligible,
            )
            _log_error('Adjusting target to %d DUTs.', num_ineligible)
            target_total = num_ineligible
        else:
            _log_message('%s %s pool: Target of %d is above minimum.',
                         self.labels.get('model', ''), self.pool, target_total)
        adjustment = target_total - self.total_hosts
        return len(self.broken_hosts) + adjustment

    def allocate_surplus(self, num_broken):
        """Allocate a list DUTs that can returned as surplus.

        Return a list of devices that can be returned in order to
        reduce this pool's supply.  Broken DUTs will be preferred
        over working ones.

        The `num_broken` parameter indicates the number of broken
        DUTs to be left in the pool.  If this number exceeds the
        number of broken DUTs actually in the pool, the returned
        list will be empty.  If this number is negative, it
        indicates a number of working DUTs to be returned in
        addition to all broken ones.

        @param num_broken    Total number of broken DUTs to be left in
                             this pool.

        @return A list of DUTs to be returned as surplus.

        """
        if num_broken >= 0:
            surplus = self.broken_hosts[num_broken:]
            return surplus
        else:
            return (self.broken_hosts +
                    self.working_hosts[:-num_broken])


def _exchange_labels(dry_run, hosts, target_pool, spare_pool):
    """Reassign a list of DUTs from one pool to another.

    For all the given hosts, remove all labels associated with
    `spare_pool`, and add the labels for `target_pool`.

    If `dry_run` is true, perform no changes, but log the `atest`
    commands needed to accomplish the necessary label changes.

    @param dry_run       Whether the logging is for a dry run or
                         for actual execution.
    @param hosts         List of DUTs (AFE hosts) to be reassigned.
    @param target_pool   The `_DUTPool` object from which the hosts
                         are drawn.
    @param spare_pool    The `_DUTPool` object to which the hosts
                         will be added.

    """
    _log_info(dry_run, 'Transferring %d DUTs from %s to %s.',
              len(hosts), spare_pool.pool, target_pool.pool)
    metrics.Counter(
        'chromeos/autotest/balance_pools/duts_moved',
        'DUTs transferred between pools',
        # TODO(jrbarnette) The 'board' field is a legacy.  We need to
        # leave it here until we do the extra work Monarch requires to
        # delete a field.
        field_spec=[
                ts_mon.StringField('board'),
                ts_mon.StringField('model'),
                ts_mon.StringField('source_pool'),
                ts_mon.StringField('target_pool'),
        ]
    ).increment_by(
            len(hosts),
            fields={
                    'board': target_pool.labels.get('model', ''),
                    'model': target_pool.labels.get('model', ''),
                    'source_pool': spare_pool.pool,
                    'target_pool': target_pool.pool,
            },
    )
    if not hosts:
        return

    additions = target_pool.pool_labels
    removals = spare_pool.pool_labels
    for host in hosts:
        if not dry_run:
            _log_message('Updating host: %s.', host.hostname)
            host.remove_labels(removals)
            host.add_labels(additions)
        else:
            _log_message('atest label remove -m %s %s',
                         host.hostname, ' '.join(removals))
            _log_message('atest label add -m %s %s',
                         host.hostname, ' '.join(additions))


def _balance_model(arguments, afe, pool, labels, start_time, end_time):
    """Balance one model as requested by command line arguments.

    @param arguments     Parsed command line arguments.
    @param afe           AFE object to be used for the changes.
    @param pool          Pool of the model to be balanced.
    @param labels        Restrict the balancing operation within DUTs
                         that have these labels.
    @param start_time    Start time for HostJobHistory objects in
                         the DUT pools.
    @param end_time      End time for HostJobHistory objects in the
                         DUT pools.

    """
    spare_pool = _DUTPool(afe, arguments.spare, labels, start_time, end_time)
    main_pool = _DUTPool(afe, pool, labels, start_time, end_time)

    target_total = main_pool.total_hosts
    if arguments.total is not None:
        target_total = arguments.total
    elif arguments.grow:
        target_total += arguments.grow
    elif arguments.shrink:
        target_total -= arguments.shrink

    spares_needed = main_pool.calculate_spares_needed(target_total)
    if spares_needed > 0:
        spare_duts = spare_pool.working_hosts[:spares_needed]
        shortfall = spares_needed - len(spare_duts)
    else:
        spare_duts = []
        shortfall = spares_needed

    surplus_duts = main_pool.allocate_surplus(shortfall)

    if spares_needed or surplus_duts or arguments.verbose:
        dry_run = arguments.dry_run
        _log_message('')

        _log_info(dry_run, 'Balancing %s %s pool:', labels, main_pool.pool)
        _log_info(dry_run,
                  'Total %d DUTs, %d working, %d broken, %d reserved.',
                  main_pool.total_hosts, len(main_pool.working_hosts),
                  len(main_pool.broken_hosts), len(main_pool.ineligible_hosts))

        if spares_needed > 0:
            add_msg = 'grow pool by %d DUTs' % spares_needed
        elif spares_needed < 0:
            add_msg = 'shrink pool by %d DUTs' % -spares_needed
        else:
            add_msg = 'no change to pool size'
        _log_info(dry_run, 'Target is %d working DUTs; %s.',
                  target_total, add_msg)

        _log_info(dry_run,
                  '%s %s pool has %d spares available for balancing pool %s',
                  labels, spare_pool.pool, len(spare_pool.working_hosts),
                  main_pool.pool)

        if spares_needed > len(spare_duts):
            _log_error('Not enough spares: need %d, only have %d.',
                       spares_needed, len(spare_duts))
        elif shortfall >= 0:
            _log_info(dry_run,
                      '%s %s pool will return %d broken DUTs, '
                      'leaving %d still in the pool.',
                      labels, main_pool.pool,
                      len(surplus_duts),
                      len(main_pool.broken_hosts) - len(surplus_duts))
        else:
            _log_info(dry_run,
                      '%s %s pool will return %d surplus DUTs, '
                      'including %d working DUTs.',
                      labels, main_pool.pool,
                      len(main_pool.broken_hosts) - shortfall,
                      -shortfall)

    if (len(main_pool.broken_hosts) > arguments.max_broken and
        not arguments.force_rebalance):
        _log_error('%s %s pool: Refusing to act on pool with %d broken DUTs.',
                   labels, main_pool.pool, len(main_pool.broken_hosts))
        _log_error('Please investigate this model to for a bug ')
        _log_error('that is bricking devices. Once you have finished your ')
        _log_error('investigation, you can force a rebalance with ')
        _log_error('--force-rebalance')
        spare_duts = []
        surplus_duts = []

    if not spare_duts and not surplus_duts:
        if arguments.verbose:
            _log_info(arguments.dry_run, 'No exchange required.')

    _exchange_labels(arguments.dry_run, surplus_duts,
                     spare_pool, main_pool)
    _exchange_labels(arguments.dry_run, spare_duts,
                     main_pool, spare_pool)


def _too_many_broken(inventory, pool, args):
    """
    Get the inventory of models and check if too many are broken.

    @param inventory: _LabInventory object.
    @param pool: The pool to check.
    @param args: Parsed command line arguments.

    @return True if the number of models with 1 or more broken duts
            exceed max_broken_models, False otherwise.
    """
    # Were we asked to skip this check?
    if (args.force_rebalance or
            (args.all_models and args.max_broken_models == 0)):
        return False

    max_broken = args.max_broken_models
    if max_broken is None:
        total_num = len(inventory.get_pool_models(pool))
        max_broken = int(_MAX_BROKEN_DEFAULT_RATIO * total_num)
    _log_info(args.dry_run,
              'Max broken models for pool %s: %d',
              pool, max_broken)

    broken = [model for model, counts in inventory.iteritems()
                  if counts.get_broken(pool) != 0]
    _log_message('There are %d models in the %s pool with at least 1 '
                 'broken DUT (max threshold %d)',
                 len(broken), pool, max_broken)
    for b in sorted(broken):
        _log_message(b)
    return len(broken) > max_broken


def _parse_command(argv):
    """Parse the command line arguments.

    Create an argument parser for this command's syntax, parse the
    command line, and return the result of the `ArgumentParser`
    `parse_args()` method.

    @param argv Standard command line argument vector; `argv[0]` is
                assumed to be the command name.

    @return Result returned by `ArgumentParser.parse_args()`.

    """
    parser = argparse.ArgumentParser(
            prog=argv[0],
            description='Balance pool shortages from spares on reserve')

    parser.add_argument(
        '-w', '--web', type=str, default=None,
        help='AFE host to use. Default comes from shadow_config.',
    )
    count_group = parser.add_mutually_exclusive_group()
    count_group.add_argument('-t', '--total', type=int,
                             metavar='COUNT', default=None,
                             help='Set the number of DUTs in the '
                                  'pool to the specified count for '
                                  'every MODEL')
    count_group.add_argument('-a', '--grow', type=int,
                             metavar='COUNT', default=None,
                             help='Add the specified number of DUTs '
                                  'to the pool for every MODEL')
    count_group.add_argument('-d', '--shrink', type=int,
                             metavar='COUNT', default=None,
                             help='Remove the specified number of DUTs '
                                  'from the pool for every MODEL')

    parser.add_argument('-s', '--spare', default=_SPARE_DEFAULT,
                        metavar='POOL',
                        help='Pool from which to draw replacement '
                             'spares (default: pool:%s)' % _SPARE_DEFAULT)
    parser.add_argument('-n', '--dry-run', action='store_true',
                        help='Report actions to take in the form of '
                             'shell commands')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print more detail about calculations for debug '
                             'purposes.')

    parser.add_argument('-m', '--max-broken', default=2, type=int,
                        metavar='COUNT',
                        help='Only rebalance a pool if it has at most '
                             'COUNT broken DUTs.')
    parser.add_argument('-f', '--force-rebalance', action='store_true',
                        help='Forcefully rebalance all DUTs in a pool, even '
                             'if it has a large number of broken DUTs. '
                             'Before doing this, please investigate whether '
                             'there is a bug that is bricking devices in the '
                             'lab.')
    parser.add_argument('--production', action='store_true',
                        help='Treat this as a production run. This will '
                             'collect metrics.')

    parser.add_argument(
            '--all-models',
            action='store_true',
            help='Rebalance all managed models.  This will do a very expensive '
                 'check to see how many models have at least one broken DUT. '
                 'To bypass that check, set --max-broken-models to 0.',
    )
    parser.add_argument(
            '--max-broken-models', default=None, type=int, metavar='COUNT',
            help='Only rebalance all models if number of models with broken '
                 'DUTs in the specified pool is less than COUNT.',
    )

    parser.add_argument('pool',
                        metavar='POOL',
                        help='Name of the pool to balance.  Use %s to balance '
                             'all critical pools' % _ALL_CRITICAL_POOLS)
    parser.add_argument('models', nargs='*', metavar='MODEL',
                        help='Names of models to balance.')

    parser.add_argument('--sku', type=str,
                        help='Optional name of sku to restrict to.')

    arguments = parser.parse_args(argv[1:])

    # Error-check arguments.
    if arguments.models and arguments.all_models:
        parser.error('Cannot specify individual models on the command line '
                     'when using --all-models.')
    if (arguments.pool == _ALL_CRITICAL_POOLS and
        arguments.spare != _SPARE_DEFAULT):
        parser.error('Cannot specify --spare pool to be %s when balancing all '
                     'critical pools.' % _SPARE_DEFAULT)
    return arguments


def infer_balancer_targets(afe, arguments, pools):
    """Take some arguments and translate them to a list of models to balance

    Args:
    @param afe           AFE object to be used for taking inventory.
    @param arguments     Parsed command line arguments.
    @param pools         The list of pools to balance.

    @returns    a list of (model, labels) tuples to be balanced

    """
    balancer_targets = []

    for pool in pools:
        if arguments.all_models:
            inventory = lab_inventory.get_inventory(afe)
            quarantine = _too_many_broken(inventory, pool, arguments)
            if quarantine:
                _log_error('Refusing to balance all models for %s pool, '
                           'too many models with at least 1 broken DUT '
                           'detected.', pool)
            else:
                for model in inventory.get_models(pool):
                    labels = labellib.LabelsMapping()
                    labels['model'] = model
                    balancer_targets.append((pool, labels.getlabels()))
            metrics.Boolean(
                'chromeos/autotest/balance_pools/unchanged_pools').set(
                    quarantine, fields={'pool': pool})
            _log_message('Pool %s quarantine status: %s', pool, quarantine)
        else:
            for model in arguments.models:
                labels = labellib.LabelsMapping()
                labels['model'] = model
                if arguments.sku:
                    labels['sku'] = arguments.sku
                balancer_targets.append((pool, labels.getlabels()))
    return balancer_targets


def main(argv):
    """Standard main routine.

    @param argv  Command line arguments including `sys.argv[0]`.

    """
    arguments = _parse_command(argv)
    if arguments.production:
        metrics_manager = site_utils.SetupTsMonGlobalState(
                'balance_pools',
                indirect=False,
                auto_flush=False,
        )
    else:
        metrics_manager = site_utils.TrivialContextManager()

    with metrics_manager:
        end_time = time.time()
        start_time = end_time - 24 * 60 * 60
        afe = frontend.AFE(server=arguments.web)

        def balancer(pool, labels):
            """Balance the specified model.

            @param pool: The pool to rebalance for the model.
            @param labels: labels to restrict to balancing operations
                    within.
            """
            _balance_model(arguments, afe, pool, labels,
                           start_time, end_time)
            _log_message('')

        pools = (lab_inventory.CRITICAL_POOLS
                if arguments.pool == _ALL_CRITICAL_POOLS
                else [arguments.pool])
        balancer_targets = infer_balancer_targets(afe, arguments, pools)
        try:
            parallel.RunTasksInProcessPool(
                    balancer,
                    balancer_targets,
                    processes=8,
            )
        except KeyboardInterrupt:
            pass
        finally:
            metrics.Flush()


if __name__ == '__main__':
    main(sys.argv)
