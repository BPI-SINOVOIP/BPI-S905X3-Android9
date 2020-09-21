# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import math
import os
import random
import re
import sys
import time

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.frontend import database_settings_helper
from autotest_lib.tko import utils


def _log_error(msg):
    """Log an error message.

    @param msg: Message string
    """
    print >> sys.stderr, msg
    sys.stderr.flush()  # we want these msgs to show up immediately


def _format_operational_error(e):
    """Format OperationalError.

    @param e: OperationalError instance.
    """
    return ("%s: An operational error occurred during a database "
            "operation: %s" % (time.strftime("%X %x"), str(e)))


class MySQLTooManyRows(Exception):
    """Too many records."""
    pass


class db_sql(object):
    """Data access."""

    def __init__(self, debug=False, autocommit=True, host=None,
                 database=None, user=None, password=None):
        self.debug = debug
        self.autocommit = autocommit
        self._load_config(host, database, user, password)

        self.con = None
        self._init_db()

        # if not present, insert statuses
        self.status_idx = {}
        self.status_word = {}
        status_rows = self.select('status_idx, word', 'tko_status', None)
        for s in status_rows:
            self.status_idx[s[1]] = s[0]
            self.status_word[s[0]] = s[1]

        machine_map = os.path.join(os.path.dirname(__file__),
                                   'machines')
        if os.path.exists(machine_map):
            self.machine_map = machine_map
        else:
            self.machine_map = None
        self.machine_group = {}


    def _load_config(self, host, database, user, password):
        """Loads configuration settings required to connect to the database.

        This will try to connect to use the settings prefixed with global_db_.
        If they do not exist, they un-prefixed settings will be used.

        If parameters are supplied, these will be taken instead of the values
        in global_config.

        @param host: If set, this host will be used, if not, the host will be
                     retrieved from global_config.
        @param database: If set, this database will be used, if not, the
                         database will be retrieved from global_config.
        @param user: If set, this user will be used, if not, the
                         user will be retrieved from global_config.
        @param password: If set, this password will be used, if not, the
                         password will be retrieved from global_config.
        """
        database_settings = database_settings_helper.get_global_db_config()

        # grab the host, database
        self.host = host or database_settings['HOST']
        self.database = database or database_settings['NAME']

        # grab the user and password
        self.user = user or database_settings['USER']
        self.password = password or database_settings['PASSWORD']

        # grab the timeout configuration
        self.query_timeout =(
                database_settings.get('OPTIONS', {}).get('timeout', 3600))

        # Using fallback to non-global in order to work without configuration
        # overhead on non-shard instances.
        get_value = global_config.global_config.get_config_value_with_fallback
        self.min_delay = get_value("AUTOTEST_WEB", "global_db_min_retry_delay",
                                   "min_retry_delay", type=int, default=20)
        self.max_delay = get_value("AUTOTEST_WEB", "global_db_max_retry_delay",
                                   "max_retry_delay", type=int, default=60)

        # TODO(beeps): Move this to django settings once we have routers.
        # On test instances mysql connects through a different port. No point
        # piping this through our entire infrastructure when it is only really
        # used for testing; Ideally we would specify this through django
        # settings and default it to the empty string so django will figure out
        # the default based on the database backend (eg: mysql, 3306), but until
        # we have database routers in place any django settings will apply to
        # both tko and afe.
        # The intended use of this port is to allow a testing shard vm to
        # update the master vm's database with test results. Specifying
        # and empty string will fallback to not even specifying the port
        # to the backend in tko/db.py. Unfortunately this means retries
        # won't work on the test cluster till we've migrated to routers.
        self.port = global_config.global_config.get_config_value(
                "AUTOTEST_WEB", "global_db_port", type=str, default='')


    def _init_db(self):
        # make sure we clean up any existing connection
        if self.con:
            self.con.close()
            self.con = None

        # create the db connection and cursor
        self.con = self.connect(self.host, self.database,
                                self.user, self.password, self.port)
        self.cur = self.con.cursor()


    def _random_delay(self):
        delay = random.randint(self.min_delay, self.max_delay)
        time.sleep(delay)


    def run_with_retry(self, function, *args, **dargs):
        """Call function(*args, **dargs) until either it passes
        without an operational error, or a timeout is reached.
        This will re-connect to the database, so it is NOT safe
        to use this inside of a database transaction.

        It can be safely used with transactions, but the
        transaction start & end must be completely contained
        within the call to 'function'.

        @param function: The function to run with retry.
        @param args: The arguments
        @param dargs: The named arguments.
        """
        OperationalError = _get_error_class("OperationalError")

        success = False
        start_time = time.time()
        while not success:
            try:
                result = function(*args, **dargs)
            except OperationalError, e:
                _log_error("%s; retrying, don't panic yet"
                           % _format_operational_error(e))
                stop_time = time.time()
                elapsed_time = stop_time - start_time
                if elapsed_time > self.query_timeout:
                    raise
                else:
                    try:
                        self._random_delay()
                        self._init_db()
                    except OperationalError, e:
                        _log_error('%s; panic now'
                                   % _format_operational_error(e))
            else:
                success = True
        return result


    def dprint(self, value):
        """Print out debug value.

        @param value: The value to print out.
        """
        if self.debug:
            sys.stdout.write('SQL: ' + str(value) + '\n')


    def _commit(self):
        """Private method for function commit to call for retry.
        """
        return self.con.commit()


    def commit(self):
        """Commit the sql transaction."""
        if self.autocommit:
            return self.run_with_retry(self._commit)
        else:
            return self._commit()


    def rollback(self):
        """Rollback the sql transaction."""
        self.con.rollback()


    def get_last_autonumber_value(self):
        """Gets the last auto number.

        @return: The last auto number.
        """
        self.cur.execute('SELECT LAST_INSERT_ID()', [])
        return self.cur.fetchall()[0][0]


    def _quote(self, field):
        return '`%s`' % field


    def _where_clause(self, where):
        if not where:
            return '', []

        if isinstance(where, dict):
            # key/value pairs (which should be equal, or None for null)
            keys, values = [], []
            for field, value in where.iteritems():
                quoted_field = self._quote(field)
                if value is None:
                    keys.append(quoted_field + ' is null')
                else:
                    keys.append(quoted_field + '=%s')
                    values.append(value)
            where_clause = ' and '.join(keys)
        elif isinstance(where, basestring):
            # the exact string
            where_clause = where
            values = []
        elif isinstance(where, tuple):
            # preformatted where clause + values
            where_clause, values = where
            assert where_clause
        else:
            raise ValueError('Invalid "where" value: %r' % where)

        return ' WHERE ' + where_clause, values



    def select(self, fields, table, where, distinct=False, group_by=None,
               max_rows=None):
        """\
                This selects all the fields requested from a
                specific table with a particular where clause.
                The where clause can either be a dictionary of
                field=value pairs, a string, or a tuple of (string,
                a list of values).  The last option is what you
                should use when accepting user input as it'll
                protect you against sql injection attacks (if
                all user data is placed in the array rather than
                the raw SQL).

                For example:
                  where = ("a = %s AND b = %s", ['val', 'val'])
                is better than
                  where = "a = 'val' AND b = 'val'"

        @param fields: The list of selected fields string.
        @param table: The name of the database table.
        @param where: The where clause string.
        @param distinct: If select distinct values.
        @param group_by: Group by clause.
        @param max_rows: unused.
        """
        cmd = ['select']
        if distinct:
            cmd.append('distinct')
        cmd += [fields, 'from', table]

        where_clause, values = self._where_clause(where)
        cmd.append(where_clause)

        if group_by:
            cmd.append(' GROUP BY ' + group_by)

        self.dprint('%s %s' % (' '.join(cmd), values))

        # create a re-runable function for executing the query
        def exec_sql():
            """Exeuctes an the sql command."""
            sql = ' '.join(cmd)
            numRec = self.cur.execute(sql, values)
            if max_rows is not None and numRec > max_rows:
                msg = 'Exceeded allowed number of records'
                raise MySQLTooManyRows(msg)
            return self.cur.fetchall()

        # run the query, re-trying after operational errors
        if self.autocommit:
            return self.run_with_retry(exec_sql)
        else:
            return exec_sql()


    def select_sql(self, fields, table, sql, values):
        """\
                select fields from table "sql"

        @param fields: The list of selected fields string.
        @param table: The name of the database table.
        @param sql: The sql string.
        @param values: The sql string parameter values.
        """
        cmd = 'select %s from %s %s' % (fields, table, sql)
        self.dprint(cmd)

        # create a -re-runable function for executing the query
        def _exec_sql():
            self.cur.execute(cmd, values)
            return self.cur.fetchall()

        # run the query, re-trying after operational errors
        if self.autocommit:
            return self.run_with_retry(_exec_sql)
        else:
            return _exec_sql()


    def _exec_sql_with_commit(self, sql, values, commit):
        if self.autocommit:
            # re-run the query until it succeeds
            def _exec_sql():
                self.cur.execute(sql, values)
                self.con.commit()
            self.run_with_retry(_exec_sql)
        else:
            # take one shot at running the query
            self.cur.execute(sql, values)
            if commit:
                self.con.commit()


    def insert(self, table, data, commit=None):
        """\
                'insert into table (keys) values (%s ... %s)', values

                data:
                        dictionary of fields and data

        @param table: The name of the table.
        @param data: The insert data.
        @param commit: If commit the transaction .
        """
        fields = data.keys()
        refs = ['%s' for field in fields]
        values = [data[field] for field in fields]
        cmd = ('insert into %s (%s) values (%s)' %
               (table, ','.join(self._quote(field) for field in fields),
                ','.join(refs)))
        self.dprint('%s %s' % (cmd, values))

        self._exec_sql_with_commit(cmd, values, commit)


    def delete(self, table, where, commit = None):
        """Delete entries.

        @param table: The name of the table.
        @param where: The where clause.
        @param commit: If commit the transaction .
        """
        cmd = ['delete from', table]
        if commit is None:
            commit = self.autocommit
        where_clause, values = self._where_clause(where)
        cmd.append(where_clause)
        sql = ' '.join(cmd)
        self.dprint('%s %s' % (sql, values))

        self._exec_sql_with_commit(sql, values, commit)


    def update(self, table, data, where, commit = None):
        """\
                'update table set data values (%s ... %s) where ...'

                data:
                        dictionary of fields and data

        @param table: The name of the table.
        @param data: The sql parameter values.
        @param where: The where clause.
        @param commit: If commit the transaction .
        """
        if commit is None:
            commit = self.autocommit
        cmd = 'update %s ' % table
        fields = data.keys()
        data_refs = [self._quote(field) + '=%s' for field in fields]
        data_values = [data[field] for field in fields]
        cmd += ' set ' + ', '.join(data_refs)

        where_clause, where_values = self._where_clause(where)
        cmd += where_clause

        values = data_values + where_values
        self.dprint('%s %s' % (cmd, values))

        self._exec_sql_with_commit(cmd, values, commit)


    def delete_job(self, tag, commit = None):
        """Delete a tko job.

        @param tag: The job tag.
        @param commit: If commit the transaction .
        """
        job_idx = self.find_job(tag)
        for test_idx in self.find_tests(job_idx):
            where = {'test_idx' : test_idx}
            self.delete('tko_iteration_result', where)
            self.delete('tko_iteration_perf_value', where)
            self.delete('tko_iteration_attributes', where)
            self.delete('tko_test_attributes', where)
            self.delete('tko_test_labels_tests', {'test_id': test_idx})
        where = {'job_idx' : job_idx}
        self.delete('tko_tests', where)
        self.delete('tko_jobs', where)


    def insert_job(self, tag, job, parent_job_id=None, commit=None):
        """Insert a tko job.

        @param tag: The job tag.
        @param job: The job object.
        @param parent_job_id: The parent job id.
        @param commit: If commit the transaction .

        @return The dict of data inserted into the tko_jobs table.
        """
        job.machine_idx = self.lookup_machine(job.machine)
        if not job.machine_idx:
            job.machine_idx = self.insert_machine(job, commit=commit)
        elif job.machine:
            # Only try to update tko_machines record if machine is set. This
            # prevents unnecessary db writes for suite jobs.
            self.update_machine_information(job, commit=commit)

        afe_job_id = utils.get_afe_job_id(tag)

        data = {'tag':tag,
                'label': job.label,
                'username': job.user,
                'machine_idx': job.machine_idx,
                'queued_time': job.queued_time,
                'started_time': job.started_time,
                'finished_time': job.finished_time,
                'afe_job_id': afe_job_id,
                'afe_parent_job_id': parent_job_id,
                'build': job.build,
                'build_version': job.build_version,
                'board': job.board,
                'suite': job.suite}
        job.afe_job_id = afe_job_id
        if parent_job_id:
            job.afe_parent_job_id = str(parent_job_id)

        # TODO(ntang): check job.index directly.
        is_update = hasattr(job, 'index')
        if is_update:
            self.update('tko_jobs', data, {'job_idx': job.index}, commit=commit)
        else:
            self.insert('tko_jobs', data, commit=commit)
            job.index = self.get_last_autonumber_value()
        self.update_job_keyvals(job, commit=commit)
        for test in job.tests:
            self.insert_test(job, test, commit=commit)

        data['job_idx'] = job.index
        return data


    def update_job_keyvals(self, job, commit=None):
        """Updates the job key values.

        @param job: The job object.
        @param commit: If commit the transaction .
        """
        for key, value in job.keyval_dict.iteritems():
            where = {'job_id': job.index, 'key': key}
            data = dict(where, value=value)
            exists = self.select('id', 'tko_job_keyvals', where=where)

            if exists:
                self.update('tko_job_keyvals', data, where=where, commit=commit)
            else:
                self.insert('tko_job_keyvals', data, commit=commit)


    def insert_test(self, job, test, commit = None):
        """Inserts a job test.

        @param job: The job object.
        @param test: The test object.
        @param commit: If commit the transaction .
        """
        kver = self.insert_kernel(test.kernel, commit=commit)
        data = {'job_idx':job.index, 'test':test.testname,
                'subdir':test.subdir, 'kernel_idx':kver,
                'status':self.status_idx[test.status],
                'reason':test.reason, 'machine_idx':job.machine_idx,
                'started_time': test.started_time,
                'finished_time':test.finished_time}
        is_update = hasattr(test, "test_idx")
        if is_update:
            test_idx = test.test_idx
            self.update('tko_tests', data,
                        {'test_idx': test_idx}, commit=commit)
            where = {'test_idx': test_idx}
            self.delete('tko_iteration_result', where)
            self.delete('tko_iteration_perf_value', where)
            self.delete('tko_iteration_attributes', where)
            where['user_created'] = 0
            self.delete('tko_test_attributes', where)
        else:
            self.insert('tko_tests', data, commit=commit)
            test_idx = test.test_idx = self.get_last_autonumber_value()
        data = {'test_idx': test_idx}

        for i in test.iterations:
            data['iteration'] = i.index
            for key, value in i.attr_keyval.iteritems():
                data['attribute'] = key
                data['value'] = value
                self.insert('tko_iteration_attributes', data,
                            commit=commit)
            for key, value in i.perf_keyval.iteritems():
                data['attribute'] = key
                if math.isnan(value) or math.isinf(value):
                    data['value'] = None
                else:
                    data['value'] = value
                self.insert('tko_iteration_result', data,
                            commit=commit)

        data = {'test_idx': test_idx}

        for key, value in test.attributes.iteritems():
            data = {'test_idx': test_idx, 'attribute': key,
                    'value': value}
            self.insert('tko_test_attributes', data, commit=commit)

        if not is_update:
            for label_index in test.labels:
                data = {'test_id': test_idx, 'testlabel_id': label_index}
                self.insert('tko_test_labels_tests', data, commit=commit)


    def read_machine_map(self):
        """Reads the machine map."""
        if self.machine_group or not self.machine_map:
            return
        for line in open(self.machine_map, 'r').readlines():
            (machine, group) = line.split()
            self.machine_group[machine] = group


    def machine_info_dict(self, job):
        """Reads the machine information of a job.

        @param job: The job object.

        @return: The machine info dictionary.
        """
        hostname = job.machine
        group = job.machine_group
        owner = job.machine_owner

        if not group:
            self.read_machine_map()
            group = self.machine_group.get(hostname, hostname)
            if group == hostname and owner:
                group = owner + '/' + hostname

        return {'hostname': hostname, 'machine_group': group, 'owner': owner}


    def insert_machine(self, job, commit = None):
        """Inserts the job machine.

        @param job: The job object.
        @param commit: If commit the transaction .
        """
        machine_info = self.machine_info_dict(job)
        self.insert('tko_machines', machine_info, commit=commit)
        return self.get_last_autonumber_value()


    def update_machine_information(self, job, commit = None):
        """Updates the job machine information.

        @param job: The job object.
        @param commit: If commit the transaction .
        """
        machine_info = self.machine_info_dict(job)
        self.update('tko_machines', machine_info,
                    where={'hostname': machine_info['hostname']},
                    commit=commit)


    def lookup_machine(self, hostname):
        """Look up the machine information.

        @param hostname: The hostname as string.
        """
        where = { 'hostname' : hostname }
        rows = self.select('machine_idx', 'tko_machines', where)
        if rows:
            return rows[0][0]
        else:
            return None


    def lookup_kernel(self, kernel):
        """Look up the kernel.

        @param kernel: The kernel object.
        """
        rows = self.select('kernel_idx', 'tko_kernels',
                                {'kernel_hash':kernel.kernel_hash})
        if rows:
            return rows[0][0]
        else:
            return None


    def insert_kernel(self, kernel, commit = None):
        """Insert a kernel.

        @param kernel: The kernel object.
        @param commit: If commit the transaction .
        """
        kver = self.lookup_kernel(kernel)
        if kver:
            return kver

        # If this kernel has any significant patches, append their hash
        # as diferentiator.
        printable = kernel.base
        patch_count = 0
        for patch in kernel.patches:
            match = re.match(r'.*(-mm[0-9]+|-git[0-9]+)\.(bz2|gz)$',
                                                    patch.reference)
            if not match:
                patch_count += 1

        self.insert('tko_kernels',
                    {'base':kernel.base,
                     'kernel_hash':kernel.kernel_hash,
                     'printable':printable},
                    commit=commit)
        kver = self.get_last_autonumber_value()

        if patch_count > 0:
            printable += ' p%d' % (kver)
            self.update('tko_kernels',
                    {'printable':printable},
                    {'kernel_idx':kver})

        for patch in kernel.patches:
            self.insert_patch(kver, patch, commit=commit)
        return kver


    def insert_patch(self, kver, patch, commit = None):
        """Insert a kernel patch.

        @param kver: The kernel version.
        @param patch: The kernel patch object.
        @param commit: If commit the transaction .
        """
        print patch.reference
        name = os.path.basename(patch.reference)[:80]
        self.insert('tko_patches',
                    {'kernel_idx': kver,
                     'name':name,
                     'url':patch.reference,
                     'hash':patch.hash},
                    commit=commit)


    def find_test(self, job_idx, testname, subdir):
        """Find a test by name.

        @param job_idx: The job index.
        @param testname: The test name.
        @param subdir: The test sub directory under the job directory.
        """
        where = {'job_idx': job_idx , 'test': testname, 'subdir': subdir}
        rows = self.select('test_idx', 'tko_tests', where)
        if rows:
            return rows[0][0]
        else:
            return None


    def find_tests(self, job_idx):
        """Find all tests by job index.

        @param job_idx: The job index.
        @return: A list of tests.
        """
        where = { 'job_idx':job_idx }
        rows = self.select('test_idx', 'tko_tests', where)
        if rows:
            return [row[0] for row in rows]
        else:
            return []


    def find_job(self, tag):
        """Find a job by tag.

        @param tag: The job tag name.
        @return: The job object or None.
        """
        rows = self.select('job_idx', 'tko_jobs', {'tag': tag})
        if rows:
            return rows[0][0]
        else:
            return None


def _get_db_type():
    """Get the database type name to use from the global config."""
    get_value = global_config.global_config.get_config_value_with_fallback
    return "db_" + get_value("AUTOTEST_WEB", "global_db_type", "db_type",
                             default="mysql")


def _get_error_class(class_name):
    """Retrieves the appropriate error class by name from the database
    module."""
    db_module = __import__("autotest_lib.tko." + _get_db_type(),
                           globals(), locals(), ["driver"])
    return getattr(db_module.driver, class_name)


def db(*args, **dargs):
    """Creates an instance of the database class with the arguments
    provided in args and dargs, using the database type specified by
    the global configuration (defaulting to mysql).

    @param args: The db_type arguments.
    @param dargs: The db_type named arguments.

    @return: An db object.
    """
    db_type = _get_db_type()
    db_module = __import__("autotest_lib.tko." + db_type, globals(),
                           locals(), [db_type])
    db = getattr(db_module, db_type)(*args, **dargs)
    return db
