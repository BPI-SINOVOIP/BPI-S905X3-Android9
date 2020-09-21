class Configuration(object):
    """
    Configuration that can be changed for configurable CFM tests.
    """
    def __init__(self, run_test_only=False, skip_enrollment=False):
        """
        Initializes.

        @param run_test_only Whether to run only the test or to also perform
            deprovisioning, enrollment and system reboot. If set to 'True',
            the DUT must already be enrolled and past the OOB screen to be able
            to execute the test.
        @param skip_enrollment Whether to skip the enrollment step. Cleanup
            at the end of the test is done regardless.
        """
        self.run_test_only = run_test_only
        self.skip_enrollment = skip_enrollment
