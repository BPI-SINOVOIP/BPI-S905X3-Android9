#!/usr/bin/python -u

"""
Utility to upload or remove the packages from the packages repository.
"""

import logging, optparse, os, shutil, sys, tempfile
import common
from autotest_lib.client.common_lib import utils as client_utils
from autotest_lib.client.common_lib import global_config, error
from autotest_lib.client.common_lib import packages
from autotest_lib.server import utils as server_utils

c = global_config.global_config
logging.basicConfig(level=logging.DEBUG)

ACTION_REMOVE = 'remove'
ACTION_UPLOAD = 'upload'
ACTION_TAR_ONLY = 'tar_only'

def get_exclude_string(client_dir):
    '''
    Get the exclude string for the tar command to exclude specific
    subdirectories inside client_dir.
    For profilers we need to exclude everything except the __init__.py
    file so that the profilers can be imported.
    '''
    exclude_string = ('--exclude="deps/*" --exclude="tests/*" '
                      '--exclude="site_tests/*" --exclude="**.pyc"')

    # Get the profilers directory
    prof_dir = os.path.join(client_dir, 'profilers')

    # Include the __init__.py file for the profilers and exclude all its
    # subdirectories
    for f in os.listdir(prof_dir):
        if os.path.isdir(os.path.join(prof_dir, f)):
            exclude_string += ' --exclude="profilers/%s"' % f

    # The '.' here is needed to zip the files in the current
    # directory. We use '-C' for tar to change to the required
    # directory i.e. src_dir and then zip up the files in that
    # directory(which is '.') excluding the ones in the exclude_dirs
    exclude_string += " ."

    # TODO(milleral): This is sad and ugly.  http://crbug.com/258161
    # Surprisingly, |exclude_string| actually means argument list, and
    # we'd like to package up the current global_config.ini also, so let's
    # just tack it on here.
    # Also note that this only works because tar prevents us from un-tarring
    # files into parent directories.
    exclude_string += " ../global_config.ini"

    return exclude_string


def parse_args():
    parser = optparse.OptionParser()
    parser.add_option("-d", "--dependency", help="package the dependency"
                      " from client/deps directory and upload to the repo",
                      dest="dep")
    parser.add_option("-p", "--profiler", help="package the profiler "
                      "from client/profilers directory and upload to the repo",
                      dest="prof")
    parser.add_option("-t", "--test", help="package the test from client/tests"
                      " or client/site_tests and upload to the repo.",
                      dest="test")
    parser.add_option("-c", "--client", help="package the client "
                      "directory alone without the tests, deps and profilers",
                      dest="client", action="store_true", default=False)
    parser.add_option("-f", "--file", help="simply uploads the specified"
                      "file on to the repo", dest="file")
    parser.add_option("-r", "--repository", help="the URL of the packages"
                      "repository location to upload the packages to.",
                      dest="repo", default=None)
    parser.add_option("-o", "--output_dir", help="the output directory"
                      "to place tarballs and md5sum files in.",
                      dest="output_dir", default=None)
    parser.add_option("-a", "--action", help="the action to perform",
                      dest="action", choices=(ACTION_UPLOAD, ACTION_REMOVE,
                                              ACTION_TAR_ONLY), default=None)
    parser.add_option("--all", help="Upload all the files locally "
                      "to all the repos specified in global_config.ini. "
                      "(includes the client, tests, deps and profilers)",
                      dest="all", action="store_true", default=False)

    options, args = parser.parse_args()
    return options, args

def get_build_dir(name, dest_dir, pkg_type):
    """Method to generate the build directory where the tarball and checksum
    is stored. The following package types are handled: test, dep, profiler.
    Package type 'client' is not handled.
    """
    if pkg_type == 'client':
        # NOTE: The "tar_only" action for pkg_type "client" has no use
        # case yet. No known invocations of packager.py with
        # --action=tar_only send in clients in the command line. Please
        # confirm the behaviour is expected before this type is enabled for
        # "tar_only" actions.
        print ('Tar action not supported for pkg_type= %s, name = %s' %
                pkg_type, name)
        return None
    # For all packages, the work-dir should have 'client' appended to it.
    base_build_dir = os.path.join(dest_dir, 'client')
    if pkg_type == 'test':
        build_dir = os.path.join(get_test_dir(name, base_build_dir), name)
    else:
        # For profiler and dep, we append 's', and then append the name.
        # TODO(pmalani): Make this less fiddly?
        build_dir = os.path.join(base_build_dir, pkg_type + 's', name)
    return build_dir

