"""
Autotest tempfile wrapper for mkstemp (known as tempfile here) and
mkdtemp (known as tempdir).

This wrapper provides a mechanism to clean up temporary files/dirs once they
are no longer need.

Files/Dirs will have a unique_id prepended to the suffix and a
_autotmp_ tag appended to the prefix.

It is required that the unique_id param is supplied when a temp dir/file is
created.
"""

import shutil, os, logging
import tempfile as module_tempfile

_TEMPLATE = '_autotmp_'


class tempfile(object):
    """
    A wrapper for tempfile.mkstemp

    @param unique_id: required, a unique string to help identify what
                      part of code created the tempfile.
    @var name: The name of the temporary file.
    @var fd:  the file descriptor of the temporary file that was created.
    @return a tempfile object
    example usage:
        t = autotemp.tempfile(unique_id='fig')
        t.name # name of file
        t.fd   # file descriptor
        t.fo   # file object
        t.clean() # clean up after yourself
    """
    def __init__(self, unique_id, suffix='', prefix='', dir=None,
                 text=False):
        suffix = unique_id + suffix
        prefix = prefix + _TEMPLATE
        self.fd, self.name = module_tempfile.mkstemp(suffix=suffix,
                                                     prefix=prefix,
                                                     dir=dir, text=text)
        self.fo = os.fdopen(self.fd)


    def clean(self):
        """
        Remove the temporary file that was created.
        This is also called by the destructor.
        """
        if self.fo:
            self.fo.close()
        if self.name and os.path.exists(self.name):
            os.remove(self.name)

        self.fd = self.fo = self.name = None


    def __del__(self):
        try:
            if self.name is not None:
                logging.debug('Cleaning %s', self.name)
                self.clean()
        except:
            try:
                msg = 'An exception occurred while calling the destructor'
                logging.exception(msg)
            except:
                pass


class tempdir(object):
    """
    A wrapper for tempfile.mkdtemp

    @var name: The name of the temporary dir.
    @return A tempdir object
    example usage:
        b = autotemp.tempdir(unique_id='exemdir')
        b.name # your directory
        b.clean() # clean up after yourself
    """
    def __init__(self,  suffix='', unique_id='', prefix='', dir=None):
        """
        Initialize temp directory.

        @param suffix: suffix for dir.
        @param prefix: prefix for dir. Defaults to '_autotmp'.
        @param unique_id: unique id of tempdir.
        @param dir: parent directory of the tempdir. Defaults to /tmp.

        eg: autotemp.tempdir(suffix='suffix', unique_id='123', prefix='prefix')
            creates a dir like '/tmp/prefix_autotmp_<random hash>123suffix'
        """
        suffix = unique_id + suffix
        prefix = prefix + _TEMPLATE
        self.name = module_tempfile.mkdtemp(suffix=suffix,
                                            prefix=prefix, dir=dir)


    def clean(self):
        """
        Remove the temporary dir that was created.
        This is also called by the destructor.
        """
        if self.name and os.path.exists(self.name):
            shutil.rmtree(self.name)

        self.name = None


    def __del__(self):
        try:
            if self.name:
                logging.debug('Clean was not called for ' + self.name)
                self.clean()
        except:
            try:
                msg = 'An exception occurred while calling the destructor'
                logging.exception(msg)
            except:
                pass


class dummy_dir(object):
    """A dummy object representing a directory with a name.

    Only used for compat with the tmpdir, in cases where we wish to
    reuse a dir with the same interface but not to delete it after
    we're done using it.
    """

    def __init__(self, name):
        """Initialize the dummy_dir object.

        @param name: Path the the directory.
        """
        self.name = name
