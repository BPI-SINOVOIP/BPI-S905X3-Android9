# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import sys

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


def cleanup_if_fail():
    """Decorator to do cleanup if container fails to be set up.
    """
    def deco_cleanup_if_fail(func):
        """Wrapper for the decorator.

        @param func: Function to be called.
        """
        def func_cleanup_if_fail(*args, **kwargs):
            """Decorator to do cleanup if container fails to be set up.

            The first argument must be a ContainerBucket object, which can be
            used to retrieve the container object by name.

            @param func: function to be called.
            @param args: arguments for function to be called.
            @param kwargs: keyword arguments for function to be called.
            """
            bucket = args[0]
            container_id = utils.get_function_arg_value(
                    func, 'container_id', args, kwargs)
            try:
                skip_cleanup = utils.get_function_arg_value(
                        func, 'skip_cleanup', args, kwargs)
            except (KeyError, ValueError):
                skip_cleanup = False
            try:
                return func(*args, **kwargs)
            except:
                exc_info = sys.exc_info()
                try:
                    container = bucket.get_container(container_id)
                    if container and not skip_cleanup:
                        container.destroy()
                except error.CmdError as e:
                    logging.error(e)

                # Raise the cached exception with original backtrace.
                raise exc_info[0], exc_info[1], exc_info[2]
        return func_cleanup_if_fail
    return deco_cleanup_if_fail