def process_packages(pkgmgr, pkg_type, pkg_names, src_dir,
                     action, dest_dir=None):
    """Method to upload or remove package depending on the flag passed to it.

    If tar_only is set to True, this routine is solely used to generate a
    tarball and compute the md5sum from that tarball.
    If the tar_only flag is True, then the remove flag is ignored.
    """
    exclude_string = ' .'
    names = [p.strip() for p in pkg_names.split(',')]
    for name in names:
        print "process_packages: Processing %s ... " % name
        if pkg_type == 'client':
            pkg_dir = src_dir
            exclude_string = get_exclude_string(pkg_dir)
        elif pkg_type == 'test':
            # if the package is a test then look whether it is in client/tests
            # or client/site_tests
            pkg_dir = os.path.join(get_test_dir(name, src_dir), name)
        else:
            # for the profilers and deps
            pkg_dir = os.path.join(src_dir, name)

        pkg_name = pkgmgr.get_tarball_name(name, pkg_type)

        exclude_string_tar = ((
            ' --exclude="**%s" --exclude="**%s.checksum" ' %
            (pkg_name, pkg_name)) + exclude_string)
        if action == ACTION_TAR_ONLY:
            # We don't want any pre-existing tarballs and checksums to
            # be repackaged, so we should purge these.
            build_dir = get_build_dir(name, dest_dir, pkg_type)
            try:
                packages.check_diskspace(build_dir)
            except error.RepoDiskFullError as e:
                msg = ("Work_dir directory for packages %s does not have "
                       "enough space available: %s" % (build_dir, e))
                raise error.RepoDiskFullError(msg)
            tarball_path = pkgmgr.tar_package(pkg_name, pkg_dir,
                                              build_dir, exclude_string_tar)

            # Create the md5 hash too.
            md5sum = pkgmgr.compute_checksum(tarball_path)
            md5sum_filepath = os.path.join(build_dir, pkg_name + '.checksum')
            with open(md5sum_filepath, "w") as f:
                f.write(md5sum)

        elif action == ACTION_UPLOAD:
            # Tar the source and upload
            temp_dir = tempfile.mkdtemp()
            try:
                try:
                    packages.check_diskspace(temp_dir)
                except error.RepoDiskFullError, e:
                    msg = ("Temporary directory for packages %s does not have "
                           "enough space available: %s" % (temp_dir, e))
                    raise error.RepoDiskFullError(msg)

                # Check if tarball already exists. If it does, then don't
                # create a tarball again.
                tarball_path = os.path.join(pkg_dir, pkg_name);
                if os.path.exists(tarball_path):
                    print("process_packages: Tarball %s already exists" %
                          tarball_path)
                else:
                    tarball_path = pkgmgr.tar_package(pkg_name, pkg_dir,
                                                      temp_dir,
                                                      exclude_string_tar)
                # Compare the checksum with what packages.checksum has. If they
                # match then we don't need to perform the upload.
                if not pkgmgr.compare_checksum(tarball_path):
                    pkgmgr.upload_pkg(tarball_path, update_checksum=True)
                else:
                    logging.warning('Checksum not changed for %s, not copied '
                                    'in packages/ directory.', tarball_path)
            finally:
                # remove the temporary directory
                shutil.rmtree(temp_dir)
        elif action == ACTION_REMOVE:
            pkgmgr.remove_pkg(pkg_name, remove_checksum=True)
        print "Done."


def tar_packages(pkgmgr, pkg_type, pkg_names, src_dir, temp_dir):
    """Tar all packages up and return a list of each tar created"""
    tarballs = []
    exclude_string = ' .'
    names = [p.strip() for p in pkg_names.split(',')]
    for name in names:
        print "tar_packages: Processing %s ... " % name
        if pkg_type == 'client':
            pkg_dir = src_dir
            exclude_string = get_exclude_string(pkg_dir)
        elif pkg_type == 'test':
            # if the package is a test then look whether it is in client/tests
            # or client/site_tests
            pkg_dir = os.path.join(get_test_dir(name, src_dir), name)
        else:
            # for the profilers and deps
            pkg_dir = os.path.join(src_dir, name)

        pkg_name = pkgmgr.get_tarball_name(name, pkg_type)

        # We don't want any pre-existing tarballs and checksums to
        # be repackaged, so we should purge these.
        exclude_string_tar = ((
            ' --exclude="**%s" --exclude="**%s.checksum" ' %
            (pkg_name, pkg_name)) + exclude_string)
        # Check if tarball already exists. If it does, don't duplicate
        # the effort.
        tarball_path = os.path.join(pkg_dir, pkg_name);
        if os.path.exists(tarball_path):
          print("tar_packages: Tarball %s already exists" % tarball_path);
        else:
            tarball_path = pkgmgr.tar_package(pkg_name, pkg_dir,
                                              temp_dir, exclude_string_tar)
        tarballs.append(tarball_path)
    return tarballs


