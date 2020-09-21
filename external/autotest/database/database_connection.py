# pylint: disable-msg=C0111

import re, time, traceback
import common
from autotest_lib.client.common_lib import global_config

RECONNECT_FOREVER = object()

_DB_EXCEPTIONS = ('DatabaseError', 'OperationalError', 'ProgrammingError')
_GLOBAL_CONFIG_NAMES = {
    'username' : 'user',
    'db_name' : 'database',
}

def _copy_exceptions(source, destination):
    for exception_name in _DB_EXCEPTIONS:
        try:
            setattr(destination, exception_name,
                    getattr(source, exception_name))
        except AttributeError:
            # Under the django backend:
            # Django 1.3 does not have OperationalError and ProgrammingError.
            # Let's just mock these classes with the base DatabaseError.
            setattr(destination, exception_name,
                    getattr(source, 'DatabaseError'))


class _GenericBackend(object):
    def __init__(self, database_module):
        self._database_module = database_module
        self._connection = None
        self._cursor = None
        self.rowcount = None
        _copy_exceptions(database_module, self)


    def connect(self, host=None, username=None, password=None, db_name=None):
        """
        This is assumed to enable autocommit.
        """
        raise NotImplementedError


    def disconnect(self):
        if self._connection:
            self._connection.close()
        self._connection = None
        self._cursor = None


    def execute(self, query, parameters=None):
        if parameters is None:
            parameters = ()
        self._cursor.execute(query, parameters)
        self.rowcount = self._cursor.rowcount
        return self._cursor.fetchall()


class _MySqlBackend(_GenericBackend):
    def __init__(self):
        import MySQLdb
        super(_MySqlBackend, self).__init__(MySQLdb)


    @staticmethod
    def convert_boolean(boolean, conversion_dict):
        'Convert booleans to integer strings'
        return str(int(boolean))


    def connect(self, host=None, username=None, password=None, db_name=None):
        import MySQLdb.converters
        convert_dict = MySQLdb.converters.conversions
        convert_dict.setdefault(bool, self.convert_boolean)

        self._connection = self._database_module.connect(
            host=host, user=username, passwd=password, db=db_name,
            conv=convert_dict)
        self._connection.autocommit(True)
        self._cursor = self._connection.cursor()


class _SqliteBackend(_GenericBackend):
    def __init__(self):
        try:
            from pysqlite2 import dbapi2
        except ImportError:
            from sqlite3 import dbapi2
        super(_SqliteBackend, self).__init__(dbapi2)
        self._last_insert_id_re = re.compile(r'\sLAST_INSERT_ID\(\)',
                                             re.IGNORECASE)


    def connect(self, host=None, username=None, password=None, db_name=None):
        self._connection = self._database_module.connect(db_name)
        self._connection.isolation_level = None # enable autocommit
        self._cursor = self._connection.cursor()


    def execute(self, query, parameters=None):
        # pysqlite2 uses paramstyle=qmark
        # TODO: make this more sophisticated if necessary
        query = query.replace('%s', '?')
        # pysqlite2 can't handle parameters=None (it throws a nonsense
        # exception)
        if parameters is None:
            parameters = ()
        # sqlite3 doesn't support MySQL's LAST_INSERT_ID().  Instead it has
        # something similar called LAST_INSERT_ROWID() that will do enough of
        # what we want (for our non-concurrent unittest use case).
        query = self._last_insert_id_re.sub(' LAST_INSERT_ROWID()', query)
        return super(_SqliteBackend, self).execute(query, parameters)


class _DjangoBackend(_GenericBackend):
    def __init__(self):
        from django.db import backend, connection, transaction
        import django.db as django_db
        super(_DjangoBackend, self).__init__(django_db)
        self._django_connection = connection
        self._django_transaction = transaction


    def connect(self, host=None, username=None, password=None, db_name=None):
        self._connection = self._django_connection
        self._cursor = self._connection.cursor()


    def execute(self, query, parameters=None):
        try:
            return super(_DjangoBackend, self).execute(query,
                                                       parameters=parameters)
        finally:
            self._django_transaction.commit_unless_managed()


_BACKEND_MAP = {
    'mysql': _MySqlBackend,
    'sqlite': _SqliteBackend,
    'django': _DjangoBackend,
}


