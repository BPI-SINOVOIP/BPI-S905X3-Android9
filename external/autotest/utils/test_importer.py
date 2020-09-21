#!/usr/bin/python
#
# Copyright 2008 Google Inc. All Rights Reserved.
"""
This utility allows for easy updating, removing and importing
of tests into the autotest_web afe_autotests table.

Example of updating client side tests:
./test_importer.py -t /usr/local/autotest/client/tests

If, for example, not all of your control files adhere to the standard outlined
at http://autotest.kernel.org/wiki/ControlRequirements, you can force options:

./test_importer.py --test-type server -t /usr/local/autotest/server/tests

You would need to pass --add-noncompliant to include such control files,
however.  An easy way to check for compliance is to run in dry mode:

./test_importer.py --dry-run -t /usr/local/autotest/server/tests/mytest

Running with no options is equivalent to --add-all --db-clear-tests.

Most options should be fairly self explanatory, use --help to display them.
"""


import common
import logging, re, os, sys, optparse, compiler

from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import logging_config, logging_manager


class TestImporterLoggingConfig(logging_config.LoggingConfig):
    #pylint: disable-msg=C0111
    def configure_logging(self, results_dir=None, verbose=False):
        super(TestImporterLoggingConfig, self).configure_logging(
                                                               use_console=True,
                                                               verbose=verbose)


# Global
DRY_RUN = False
DEPENDENCIES_NOT_FOUND = set()


def update_all(autotest_dir, add_noncompliant, add_experimental):
    """
    Function to scan through all tests and add them to the database.

    This function invoked when no parameters supplied to the command line.
    It 'synchronizes' the test database with the current contents of the
    client and server test directories.  When test code is discovered
    in the file system new tests may be added to the db.  Likewise,
    if test code is not found in the filesystem, tests may be removed
    from the db.  The base test directories are hard-coded to client/tests,
    client/site_tests, server/tests and server/site_tests.

    @param autotest_dir: prepended to path strings (/usr/local/autotest).
    @param add_noncompliant: attempt adding test with invalid control files.
    @param add_experimental: add tests with experimental attribute set.
    """
    for path in [ 'server/tests', 'server/site_tests', 'client/tests',
                  'client/site_tests']:
        test_path = os.path.join(autotest_dir, path)
        if not os.path.exists(test_path):
            continue
        logging.info("Scanning %s", test_path)
        tests = []
        tests = get_tests_from_fs(test_path, "^control.*",
                                 add_noncompliant=add_noncompliant)
        update_tests_in_db(tests, add_experimental=add_experimental,
                           add_noncompliant=add_noncompliant,
                           autotest_dir=autotest_dir)
    test_suite_path = os.path.join(autotest_dir, 'test_suites')
    if os.path.exists(test_suite_path):
        logging.info("Scanning %s", test_suite_path)
        tests = get_tests_from_fs(test_suite_path, '.*',
                                 add_noncompliant=add_noncompliant)
        update_tests_in_db(tests, add_experimental=add_experimental,
                           add_noncompliant=add_noncompliant,
                           autotest_dir=autotest_dir)

    profilers_path = os.path.join(autotest_dir, "client/profilers")
    if os.path.exists(profilers_path):
        logging.info("Scanning %s", profilers_path)
        profilers = get_tests_from_fs(profilers_path, '.*py$')
        update_profilers_in_db(profilers, add_noncompliant=add_noncompliant,
                               description='NA')
    # Clean bad db entries
    db_clean_broken(autotest_dir)


def update_samples(autotest_dir, add_noncompliant, add_experimental):
    """
    Add only sample tests to the database from the filesystem.

    This function invoked when -S supplied on command line.
    Only adds tests to the database - does not delete any.
    Samples tests are formatted slightly differently than other tests.

    @param autotest_dir: prepended to path strings (/usr/local/autotest).
    @param add_noncompliant: attempt adding test with invalid control files.
    @param add_experimental: add tests with experimental attribute set.
    """
    sample_path = os.path.join(autotest_dir, 'server/samples')
    if os.path.exists(sample_path):
        logging.info("Scanning %s", sample_path)
        tests = get_tests_from_fs(sample_path, '.*srv$',
                                  add_noncompliant=add_noncompliant)
        update_tests_in_db(tests, add_experimental=add_experimental,
                           add_noncompliant=add_noncompliant,
                           autotest_dir=autotest_dir)