def process_all_packages(pkgmgr, client_dir, action):
    """Process a full upload of packages as a directory upload."""
    dep_dir = os.path.join(client_dir, "deps")
    prof_dir = os.path.join(client_dir, "profilers")
    # Directory where all are kept
    temp_dir = tempfile.mkdtemp()
    try:
        packages.check_diskspace(temp_dir)
    except error.RepoDiskFullError, e:
        print ("Temp destination for packages is full %s, aborting upload: %s"
               % (temp_dir, e))
        os.rmdir(temp_dir)
        sys.exit(1)

    # process tests
    tests_list = get_subdir_list('tests', client_dir)
    tests = ','.join(tests_list)

    # process site_tests
    site_tests_list = get_subdir_list('site_tests', client_dir)
    site_tests = ','.join(site_tests_list)

    # process deps
    deps_list = get_subdir_list('deps', client_dir)
    deps = ','.join(deps_list)

    # process profilers
    profilers_list = get_subdir_list('profilers', client_dir)
    profilers = ','.join(profilers_list)

    # Update md5sum
    if action == ACTION_UPLOAD:
        all_packages = []
        all_packages.extend(tar_packages(pkgmgr, 'profiler', profilers,
                                         prof_dir, temp_dir))
        all_packages.extend(tar_packages(pkgmgr, 'dep', deps, dep_dir,
                                         temp_dir))
        all_packages.extend(tar_packages(pkgmgr, 'test', site_tests,
                                         client_dir, temp_dir))
        all_packages.extend(tar_packages(pkgmgr, 'test', tests, client_dir,
                                         temp_dir))
        all_packages.extend(tar_packages(pkgmgr, 'client', 'autotest',
                                         client_dir, temp_dir))
        for package in all_packages:
            pkgmgr.upload_pkg(package, update_checksum=True)
        client_utils.run('rm -rf ' + temp_dir)
    elif action == ACTION_REMOVE:
        process_packages(pkgmgr, 'test', tests, client_dir, action=action)
        process_packages(pkgmgr, 'test', site_tests, client_dir, action=action)
        process_packages(pkgmgr, 'client', 'autotest', client_dir,
                         action=action)
        process_packages(pkgmgr, 'dep', deps, dep_dir, action=action)
        process_packages(pkgmgr, 'profiler', profilers, prof_dir,
                         action=action)


# Get the list of sub directories present in a directory
def get_subdir_list(name, client_dir):
    dir_name = os.path.join(client_dir, name)
    return [f for f in
            os.listdir(dir_name)
            if os.path.isdir(os.path.join(dir_name, f)) ]


# Look whether the test is present in client/tests and client/site_tests dirs
def get_test_dir(name, client_dir):
    names_test = os.listdir(os.path.join(client_dir, 'tests'))
    names_site_test = os.listdir(os.path.join(client_dir, 'site_tests'))
    if name in names_test:
        src_dir = os.path.join(client_dir, 'tests')
    elif name in names_site_test:
        src_dir = os.path.join(client_dir, 'site_tests')
    else:
        print "Test %s not found" % name
        sys.exit(0)
    return src_dir


def main():
    # get options and args
    options, args = parse_args()

    server_dir = server_utils.get_server_dir()
    autotest_dir = os.path.abspath(os.path.join(server_dir, '..'))

    # extract the pkg locations from global config
    repo_urls = c.get_config_value('PACKAGES', 'fetch_location',
                                   type=list, default=[])
    upload_paths = c.get_config_value('PACKAGES', 'upload_location',
                                      type=list, default=[])

    if options.repo:
        upload_paths.append(options.repo)

    # Having no upload paths basically means you're not using packaging.
    if not upload_paths:
        print("No upload locations found. Please set upload_location under"
              " PACKAGES in the global_config.ini or provide a location using"
              " the --repository option.")
        return

    client_dir = os.path.join(autotest_dir, "client")

    # Bail out if the client directory does not exist
    if not os.path.exists(client_dir):
        sys.exit(0)

    dep_dir = os.path.join(client_dir, "deps")
    prof_dir = os.path.join(client_dir, "profilers")

    # Due to the delayed uprev-ing of certain ebuilds, we need to support
    # both the legacy command line and the new one.
    # So, if the new "action" option isn't specified, try looking for the
    # old style remove/upload argument
    if options.action is None:
        if len(args) == 0 or args[0] not in ['upload', 'remove']:
            print("Either 'upload' or 'remove' needs to be specified "
                  "for the package")
            sys.exit(0)
        cur_action = args[0]
    else:
        cur_action = options.action

    if cur_action == ACTION_TAR_ONLY and options.output_dir is None:
        print("An output dir has to be specified")
        sys.exit(0)

    pkgmgr = packages.PackageManager(autotest_dir, repo_urls=repo_urls,
                                     upload_paths=upload_paths,
                                     run_function_dargs={'timeout':600})

    if options.all:
        process_all_packages(pkgmgr, client_dir, action=cur_action)

    if options.client:
        process_packages(pkgmgr, 'client', 'autotest', client_dir,
                         action=cur_action)

    if options.dep:
        process_packages(pkgmgr, 'dep', options.dep, dep_dir,
                         action=cur_action, dest_dir=options.output_dir)

    if options.test:
        process_packages(pkgmgr, 'test', options.test, client_dir,
                         action=cur_action, dest_dir=options.output_dir)

    if options.prof:
        process_packages(pkgmgr, 'profiler', options.prof, prof_dir,
                         action=cur_action, dest_dir=options.output_dir)

    if options.file:
        if cur_action == ACTION_REMOVE:
            pkgmgr.remove_pkg(options.file, remove_checksum=True)
        elif cur_action == ACTION_UPLOAD:
            pkgmgr.upload_pkg(options.file, update_checksum=True)


if __name__ == "__main__":
    main()