class DatabaseConnection(object):
    """
    Generic wrapper for a database connection.  Supports both mysql and sqlite
    backends.

    Public attributes:
    * reconnect_enabled: if True, when an OperationalError occurs the class will
      try to reconnect to the database automatically.
    * reconnect_delay_sec: seconds to wait before reconnecting
    * max_reconnect_attempts: maximum number of time to try reconnecting before
      giving up.  Setting to RECONNECT_FOREVER removes the limit.
    * rowcount - will hold cursor.rowcount after each call to execute().
    * global_config_section - the section in which to find DB information. this
      should be passed to the constructor, not set later, and may be None, in
      which case information must be passed to connect().
    * debug - if set True, all queries will be printed before being executed
    """
    _DATABASE_ATTRIBUTES = ('db_type', 'host', 'username', 'password',
                            'db_name')

    def __init__(self, global_config_section=None, debug=False):
        self.global_config_section = global_config_section
        self._backend = None
        self.rowcount = None
        self.debug = debug

        # reconnect defaults
        self.reconnect_enabled = True
        self.reconnect_delay_sec = 20
        self.max_reconnect_attempts = 10

        self._read_options()


    def _get_option(self, name, provided_value, use_afe_setting=False):
        """Get value of given option from global config.

        @param name: Name of the config.
        @param provided_value: Value being provided to override the one from
                               global config.
        @param use_afe_setting: Force to use the settings in AFE, default is
                                False.
        """
        # TODO(dshi): This function returns the option value depends on multiple
        # conditions. The value of `provided_value` has highest priority, then
        # the code checks if use_afe_setting is True, if that's the case, force
        # to use settings in AUTOTEST_WEB. At last the value is retrieved from
        # specified global config section.
        # The logic is too complicated for a generic function named like
        # _get_option. Ideally we want to make it clear from caller that it
        # wants to get database credential from one of the 3 ways:
        # 1. Use the credential from given config section
        # 2. Use the credential from AUTOTEST_WEB section
        # 3. Use the credential provided by caller.
        if provided_value is not None:
            return provided_value
        section = ('AUTOTEST_WEB' if use_afe_setting else
                   self.global_config_section)
        if section:
            global_config_name = _GLOBAL_CONFIG_NAMES.get(name, name)
            return global_config.global_config.get_config_value(
                    section, global_config_name)

        return getattr(self, name, None)


    def _read_options(self, db_type=None, host=None, username=None,
                      password=None, db_name=None):
        """Read database information from global config.

        Unless any parameter is specified a value, the connection will use
        database name from given configure section (self.global_config_section),
        and database credential from AFE database settings (AUTOTEST_WEB).

        @param db_type: database type, default to None.
        @param host: database hostname, default to None.
        @param username: user name for database connection, default to None.
        @param password: database password, default to None.
        @param db_name: database name, default to None.
        """
        self.db_name = self._get_option('db_name', db_name)
        use_afe_setting = not bool(db_type or host or username or password)

        # Database credential can be provided by the caller, as passed in from
        # function connect.
        self.db_type = self._get_option('db_type', db_type, use_afe_setting)
        self.host = self._get_option('host', host, use_afe_setting)
        self.username = self._get_option('username', username, use_afe_setting)
        self.password = self._get_option('password', password, use_afe_setting)


    def _get_backend(self, db_type):
        if db_type not in _BACKEND_MAP:
            raise ValueError('Invalid database type: %s, should be one of %s' %
                             (db_type, ', '.join(_BACKEND_MAP.keys())))
        backend_class = _BACKEND_MAP[db_type]
        return backend_class()


    def _reached_max_attempts(self, num_attempts):
        return (self.max_reconnect_attempts is not RECONNECT_FOREVER and
                num_attempts > self.max_reconnect_attempts)


    def _is_reconnect_enabled(self, supplied_param):
        if supplied_param is not None:
            return supplied_param
        return self.reconnect_enabled


    def _connect_backend(self, try_reconnecting=None):
        num_attempts = 0
        while True:
            try:
                self._backend.connect(host=self.host, username=self.username,
                                      password=self.password,
                                      db_name=self.db_name)
                return
            except self._backend.OperationalError:
                num_attempts += 1
                if not self._is_reconnect_enabled(try_reconnecting):
                    raise
                if self._reached_max_attempts(num_attempts):
                    raise
                traceback.print_exc()
                print ("Can't connect to database; reconnecting in %s sec" %
                       self.reconnect_delay_sec)
                time.sleep(self.reconnect_delay_sec)
                self.disconnect()


    def connect(self, db_type=None, host=None, username=None, password=None,
                db_name=None, try_reconnecting=None):
        """
        Parameters passed to this function will override defaults from global
        config.  try_reconnecting, if passed, will override
        self.reconnect_enabled.
        """
        self.disconnect()
        self._read_options(db_type, host, username, password, db_name)

        self._backend = self._get_backend(self.db_type)
        _copy_exceptions(self._backend, self)
        self._connect_backend(try_reconnecting)


    def disconnect(self):
        if self._backend:
            self._backend.disconnect()


    def execute(self, query, parameters=None, try_reconnecting=None):
        """
        Execute a query and return cursor.fetchall(). try_reconnecting, if
        passed, will override self.reconnect_enabled.
        """
        if self.debug:
            print 'Executing %s, %s' % (query, parameters)
        # _connect_backend() contains a retry loop, so don't loop here
        try:
            results = self._backend.execute(query, parameters)
        except self._backend.OperationalError:
            if not self._is_reconnect_enabled(try_reconnecting):
                raise
            traceback.print_exc()
            print ("MYSQL connection died; reconnecting")
            self.disconnect()
            self._connect_backend(try_reconnecting)
            results = self._backend.execute(query, parameters)

        self.rowcount = self._backend.rowcount
        return results


    def get_database_info(self):
        return dict((attribute, getattr(self, attribute))
                    for attribute in self._DATABASE_ATTRIBUTES)


    @classmethod
    def get_test_database(cls, file_path=':memory:', **constructor_kwargs):
        """
        Factory method returning a DatabaseConnection for a temporary in-memory
        database.
        """
        database = cls(**constructor_kwargs)
        database.reconnect_enabled = False
        database.connect(db_type='sqlite', db_name=file_path)
        return database


class TranslatingDatabase(DatabaseConnection):
    """
    Database wrapper than applies arbitrary substitution regexps to each query
    string.  Useful for SQLite testing.
    """
    def __init__(self, translators):
        """
        @param translation_regexps: list of callables to apply to each query
                string (in order).  Each accepts a query string and returns a
                (possibly) modified query string.
        """
        super(TranslatingDatabase, self).__init__()
        self._translators = translators


    def execute(self, query, parameters=None, try_reconnecting=None):
        for translator in self._translators:
            query = translator(query)
        return super(TranslatingDatabase, self).execute(
                query, parameters=parameters, try_reconnecting=try_reconnecting)


    @classmethod
    def make_regexp_translator(cls, search_re, replace_str):
        """
        Returns a translator that calls re.sub() on the query with the given
        search and replace arguments.
        """
        def translator(query):
            return re.sub(search_re, replace_str, query)
        return translator