def db_clean_broken(autotest_dir):
    """
    Remove tests from autotest_web that do not have valid control files

    This function invoked when -c supplied on the command line and when
    running update_all().  Removes tests from database which are not
    found in the filesystem.  Also removes profilers which are just
    a special case of tests.

    @param autotest_dir: prepended to path strings (/usr/local/autotest).
    """
    for test in models.Test.objects.all():
        full_path = os.path.join(autotest_dir, test.path)
        if not os.path.isfile(full_path):
            logging.info("Removing %s", test.path)
            _log_or_execute(repr(test), test.delete)

    # Find profilers that are no longer present
    for profiler in models.Profiler.objects.all():
        full_path = os.path.join(autotest_dir, "client", "profilers",
                                 profiler.name)
        if not os.path.exists(full_path):
            logging.info("Removing %s", profiler.name)
            _log_or_execute(repr(profiler), profiler.delete)


def db_clean_all(autotest_dir):
    """
    Remove all tests from autotest_web - very destructive

    This function invoked when -C supplied on the command line.
    Removes ALL tests from the database.

    @param autotest_dir: prepended to path strings (/usr/local/autotest).
    """
    for test in models.Test.objects.all():
        full_path = os.path.join(autotest_dir, test.path)
        logging.info("Removing %s", test.path)
        _log_or_execute(repr(test), test.delete)

    # Find profilers that are no longer present
    for profiler in models.Profiler.objects.all():
        full_path = os.path.join(autotest_dir, "client", "profilers",
                                 profiler.name)
        logging.info("Removing %s", profiler.name)
        _log_or_execute(repr(profiler), profiler.delete)


def update_profilers_in_db(profilers, description='NA',
                           add_noncompliant=False):
    """
    Add only profilers to the database from the filesystem.

    This function invoked when -p supplied on command line.
    Only adds profilers to the database - does not delete any.
    Profilers are formatted slightly differently than tests.

    @param profilers: list of profilers found in the file system.
    @param description: simple text to satisfy docstring.
    @param add_noncompliant: attempt adding test with invalid control files.
    """
    for profiler in profilers:
        name = os.path.basename(profiler)
        if name.endswith('.py'):
            name = name[:-3]
        if not profilers[profiler]:
            if add_noncompliant:
                doc = description
            else:
                logging.warning("Skipping %s, missing docstring", profiler)
                continue
        else:
            doc = profilers[profiler]

        model = models.Profiler.objects.get_or_create(name=name)[0]
        model.description = doc
        _log_or_execute(repr(model), model.save)


def _set_attributes_custom(test, data):
    # We set the test name to the dirname of the control file.
    test_new_name = test.path.split('/')
    if test_new_name[-1] == 'control' or test_new_name[-1] == 'control.srv':
        test.name = test_new_name[-2]
    else:
        control_name = "%s:%s"
        control_name %= (test_new_name[-2],
                         test_new_name[-1])
        test.name = re.sub('control.*\.', '', control_name)

    # We set verify to always False (0).
    test.run_verify = 0

    if hasattr(data, 'test_parameters'):
        for para_name in data.test_parameters:
            test_parameter = models.TestParameter.objects.get_or_create(
                test=test, name=para_name)[0]
            test_parameter.save()


