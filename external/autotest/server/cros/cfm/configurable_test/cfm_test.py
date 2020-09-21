from autotest_lib.server.cros.cfm.configurable_test import configuration

class CfmTest(object):
    """
    Specification of one configurable CFM test.
    """

    def __init__(self, scenario, configuration=configuration.Configuration()):
        """
        Initializes.

        @param scenario The scenario to run.
        @param configuration The configuration to use. Optional, defaults
                to configuration with default values.
        """
        self.scenario = scenario
        self.configuration = configuration

    def __str__(self):
        return ('CfmTest\n  scenario: %s\n  configuration: %s'
                        % (self.scenario, self.configuration))


