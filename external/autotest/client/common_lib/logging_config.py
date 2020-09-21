import logging
import os
import sys
import time
from autotest_lib.client.common_lib import utils

# Set up a simple catchall configuration for use during import time.  Some code
# may log messages at import time and we don't want those to get completely
# thrown away.  We'll clear this out when actual configuration takes place.
logging.basicConfig(level=logging.DEBUG)


class AllowBelowSeverity(logging.Filter):
    """
    Allows only records less severe than a given level (the opposite of what
    the normal logging level filtering does.
    """

    def __init__(self, level):
        self.level = level

    def filter(self, record):
        return record.levelno < self.level


class VarLogMessageFormatter(logging.Formatter):
    """
    Respews logging.* strings on the DUT to /var/log/messages for easier
    correlation of user mode test activity with kernel messages.
    """
    _should_respew = True

    def format(self, record):
        s = super(VarLogMessageFormatter, self).format(record)
        # On Brillo the logger binary is not available. Disable after error.
        if self._should_respew:
            try:
                os.system('logger -t "autotest" "%s"' % utils.sh_escape(s))
            except OSError:
                self._should_respew = False
        return s


class LoggingConfig(object):
    global_level = logging.DEBUG
    stdout_level = logging.INFO
    stderr_level = logging.ERROR

    FILE_FORMAT = ('%(asctime)s.%(msecs)03d %(levelname)-5.5s|%(module)18.18s:'
                   '%(lineno)4.4d| %(message)s')
    FILE_FORMAT_WITH_THREADNAME = (
            '%(asctime)s.%(msecs)03d %(levelname)-5.5s|%(module)18.18s:'
            '%(lineno)4.4d| %(threadName)16.16s| %(message)s')
    DATE_FORMAT = '%m/%d %H:%M:%S'

    file_formatter = logging.Formatter(fmt=FILE_FORMAT, datefmt=DATE_FORMAT)

    CONSOLE_FORMAT = '%(asctime)s %(levelname)-5.5s| %(message)s'

    console_formatter = logging.Formatter(fmt=CONSOLE_FORMAT,
                                          datefmt='%H:%M:%S')

    system_formatter = VarLogMessageFormatter(fmt=FILE_FORMAT,
                                              datefmt='%H:%M:%S')

    def __init__(self, use_console=True):
        self.logger = logging.getLogger()
        self.global_level = logging.DEBUG
        self.use_console = use_console

    @classmethod
    def get_timestamped_log_name(cls, base_name):
        return '%s.log.%s' % (base_name, time.strftime('%Y-%m-%d-%H.%M.%S'))

    @classmethod
    def get_autotest_root(cls):
        common_lib_dir = os.path.dirname(__file__)
        return os.path.abspath(os.path.join(common_lib_dir, '..', '..'))

    @classmethod
    def get_server_log_dir(cls):
        return os.path.join(cls.get_autotest_root(), 'logs')

    def add_stream_handler(self, stream, level=logging.DEBUG, datefmt=None):
        handler = logging.StreamHandler(stream)
        handler.setLevel(level)
        formatter = self.console_formatter
        if datefmt:
            formatter = logging.Formatter(fmt=self.CONSOLE_FORMAT,
                                          datefmt=datefmt)
        handler.setFormatter(formatter)
        self.logger.addHandler(handler)
        return handler

    def add_console_handlers(self, datefmt=None):
        stdout_handler = self.add_stream_handler(sys.stdout,
                                                 level=self.stdout_level,
                                                 datefmt=datefmt)
        # only pass records *below* STDERR_LEVEL to stdout, to avoid duplication
        stdout_handler.addFilter(AllowBelowSeverity(self.stderr_level))

        self.add_stream_handler(sys.stderr, self.stderr_level, datefmt)

    def add_file_handler(self,
                         file_path,
                         level=logging.DEBUG,
                         log_dir=None,
                         datefmt=None):
        if log_dir:
            file_path = os.path.join(log_dir, file_path)
        handler = logging.FileHandler(file_path)
        handler.setLevel(level)
        formatter = self.file_formatter
        if datefmt:
            formatter = logging.Formatter(fmt=self.FILE_FORMAT, datefmt=datefmt)
        # Respew the content of the selected file to /var/log/messages.
        if file_path == '/usr/local/autotest/results/default/debug/client.0.WARNING':
            formatter = self.system_formatter
        handler.setFormatter(formatter)
        self.logger.addHandler(handler)
        return handler

    def _add_file_handlers_for_all_levels(self, log_dir, log_name):
        for level in (logging.DEBUG, logging.INFO, logging.WARNING,
                      logging.ERROR):
            file_name = '%s.%s' % (log_name, logging.getLevelName(level))
            self.add_file_handler(file_name, level=level, log_dir=log_dir)

    def add_debug_file_handlers(self, log_dir, log_name=None):
        raise NotImplementedError

    def _clear_all_handlers(self):
        for handler in list(self.logger.handlers):
            self.logger.removeHandler(handler)
            # Attempt to close the handler. If it's already closed a KeyError
            # will be generated. http://bugs.python.org/issue8581
            try:
                handler.close()
            except KeyError:
                pass

    def configure_logging(self, use_console=True, verbose=False, datefmt=None):
        self._clear_all_handlers()  # see comment at top of file
        self.logger.setLevel(self.global_level)

        if verbose:
            self.stdout_level = logging.DEBUG
        if use_console:
            self.add_console_handlers(datefmt)


class TestingConfig(LoggingConfig):

    def add_stream_handler(self, *args, **kwargs):
        pass

    def add_file_handler(self, *args, **kwargs):
        pass

    def configure_logging(self, **kwargs):
        pass


def add_threadname_in_log():
    """Change logging formatter to include thread name.

    This is to help logs from each thread runs to include its thread name.
    """
    log = logging.getLogger()
    for handler in logging.getLogger().handlers:
        if isinstance(handler, logging.FileHandler):
            log.removeHandler(handler)
            handler.setFormatter(logging.Formatter(
                    LoggingConfig.FILE_FORMAT_WITH_THREADNAME,
                    LoggingConfig.DATE_FORMAT))
            log.addHandler(handler)
    logging.debug('Logging handler\'s format updated with thread name.')
