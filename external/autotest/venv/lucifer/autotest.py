# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Kludges to support legacy Autotest code.

Autotest imports should be done by calling monkeypatch() first and then
calling load().  monkeypatch() should only be called once from a
script's main function.

chromite imports should be done with chromite_load(), and any third
party packages should be imported with deps_load().  The reason for this
is to present a clear API for these unsafe imports, making it easier to
identify which imports are currently unsafe.  Eventually, everything
should be moved to virtualenv, but that will not be in the near future.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import ast
import contextlib
import imp
import importlib
import logging
import os
import site
import subprocess
import sys

import autotest_lib

AUTOTEST_DIR = autotest_lib.__path__[0]
_SITEPKG_DIR = os.path.join(AUTOTEST_DIR, 'site-packages')
_SYSTEM_PYTHON = '/usr/bin/python2.7'

_setup_done = False

logger = logging.getLogger(__name__)


def monkeypatch():
    """Monkeypatch everything needed to import Autotest.

    This should be called before any calls to load().  Only the main
    function in scripts should call this function.
    """
    with _global_setup():
        _monkeypatch_body()


@contextlib.contextmanager
def _global_setup():
    """Context manager for checking and setting global _setup_done variable."""
    global _setup_done
    assert not _setup_done
    try:
        yield
    except Exception:  # pragma: no cover
        # We cannot recover from this since we leave the interpreter in
        # an unknown state.
        logger.exception('Uncaught exception escaped Autotest setup')
        sys.exit(1)
    else:
        _setup_done = True


def _monkeypatch_body():
    """The body of monkeypatch() running within _global_setup() context."""
    # Add Autotest's site-packages.
    site.addsitedir(_SITEPKG_DIR)

    # Dummy out common imports as they may cause problems.
    sys.meta_path.insert(0, _CommonRemovingFinder())

    # Add chromite's third-party to the import path (chromite does this
    # on import).
    try:
        importlib.import_module('chromite')
    except ImportError:
        # Moblab does not run build_externals; dependencies like
        # chromite are installed system-wide.
        logger.info("""\
Could not find chromite; adding system packages and retrying \
(This should only happen on Moblab)""")
        for d in _system_site_packages():
            site.addsitedir(d)
        importlib.import_module('chromite')

    # Set up Django environment variables.
    importlib.import_module('autotest_lib.frontend.setup_django_environment')

    # Make Django app paths absolute.
    settings = importlib.import_module('autotest_lib.frontend.settings')
    settings.INSTALLED_APPS = (
            'autotest_lib.frontend.afe',
            'autotest_lib.frontend.tko',
            'django.contrib.admin',
            'django.contrib.auth',
            'django.contrib.contenttypes',
            'django.contrib.sessions',
            'django.contrib.sites',
    )

    # drone_utility uses this.
    common = importlib.import_module('autotest_lib.scheduler.common')
    common.autotest_dir = AUTOTEST_DIR


def _system_site_packages():
    """Get list of system site-package directories.

    This is needed for Moblab because dependencies are installed
    system-wide instead of using build_externals.py.
    """
    output = subprocess.check_output([
        _SYSTEM_PYTHON, '-c',
        'import site; print repr(site.getsitepackages())'])
    return ast.literal_eval(output)


class _CommonRemovingFinder(object):
    """Python import finder that neuters Autotest's common.py

    The common module is replaced with an empty module everywhere it is
    imported.  common.py should have only been imported for side
    effects, so nothing should actually use the imported module.

    See also https://www.python.org/dev/peps/pep-0302/
    """

    def find_module(self, fullname, path=None):
        """Find module."""
        del path  # unused
        if not self._is_autotest_common(fullname):
            return None
        logger.debug('Dummying out %s import', fullname)
        return self

    def _is_autotest_common(self, fullname):
        return (fullname.partition('.')[0] == 'autotest_lib'
                and fullname.rpartition('.')[-1] == 'common')

    def load_module(self, fullname):
        """Load module."""
        if fullname in sys.modules:  # pragma: no cover
            return sys.modules[fullname]
        mod = imp.new_module(fullname)
        mod.__file__ = '<removed>'
        mod.__loader__ = self
        mod.__package__ = fullname.rpartition('.')[0]
        sys.modules[fullname] = mod
        return mod


def load(name):
    """Import module from autotest.

    This enforces that monkeypatch() is called first.

    @param name: name of module as string, e.g., 'frontend.afe.models'
    """
    return _load('autotest_lib.%s' % name)


def chromite_load(name):
    """Import module from chromite.lib.

    This enforces that monkeypatch() is called first.

    @param name: name of module as string, e.g., 'metrics'
    """
    return _load('chromite.lib.%s' % name)


def deps_load(name):
    """Import module from chromite.lib.

    This enforces that monkeypatch() is called first.

    @param name: name of module as string, e.g., 'metrics'
    """
    assert not name.startswith('autotest_lib')
    assert not name.startswith('chromite.lib')
    return _load(name)


def _load(name):
    """Import a module.

    This enforces that monkeypatch() is called first.

    @param name: name of module as string
    """
    if not _setup_done:
        raise ImportError('cannot load chromite modules before monkeypatching')
    return importlib.import_module(name)
