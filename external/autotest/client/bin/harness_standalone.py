"""The standalone harness interface

The default interface as required for the standalone reboot helper.
"""

__author__ = """Copyright Andy Whitcroft 2007"""

import os, harness

class harness_standalone(harness.harness):
    """The standalone server harness

    Properties:
            job
                    The job object for this job
    """

    def __init__(self, job, harness_args):
        """
                job
                        The job object for this job
        """
        self.autodir = os.path.abspath(os.environ['AUTODIR'])
        self.setup(job)