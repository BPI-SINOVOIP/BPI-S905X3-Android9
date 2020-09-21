# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Wrapper class to store size related information of test results.
"""

import contextlib
import json
import os

import result_info_lib
import utils_lib


class ResultInfoError(Exception):
    """Exception to raise when error occurs in ResultInfo collection."""


class ResultInfo(dict):
    """A wrapper class to store result file information.

    Details of a result include:
    original_size: Original size in bytes of the result, before throttling.
    trimmed_size: Size in bytes after the result is throttled.
    collected_size: Size in bytes of the results collected from the dut.
    files: A list of ResultInfo for the files and sub-directories of the result.

    The class contains the size information of a result file/directory, and the
    information can be merged if a file was collected multiple times during
    the test.
    For example, `messages` of size 100 bytes was collected before the test
    starts, ResultInfo for this file shall be:
        {'messages': {'/S': 100}}
    Later in the test, the file was collected again when it's size becomes 200
    bytes, the new ResultInfo will be:
        {'messages': {'/S': 200}}

    Not that the result infos collected from the dut don't have collected_size
    (/C) set. That's because the collected size in such case is equal to the
    trimmed_size (/T). If the reuslt is not trimmed and /T is not set, the
    value of collected_size can fall back to original_size. The design is to not
    to inject duplicated information in the summary json file, thus reduce the
    size of data needs to be transfered from the dut.

    At the end of the test, the file is considered too big, and trimmed down to
    150 bytes, thus the final ResultInfo of the file becomes:
        {'messages': {# The original size is 200 bytes
                      '/S': 200,
                      # The total collected size is 300(100+200} bytes
                      '/C': 300,
                      # The trimmed size is the final size on disk
                      '/T': 150}
    From this example, the original size tells us how large the file was.
    The collected size tells us how much data was transfered from dut to drone
    to get this file. And the trimmed size shows the final size of the file when
    the test is finished and the results are throttled again on the server side.

    The class is a wrapper of dictionary. The properties are all keyvals in a
    dictionary. For example, an instance of ResultInfo can have following
    dictionary value:
    {'debug': {
            # Original size of the debug folder is 1000 bytes.
            '/S': 1000,
            # The debug folder was throttled and the size is reduced to 500
            # bytes.
            '/T': 500,
            # collected_size ('/C') can be ignored, its value falls back to
            # trimmed_size ('/T'). If trimmed_size is not set, its value falls
            # back to original_size ('S')

            # Sub-files and sub-directories are included in a list of '/D''s
            # value.
            # In this example, debug folder has a file `file1`, whose original
            # size is 1000 bytes, which is trimmed down to 500 bytes.
            '/D': [
                    {'file1': {
                            '/S': 1000,
                            '/T': 500,
                        }
                    }
                ]
        }
    }
    """

    def __init__(self, parent_dir, name=None, parent_result_info=None,
                 original_info=None):
        """Initialize a collection of size information for a given result path.

        A ResultInfo object can be initialized in two ways:
        1. Create from a physical file, which reads the size from the file.
           In this case, `name` value should be given, and `original_info`
           should not be set.
        2. Create from previously collected information, i.e., a dictionary
           deserialized from persisted json file. In this case, `original_info`
           should be given, and `name` should not be set.

        @param parent_dir: Path to the parent directory.
        @param name: Name of the result file or directory.
        @param parent_result_info: A ResultInfo object for the parent directory.
        @param original_info: A dictionary of the result's size information.
                This is retrieved from the previously serialized json string.
                For example: {'file_name':
                            {'/S': 100, '/T': 50}
                         }
                which means a file's original size is 100 bytes, and trimmed
                down to 50 bytes. This argument is used when the object is
                restored from a json string.
        """
        super(ResultInfo, self).__init__()

        if name is not None and original_info is not None:
            raise ResultInfoError(
                    'Only one of parameter `name` and `original_info` can be '
                    'set.')

        # _initialized is a flag to indicating the object is in constructor.
        # It can be used to block any size update to make restoring from json
        # string faster. For example, if file_details has sub-directories,
        # all sub-directories will be added to this class recursively, blocking
        # the size updates can reduce unnecessary calculations.
        self._initialized = False
        self._parent_result_info = parent_result_info

        if original_info is None:
            self._init_from_file(parent_dir, name)
        else:
            self._init_with_original_info(parent_dir, original_info)

        # Size of bytes collected in an overwritten or removed directory.
        self._previous_collected_size = 0
        self._initialized = True

    def _init_from_file(self, parent_dir, name):
        """Initialize with the physical file.

        @param parent_dir: Path to the parent directory.
        @param name: Name of the result file or directory.
        """
        assert name != None
        self._name = name

        # Dictionary to store details of the given path is set to a keyval of
        # the wrapper class. Save the dictionary to an attribute for faster
        # access.
        self._details = {}
        self[self.name] = self._details

        # rstrip is to remove / when name is ROOT_DIR ('').
        self._path = os.path.join(parent_dir, self.name).rstrip(os.sep)
        self._is_dir = os.path.isdir(self._path)

        if self.is_dir:
            # The value of key utils_lib.DIRS is a list of ResultInfo objects.
            self.details[utils_lib.DIRS] = []

        # Set original size to be the physical size if file details are not
        # given and the path is for a file.
        if self.is_dir:
            # Set directory size to 0, it will be updated later after its
            # sub-directories are added.
            self.original_size = 0
        else:
            self.original_size = self.size

    def _init_with_original_info(self, parent_dir, original_info):
        """Initialize with pre-collected information.

        @param parent_dir: Path to the parent directory.
        @param original_info: A dictionary of the result's size information.
                This is retrieved from the previously serialized json string.
                For example: {'file_name':
                            {'/S': 100, '/T': 50}
                         }
                which means a file's original size is 100 bytes, and trimmed
                down to 50 bytes. This argument is used when the object is
                restored from a json string.
        """
        assert original_info
        # The result information dictionary has only 1 key, which is the file or
        # directory name.
        self._name = original_info.keys()[0]

        # Dictionary to store details of the given path is set to a keyval of
        # the wrapper class. Save the dictionary to an attribute for faster
        # access.
        self._details = {}
        self[self.name] = self._details

        # rstrip is to remove / when name is ROOT_DIR ('').
        self._path = os.path.join(parent_dir, self.name).rstrip(os.sep)

        self._is_dir = utils_lib.DIRS in original_info[self.name]

        if self.is_dir:
            # The value of key utils_lib.DIRS is a list of ResultInfo objects.
            self.details[utils_lib.DIRS] = []

        # This is restoring ResultInfo from a json string.
        self.original_size = original_info[self.name][
                utils_lib.ORIGINAL_SIZE_BYTES]
        if utils_lib.TRIMMED_SIZE_BYTES in original_info[self.name]:
            self.trimmed_size = original_info[self.name][
                    utils_lib.TRIMMED_SIZE_BYTES]
        if self.is_dir:
            dirs = original_info[self.name][utils_lib.DIRS]
            # TODO: Remove this conversion after R62 is in stable channel.
            if isinstance(dirs, dict):
                # The summary is generated from older format which stores sub-
                # directories in a dictionary, rather than a list. Convert the
                # data in old format to a list of dictionary.
                dirs = [{dir_name: dirs[dir_name]} for dir_name in dirs]
            for sub_file in dirs:
                self.add_file(None, sub_file)

    @contextlib.contextmanager
    def disable_updating_parent_size_info(self):
        """Disable recursive calls to update parent result_info's sizes.

        This context manager allows removing sub-directories to run faster
        without triggering recursive calls to update parent result_info's sizes.
        """
        old_value = self._initialized
        self._initialized = False
        try:
            yield
        finally:
            self._initialized = old_value

    def update_dir_original_size(self):
        """Update all directories' original size information.
        """
        for f in [f for f in self.files if f.is_dir]:
            f.update_dir_original_size()
        self.update_original_size(skip_parent_update=True)

    @staticmethod
    def build_from_path(parent_dir,
                        name=utils_lib.ROOT_DIR,
                        parent_result_info=None, top_dir=None,
                        all_dirs=None):
        """Get the ResultInfo for the given path.

        @param parent_dir: The parent directory of the given file.
        @param name: Name of the result file or directory.
        @param parent_result_info: A ResultInfo instance for the parent
                directory.
        @param top_dir: The top directory to collect ResultInfo. This is to
                check if a directory is a subdir of the original directory to
                collect summary.
        @param all_dirs: A set of paths that have been collected. This is to
                prevent infinite recursive call caused by symlink.

        @return: A ResultInfo instance containing the directory summary.
        """
        is_top_level = top_dir is None
        top_dir = top_dir or parent_dir
        all_dirs = all_dirs or set()

        # If the given parent_dir is a file and name is ROOT_DIR, that means
        # the ResultInfo is for a single file with root directory of the default
        # ROOT_DIR.
        if not os.path.isdir(parent_dir) and name == utils_lib.ROOT_DIR:
            root_dir = os.path.dirname(parent_dir)
            dir_info = ResultInfo(parent_dir=root_dir,
                                  name=utils_lib.ROOT_DIR)
            dir_info.add_file(os.path.basename(parent_dir))
            return dir_info

        dir_info = ResultInfo(parent_dir=parent_dir,
                              name=name,
                              parent_result_info=parent_result_info)

        path = os.path.join(parent_dir, name)
        if os.path.isdir(path):
            real_path = os.path.realpath(path)
            # The assumption here is that results are copied back to drone by
            # copying the symlink, not the content, which is true with currently
            # used rsync in cros_host.get_file call.
            # Skip scanning the child folders if any of following condition is
            # true:
            # 1. The directory is a symlink and link to a folder under `top_dir`
            # 2. The directory was scanned already.
            if ((os.path.islink(path) and real_path.startswith(top_dir)) or
                real_path in all_dirs):
                return dir_info
            all_dirs.add(real_path)
            for f in sorted(os.listdir(path)):
                dir_info.files.append(ResultInfo.build_from_path(
                        parent_dir=path,
                        name=f,
                        parent_result_info=dir_info,
                        top_dir=top_dir,
                        all_dirs=all_dirs))

        # Update all directory's original size at the end of the tree building.
        if is_top_level:
            dir_info.update_dir_original_size()

        return dir_info

    @property
    def details(self):
        """Get the details of the result.

        @return: A dictionary of size and sub-directory information.
        """
        return self._details

    @property
    def is_dir(self):
        """Get if the result is a directory.
        """
        return self._is_dir

    @property
    def name(self):
        """Name of the result.
        """
        return self._name

    @property
    def path(self):
        """Full path to the result.
        """
        return self._path

    @property
    def files(self):
        """All files or sub-directories of the result.

        @return: A list of ResultInfo objects.
        @raise ResultInfoError: If the result is not a directory.
        """
        if not self.is_dir:
            raise ResultInfoError('%s is not a directory.' % self.path)
        return self.details[utils_lib.DIRS]

    @property
    def size(self):
        """Physical size in bytes for the result file.

        @raise ResultInfoError: If the result is a directory.
        """
        if self.is_dir:
            raise ResultInfoError(
                    '`size` property does not support directory. Try to use '
                    '`original_size` property instead.')
        return result_info_lib.get_file_size(self._path)

    @property
    def original_size(self):
        """The original size in bytes of the result before it's throttled.
        """
        return self.details[utils_lib.ORIGINAL_SIZE_BYTES]

    @original_size.setter
    def original_size(self, value):
        """Set the original size in bytes of the result.

        @param value: The original size in bytes of the result.
        """
        self.details[utils_lib.ORIGINAL_SIZE_BYTES] = value
        # Update the size of parent result infos if the object is already
        # initialized.
        if self._initialized and self._parent_result_info is not None:
            self._parent_result_info.update_original_size()

    @property
    def trimmed_size(self):
        """The size in bytes of the result after it's throttled.
        """
        return self.details.get(utils_lib.TRIMMED_SIZE_BYTES,
                                self.original_size)

    @trimmed_size.setter
    def trimmed_size(self, value):
        """Set the trimmed size in bytes of the result.

        @param value: The trimmed size in bytes of the result.
        """
        self.details[utils_lib.TRIMMED_SIZE_BYTES] = value
        # Update the size of parent result infos if the object is already
        # initialized.
        if self._initialized and self._parent_result_info is not None:
            self._parent_result_info.update_trimmed_size()

    @property
    def collected_size(self):
        """The collected size in bytes of the result.

        The file is throttled on the dut, so the number of bytes collected from
        dut is default to the trimmed_size. If a file is modified between
        multiple result collections and is collected multiple times during the
        test run, the collected_size will be the sum of the multiple
        collections. Therefore, its value will be greater than the trimmed_size
        of the last copy.
        """
        return self.details.get(utils_lib.COLLECTED_SIZE_BYTES,
                                self.trimmed_size)

    @collected_size.setter
    def collected_size(self, value):
        """Set the collected size in bytes of the result.

        @param value: The collected size in bytes of the result.
        """
        self.details[utils_lib.COLLECTED_SIZE_BYTES] = value
        # Update the size of parent result infos if the object is already
        # initialized.
        if self._initialized and self._parent_result_info is not None:
            self._parent_result_info.update_collected_size()

    @property
    def is_collected_size_recorded(self):
        """Flag to indicate if the result has collected size set.

        This flag is used to avoid unnecessary entry in result details, as the
        default value of collected size is the trimmed size. Removing the
        redundant information helps to reduce the size of the json file.
        """
        return utils_lib.COLLECTED_SIZE_BYTES in self.details

    @property
    def parent_result_info(self):
        """The result info of the parent directory.
        """
        return self._parent_result_info

    def add_file(self, name, original_info=None):
        """Add a file to the result.

        @param name: Name of the file.
        @param original_info: A dictionary of the file's size and sub-directory
                information.
        """
        self.details[utils_lib.DIRS].append(
                ResultInfo(parent_dir=self._path,
                           name=name,
                           parent_result_info=self,
                           original_info=original_info))
        # After a new ResultInfo is added, update the sizes if the object is
        # already initialized.
        if self._initialized:
            self.update_sizes()

    def remove_file(self, name):
        """Remove a file with the given name from the result.

        @param name: Name of the file to be removed.
        """
        self.files.remove(self.get_file(name))
        # After a new ResultInfo is removed, update the sizes if the object is
        # already initialized.
        if self._initialized:
            self.update_sizes()

    def get_file_names(self):
        """Get a set of all the files under the result.
        """
        return set([f.keys()[0] for f in self.files])

    def get_file(self, name):
        """Get a file with the given name under the result.

        @param name: Name of the file.
        @return: A ResultInfo object of the file.
        @raise ResultInfoError: If the result is not a directory, or the file
                with the given name is not found.
        """
        if not self.is_dir:
            raise ResultInfoError('%s is not a directory. Can\'t locate file '
                                  '%s' % (self.path, name))
        for file_info in self.files:
            if file_info.name == name:
                return file_info
        raise ResultInfoError('Can\'t locate file %s in directory %s' %
                              (name, self.path))

    def convert_to_dir(self):
        """Convert the result file to a directory.

        This happens when a result file was overwritten by a directory. The
        conversion will reset the details of this result to be a directory,
        and save the collected_size to attribute `_previous_collected_size`,
        so it can be counted when merging multiple result infos.

        @raise ResultInfoError: If the result is already a directory.
        """
        if self.is_dir:
            raise ResultInfoError('%s is already a directory.' % self.path)
        # The size that's collected before the file was replaced as a directory.
        collected_size = self.collected_size
        self._is_dir = True
        self.details[utils_lib.DIRS] = []
        self.original_size = 0
        self.trimmed_size = 0
        self._previous_collected_size = collected_size
        self.collected_size = collected_size

    def update_original_size(self, skip_parent_update=False):
        """Update the original size of the result and trigger its parent to
        update.

        @param skip_parent_update: True to skip updating parent directory's
                original size. Default is set to False.
        """
        if self.is_dir:
            self.original_size = sum([
                    f.original_size for f in self.files])
        elif self.original_size is None:
            # Only set original_size if it's not initialized yet.
            self.orginal_size = self.size

        # Update the size of parent result infos.
        if not skip_parent_update and self._parent_result_info is not None:
            self._parent_result_info.update_original_size()

    def update_trimmed_size(self):
        """Update the trimmed size of the result and trigger its parent to
        update.
        """
        if self.is_dir:
            new_trimmed_size = sum([f.trimmed_size for f in self.files])
        else:
            new_trimmed_size = self.size

        # Only set trimmed_size if the value is changed or different from the
        # original size.
        if (new_trimmed_size != self.original_size or
            new_trimmed_size != self.trimmed_size):
            self.trimmed_size = new_trimmed_size

        # Update the size of parent result infos.
        if self._parent_result_info is not None:
            self._parent_result_info.update_trimmed_size()

    def update_collected_size(self):
        """Update the collected size of the result and trigger its parent to
        update.
        """
        if self.is_dir:
            new_collected_size = (
                    self._previous_collected_size +
                    sum([f.collected_size for f in self.files]))
        else:
            new_collected_size = self.size

        # Only set collected_size if the value is changed or different from the
        # trimmed size or existing collected size.
        if (new_collected_size != self.trimmed_size or
            new_collected_size != self.collected_size):
            self.collected_size = new_collected_size

        # Update the size of parent result infos.
        if self._parent_result_info is not None:
            self._parent_result_info.update_collected_size()

    def update_sizes(self):
        """Update all sizes information of the result.
        """
        self.update_original_size()
        self.update_trimmed_size()
        self.update_collected_size()

    def set_parent_result_info(self, parent_result_info, update_sizes=True):
        """Set the parent result info.

        It's used when a ResultInfo object is moved to a different file
        structure.

        @param parent_result_info: A ResultInfo object for the parent directory.
        @param update_sizes: True to update the parent's size information. Set
                it to False to delay the update for better performance.
        """
        self._parent_result_info = parent_result_info
        # As the parent reference changed, update all sizes of the parent.
        if parent_result_info and update_sizes:
            self._parent_result_info.update_sizes()

    def merge(self, new_info, is_final=False):
        """Merge a ResultInfo instance to the current one.

        Update the old directory's ResultInfo with the new one. Also calculate
        the total size of results collected from the client side based on the
        difference between the two ResultInfo.

        When merging with newer collected results, any results not existing in
        the new ResultInfo or files with size different from the newer files
        collected are considered as extra results collected or overwritten by
        the new results.
        Therefore, the size of the collected result should include such files,
        and the collected size can be larger than trimmed size.
        As an example:
        current: {'file1': {TRIMMED_SIZE_BYTES: 1024,
                            ORIGINAL_SIZE_BYTES: 1024,
                            COLLECTED_SIZE_BYTES: 1024}}
        This means a result `file1` of original size 1KB was collected with size
        of 1KB byte.
        new_info: {'file1': {TRIMMED_SIZE_BYTES: 1024,
                             ORIGINAL_SIZE_BYTES: 2048,
                             COLLECTED_SIZE_BYTES: 1024}}
        This means a result `file1` of 2KB was trimmed down to 1KB and was
        collected with size of 1KB byte.
        Note that the second result collection has an updated result `file1`
        (because of the different ORIGINAL_SIZE_BYTES), and it needs to be
        rsync-ed to the drone. Therefore, the merged ResultInfo will be:
        {'file1': {TRIMMED_SIZE_BYTES: 1024,
                   ORIGINAL_SIZE_BYTES: 2048,
                   COLLECTED_SIZE_BYTES: 2048}}
        Note that:
        * TRIMMED_SIZE_BYTES is still at 1KB, which reflects the actual size of
          the file be collected.
        * ORIGINAL_SIZE_BYTES is updated to 2KB, which is the size of the file
          in the new result `file1`.
        * COLLECTED_SIZE_BYTES is 2KB because rsync will copy `file1` twice as
          it's changed.

        The only exception is that the new ResultInfo's ORIGINAL_SIZE_BYTES is
        the same as the current ResultInfo's TRIMMED_SIZE_BYTES. That means the
        file was trimmed in the current ResultInfo and the new ResultInfo is
        collecting the trimmed file. Therefore, the merged summary will keep the
        data in the current ResultInfo.

        @param new_info: New ResultInfo to be merged into the current one.
        @param is_final: True if new_info is built from the final result folder.
                Default is set to False.
        """
        new_files = new_info.get_file_names()
        old_files = self.get_file_names()
        # A flag to indicate if the sizes need to be updated. It's required when
        # child result_info is added to `self`.
        update_sizes_pending = False
        for name in new_files:
            new_file = new_info.get_file(name)
            if not name in old_files:
                # A file/dir exists in new client dir, but not in the old one,
                # which means that the file or a directory is newly collected.
                self.files.append(new_file)
                # Once parent_result_info is changed, new_file object will no
                # longer associated with `new_info` object.
                new_file.set_parent_result_info(self, update_sizes=False)
                update_sizes_pending = True
            elif new_file.is_dir:
                # `name` is a directory in the new ResultInfo, try to merge it
                # with the current ResultInfo.
                old_file = self.get_file(name)

                if not old_file.is_dir:
                    # If `name` is a file in the current ResultInfo but a
                    # directory in new ResultInfo, the file in the current
                    # ResultInfo will be overwritten by the new directory by
                    # rsync. Therefore, force it to be an empty directory in
                    # the current ResultInfo, so that the new directory can be
                    # merged.
                    old_file.convert_to_dir()

                old_file.merge(new_file, is_final)
            else:
                old_file = self.get_file(name)

                # If `name` is a directory in the current ResultInfo, but a file
                # in the new ResultInfo, rsync will fail to copy the file as it
                # can't overwrite an directory. Therefore, skip the merge.
                if old_file.is_dir:
                    continue

                new_size = new_file.original_size
                old_size = old_file.original_size
                new_trimmed_size = new_file.trimmed_size
                old_trimmed_size = old_file.trimmed_size

                # Keep current information if the sizes are not changed.
                if (new_size == old_size and
                    new_trimmed_size == old_trimmed_size):
                    continue

                # Keep current information if the newer size is the same as the
                # current trimmed size, and the file is not trimmed in new
                # ResultInfo. That means the file was trimmed earlier and stays
                # the same when collecting the information again.
                if (new_size == old_trimmed_size and
                    new_size == new_trimmed_size):
                    continue

                # If the file is merged from the final result folder to an older
                # ResultInfo, it's not considered to be trimmed if the size is
                # not changed. The reason is that the file on the server side
                # does not have the info of its original size.
                if is_final and new_trimmed_size == old_trimmed_size:
                    continue

                # `name` is a file, and both the original_size and trimmed_size
                # are changed, that means the file is overwritten, so increment
                # the collected_size.
                # Before trimming is implemented, collected_size is the
                # value of original_size.
                new_collected_size = new_file.collected_size
                old_collected_size = old_file.collected_size

                old_file.collected_size = (
                        new_collected_size + old_collected_size)
                # Only set trimmed_size if one of the following two conditions
                # are true:
                # 1. In the new summary the file's trimmed size is different
                #    from the original size, which means the file was trimmed
                #    in the new summary.
                # 2. The original size in the new summary equals the trimmed
                #    size in the old summary, which means the file was trimmed
                #    again in the new summary.
                if (new_size == old_trimmed_size or
                    new_size != new_trimmed_size):
                    old_file.trimmed_size = new_file.trimmed_size
                old_file.original_size = new_size

        if update_sizes_pending:
            self.update_sizes()


# An empty directory, used to compare with a ResultInfo.
EMPTY = ResultInfo(parent_dir='',
                   original_info={'': {utils_lib.ORIGINAL_SIZE_BYTES: 0,
                                       utils_lib.DIRS: []}})


def save_summary(summary, json_file):
    """Save the given directory summary to a file.

    @param summary: A ResultInfo object for a result directory.
    @param json_file: Path to a json file to save to.
    """
    with open(json_file, 'w') as f:
        json.dump(summary, f)


def load_summary_json_file(json_file):
    """Load result info from the given json_file.

    @param json_file: Path to a json file containing a directory summary.
    @return: A ResultInfo object containing the directory summary.
    """
    with open(json_file, 'r') as f:
        summary = json.load(f)

    # Convert summary to ResultInfo objects
    result_dir = os.path.dirname(json_file)
    return ResultInfo(parent_dir=result_dir, original_info=summary)
