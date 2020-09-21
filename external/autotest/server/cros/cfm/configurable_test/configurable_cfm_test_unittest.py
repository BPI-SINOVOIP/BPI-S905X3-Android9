import unittest

from mock import MagicMock

from autotest_lib.server.cros.cfm.configurable_test import action_context
from autotest_lib.server.cros.cfm.configurable_test import configurable_cfm_test
from autotest_lib.server.cros.cfm.configurable_test.dsl import *
from autotest_lib.server.cros.multimedia import cfm_facade_adapter

# Test, disable missing-docstring
# Also, disable undefined-variable since the import * confuses the linter (we
# want to use import * since that is what the control files will use)
# pylint: disable=missing-docstring,undefined-variable
class TestConfigurableCfmTest(unittest.TestCase):
    """Tests running configurable CFM tests."""
    def test_join_leave(self):
        cfm_test = CfmTest(
            scenario=Scenario(
                CreateMeeting(),
                LeaveMeeting()
            )
        )
        cfm_facade_mock = MagicMock(
                spec=cfm_facade_adapter.CFMFacadeRemoteAdapter)
        context = action_context.ActionContext(cfm_facade=cfm_facade_mock)
        runner = configurable_cfm_test.TestRunner(context)
        runner.run_test(cfm_test)
        cfm_facade_mock.start_meeting_session.assert_called_once_with()
        cfm_facade_mock.end_meeting_session.assert_called_once_with()

    def test_scenario_with_repeat(self):
        cfm_test = CfmTest(
            scenario=Scenario(
                CreateMeeting(),
                RepeatTimes(5, Scenario(
                    MuteMicrophone(),
                )),
            )
        )
        cfm_facade_mock = MagicMock(
                spec=cfm_facade_adapter.CFMFacadeRemoteAdapter)
        context = action_context.ActionContext(cfm_facade=cfm_facade_mock)
        runner = configurable_cfm_test.TestRunner(context)
        runner.run_test(cfm_test)
        cfm_facade_mock.start_meeting_session.assert_called_once_with()
        self.assertEqual(5, cfm_facade_mock.mute_mic.call_count)