def update_tests_in_db(tests, dry_run=False, add_experimental=False,
                       add_noncompliant=False, autotest_dir=None):
    """
    Scans through all tests and add them to the database.

    This function invoked when -t supplied and for update_all.
    When test code is discovered in the file system new tests may be added

    @param tests: list of tests found in the filesystem.
    @param dry_run: not used at this time.
    @param add_experimental: add tests with experimental attribute set.
    @param add_noncompliant: attempt adding test with invalid control files.
    @param autotest_dir: prepended to path strings (/usr/local/autotest).
    """
    for test in tests:
        new_test = models.Test.objects.get_or_create(
                path=test.replace(autotest_dir, '').lstrip('/'))[0]
        logging.info("Processing %s", new_test.path)

        # Set the test's attributes
        data = tests[test]
        _set_attributes_clean(new_test, data)

        # Custom Attribute Update
        _set_attributes_custom(new_test, data)

        # This only takes place if --add-noncompliant is provided on the CLI
        if not new_test.name:
            test_new_test = test.split('/')
            if test_new_test[-1] == 'control':
                new_test.name = test_new_test[-2]
            else:
                control_name = "%s:%s"
                control_name %= (test_new_test[-2],
                                 test_new_test[-1])
                new_test.name = control_name.replace('control.', '')

        # Experimental Check
        if not add_experimental and new_test.experimental:
            continue

        _log_or_execute(repr(new_test), new_test.save)
        add_label_dependencies(new_test)

        # save TestParameter
        for para_name in data.test_parameters:
            test_parameter = models.TestParameter.objects.get_or_create(
                test=new_test, name=para_name)[0]
            test_parameter.save()


def _set_attributes_clean(test, data):
    """
    First pass sets the attributes of the Test object from file system.

    @param test: a test object to be populated for the database.
    @param data: object with test data from the file system.
    """
    test_time = { 'short' : 1,
                  'medium' : 2,
                  'long' : 3, }


    string_attributes = ('name', 'author', 'test_class', 'test_category',
                         'test_category', 'sync_count')
    for attribute in string_attributes:
        setattr(test, attribute, getattr(data, attribute))

    test.description = data.doc
    test.dependencies = ", ".join(data.dependencies)

    try:
        test.test_type = control_data.CONTROL_TYPE.get_value(data.test_type)
    except AttributeError:
        raise Exception('Unknown test_type %s for test %s', data.test_type,
                        data.name)

    int_attributes = ('experimental', 'run_verify')
    for attribute in int_attributes:
        setattr(test, attribute, int(getattr(data, attribute)))

    try:
        test.test_time = int(data.time)
        if test.test_time < 1 or test.time > 3:
            raise Exception('Incorrect number %d for time' % test.time)
    except ValueError:
        pass

    if not test.test_time and str == type(data.time):
        test.test_time = test_time[data.time.lower()]

    test.test_retry = data.retries


def add_label_dependencies(test):
    """
    Add proper many-to-many relationships from DEPENDENCIES field.

    @param test: test object for the database.
    """

    # clear out old relationships
    _log_or_execute(repr(test), test.dependency_labels.clear,
                    subject='clear dependencies from')

    for label_name in test.dependencies.split(','):
        label_name = label_name.strip().lower()
        if not label_name:
            continue

        try:
            label = models.Label.objects.get(name=label_name)
        except models.Label.DoesNotExist:
            log_dependency_not_found(label_name)
            continue

        _log_or_execute(repr(label), test.dependency_labels.add, label,
                        subject='add dependency to %s' % test.name)


def log_dependency_not_found(label_name):
    """
    Exception processing when label not found in database.

    @param label_name: from test dependencies.
    """
    if label_name in DEPENDENCIES_NOT_FOUND:
        return
    logging.info("Dependency %s not found", label_name)
    DEPENDENCIES_NOT_FOUND.add(label_name)


def get_tests_from_fs(parent_dir, control_pattern, add_noncompliant=False):
    """
    Find control files in file system and load a list with their info.

    @param parent_dir: directory to search recursively.
    @param control_pattern: name format of control file.
    @param add_noncompliant: ignore control file parse errors.

    @return dictionary of the form: tests[file_path] = parsed_object
    """
    tests = {}
    profilers = False
    if 'client/profilers' in parent_dir:
        profilers = True
    for dir in [ parent_dir ]:
        files = recursive_walk(dir, control_pattern)
        for file in files:
            if '__init__.py' in file or '.svn' in file:
                continue
            if not profilers:
                if not add_noncompliant:
                    try:
                        found_test = control_data.parse_control(file,
                                                            raise_warnings=True)
                        tests[file] = found_test
                    except control_data.ControlVariableException, e:
                        logging.warning("Skipping %s\n%s", file, e)
                    except Exception, e:
                        logging.error("Bad %s\n%s", file, e)
                else:
                    found_test = control_data.parse_control(file)
                    tests[file] = found_test
            else:
                tests[file] = compiler.parseFile(file).doc
    return tests


