# Configurable CfM Tests

The Configurable CfM Tests modules allow creating test scenarios using an API
that clearly describes the steps executed. A sample of a configurable CfM test
is:

    CfmTest(
        scenario=Scenario(
            CreateMeeting(),
            RepeatTimes(5, Scenario(
                MuteMicrophone(),
                UnmuteMicrophone()
            )),
            LeaveMeeting(),
            AssertUsbDevices(ATRUS),
            AssertFileDoesNotContain('/var/log/messages', ['FATAL ERROR'])
        ),
        configuration=Configuration(
            run_test_only = False
        )
    )

This test creates a meeting, mutes and unmutes the microphone five times and
leaves the meeting. It then verifies that an ATRUS device is visible and that
the log file `/var/log/messages` does not contains `FATAL ERROR`.

A configurable test can be setup in a `control` file so that third parties
that have no easy way to modify other source code can create and modify
such tests.

For the test to be executed properly it has to subclass [`autotest_lib.server.cros.cfm.configurable_test.configurable_cfm_tests.ConfigurableCfmTest`](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/master/server/cros/cfm/configurable_test/configurable_cfm_test.py).

## Actions

Each step in a scenario is an Action. The available actions are listed in
[`autotest_lib.server.cros.cfm.configurable_test.actions`](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/master/server/cros/cfm/configurable_test/actions.py).

## Configuration

Besides Actions, a test can be configured with configuration params that affect
behavior outside of the actions. The available configuration flags are
documented in
[`autotest_lib.server.cros.cfm.configurable_test.configuration`](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/master/server/cros/cfm/configurable_test/configuration.py).

## Samples

For complete samples see the Autotest
[`enterprise_CFM_ConfigurableCfmTestSanity`](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/master/server/site_tests/enterprise_CFM_ConfigurableCfmTestSanity/)
that we use to test the framework itself.