def recursive_walk(path, wildcard):
    """
    Recursively go through a directory.

    This function invoked by get_tests_from_fs().

    @param path: base directory to start search.
    @param wildcard: name format to match.

    @return A list of files that match wildcard
    """
    files = []
    directories = [ path ]
    while len(directories)>0:
        directory = directories.pop()
        for name in os.listdir(directory):
            fullpath = os.path.join(directory, name)
            if os.path.isfile(fullpath):
                # if we are a control file
                if re.search(wildcard, name):
                    files.append(fullpath)
            elif os.path.isdir(fullpath):
                directories.append(fullpath)
    return files


def _log_or_execute(content, func, *args, **kwargs):
    """
    Log a message if dry_run is enabled, or execute the given function.

    Relies on the DRY_RUN global variable.

    @param content: the actual log message.
    @param func: function to execute if dry_run is not enabled.
    @param subject: (Optional) The type of log being written. Defaults to
                     the name of the provided function.
    """
    subject = kwargs.get('subject', func.__name__)

    if DRY_RUN:
        logging.info("Would %s: %s",  subject, content)
    else:
        func(*args)


def _create_whitelist_set(whitelist_path):
    """
    Create a set with contents from a whitelist file for membership testing.

    @param whitelist_path: full path to the whitelist file.

    @return set with files listed one/line - newlines included.
    """
    f = open(whitelist_path, 'r')
    whitelist_set = set([line.strip() for line in f])
    f.close()
    return whitelist_set


def update_from_whitelist(whitelist_set, add_experimental, add_noncompliant,
                          autotest_dir):
    """
    Scans through all tests in the whitelist and add them to the database.

    This function invoked when -w supplied.

    @param whitelist_set: set of tests in full-path form from a whitelist.
    @param add_experimental: add tests with experimental attribute set.
    @param add_noncompliant: attempt adding test with invalid control files.
    @param autotest_dir: prepended to path strings (/usr/local/autotest).
    """
    tests = {}
    profilers = {}
    for file_path in whitelist_set:
        if file_path.find('client/profilers') == -1:
            try:
                found_test = control_data.parse_control(file_path,
                                                        raise_warnings=True)
                tests[file_path] = found_test
            except control_data.ControlVariableException, e:
                logging.warning("Skipping %s\n%s", file, e)
        else:
            profilers[file_path] = compiler.parseFile(file_path).doc

    if len(tests) > 0:
        update_tests_in_db(tests, add_experimental=add_experimental,
                           add_noncompliant=add_noncompliant,
                           autotest_dir=autotest_dir)
    if len(profilers) > 0:
        update_profilers_in_db(profilers, add_noncompliant=add_noncompliant,
                               description='NA')


def main(argv):
    """Main function
    @param argv: List of command line parameters.
    """

    global DRY_RUN
    parser = optparse.OptionParser()
    parser.add_option('-c', '--db-clean-tests',
                      dest='clean_tests', action='store_true',
                      default=False,
                help='Clean client and server tests with invalid control files')
    parser.add_option('-C', '--db-clear-all-tests',
                      dest='clear_all_tests', action='store_true',
                      default=False,
                help='Clear ALL client and server tests')
    parser.add_option('-d', '--dry-run',
                      dest='dry_run', action='store_true', default=False,
                      help='Dry run for operation')
    parser.add_option('-A', '--add-all',
                      dest='add_all', action='store_true',
                      default=False,
                      help='Add site_tests, tests, and test_suites')
    parser.add_option('-S', '--add-samples',
                      dest='add_samples', action='store_true',
                      default=False,
                      help='Add samples.')
    parser.add_option('-E', '--add-experimental',
                      dest='add_experimental', action='store_true',
                      default=True,
                      help='Add experimental tests to frontend, works only '
                           'with -A (--add-all) option')
    parser.add_option('-N', '--add-noncompliant',
                      dest='add_noncompliant', action='store_true',
                      default=False,
                      help='Add non-compliant tests (i.e. tests that do not '
                           'define all required control variables), works '
                           'only with -A (--add-all) option')
    parser.add_option('-p', '--profile-dir', dest='profile_dir',
                      help='Directory to recursively check for profiles')
    parser.add_option('-t', '--tests-dir', dest='tests_dir',
                      help='Directory to recursively check for control.*')
    parser.add_option('-r', '--control-pattern', dest='control_pattern',
                      default='^control.*',
               help='The pattern to look for in directories for control files')
    parser.add_option('-v', '--verbose',
                      dest='verbose', action='store_true', default=False,
                      help='Run in verbose mode')
    parser.add_option('-w', '--whitelist-file', dest='whitelist_file',
                      help='Filename for list of test names that must match')
    parser.add_option('-z', '--autotest-dir', dest='autotest_dir',
                      default=os.path.join(os.path.dirname(__file__), '..'),
                      help='Autotest directory root')
    options, args = parser.parse_args()

    logging_manager.configure_logging(TestImporterLoggingConfig(),
                                      verbose=options.verbose)

    DRY_RUN = options.dry_run
    if DRY_RUN:
        logging.getLogger().setLevel(logging.WARN)

    if len(argv) > 1 and options.add_noncompliant and not options.add_all:
        logging.error('-N (--add-noncompliant) must be ran with option -A '
                      '(--add-All).')
        return 1

    if len(argv) > 1 and options.add_experimental and not options.add_all:
        logging.error('-E (--add-experimental) must be ran with option -A '
                      '(--add-All).')
        return 1

    # Make sure autotest_dir is the absolute path
    options.autotest_dir = os.path.abspath(options.autotest_dir)

    if len(args) > 0:
        logging.error("Invalid option(s) provided: %s", args)
        parser.print_help()
        return 1

    if options.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    if len(argv) == 1 or (len(argv) == 2 and options.verbose):
        update_all(options.autotest_dir, options.add_noncompliant,
                   options.add_experimental)
        db_clean_broken(options.autotest_dir)
        return 0

    if options.clear_all_tests:
        if (options.clean_tests or options.add_all or options.add_samples or
            options.add_noncompliant):
            logging.error(
                "Can only pass --autotest-dir, --dry-run and --verbose with "
                "--db-clear-all-tests")
            return 1
        db_clean_all(options.autotest_dir)

    whitelist_set = None
    if options.whitelist_file:
        if options.add_all:
            logging.error("Cannot pass both --add-all and --whitelist-file")
            return 1
        whitelist_path = os.path.abspath(options.whitelist_file)
        if not os.path.isfile(whitelist_path):
            logging.error("--whitelist-file (%s) not found", whitelist_path)
            return 1
        logging.info("Using whitelist file %s", whitelist_path)
        whitelist_set =  _create_whitelist_set(whitelist_path)
        update_from_whitelist(whitelist_set,
                              add_experimental=options.add_experimental,
                              add_noncompliant=options.add_noncompliant,
                              autotest_dir=options.autotest_dir)
    if options.add_all:
        update_all(options.autotest_dir, options.add_noncompliant,
                   options.add_experimental)
    if options.add_samples:
        update_samples(options.autotest_dir, options.add_noncompliant,
                       options.add_experimental)
    if options.tests_dir:
        options.tests_dir = os.path.abspath(options.tests_dir)
        tests = get_tests_from_fs(options.tests_dir, options.control_pattern,
                                  add_noncompliant=options.add_noncompliant)
        update_tests_in_db(tests, add_experimental=options.add_experimental,
                           add_noncompliant=options.add_noncompliant,
                           autotest_dir=options.autotest_dir)
    if options.profile_dir:
        profilers = get_tests_from_fs(options.profile_dir, '.*py$')
        update_profilers_in_db(profilers,
                               add_noncompliant=options.add_noncompliant,
                               description='NA')
    if options.clean_tests:
        db_clean_broken(options.autotest_dir)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
