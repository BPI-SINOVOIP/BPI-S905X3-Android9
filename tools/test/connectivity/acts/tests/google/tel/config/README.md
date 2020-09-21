## Telephony Testing Config Files
Telephony config files have some differences from other ACTS configs that require additional keys to be defined in order to run nearly all tests. Below are details of many such keys:
#### Generally-applicable test keys:
  - **(\*experimental) enable_wifi_verbose_logging** - This key, if defined (the value is unused), will enable the 'WiFi Verbose Logging' developer option on connected AndroidDevices.
  - **no_bug_report_on_fail** - The default behavior of telephony tests is to capture a bug report after each failed test. In situations where this is overly time consuming or otherwise not desirable, defining this key (the value is unused) will bypass the process of collecting a bug report.
  - **sim_conf_file** - Path to a SIM card config file, which is needed in the event that the MSISDN/MDN is not programmed onto SIM cards used in a defined testbed. The file provides key/value pairs mapping ICCIDs to MSISDNs. This file can be generated and amended by the 'manage_sim.py' utility. Its contents are subject to change so users are highly encouraged to use manage_sim.py.
  - **stress_test_number** - This controls the number of iterations run by "stress" tests. (Tests that require this always have the word "stress" in the test name).
  - **telephony_auto_rerun** - Because testing with live infrastructure sometimes yields flaky results, when no other options are available to mitigate this uncertainty, this key specifies a maximum number of re-runs that will be performed in the event of a test failure. The test will be reported as a 'pass' after the first successful run.
  - **wifi_network_pass** - The password to the network specified by *wifi_network_ssid*.
  - **wifi_network_ssid** - The SSID of a wifi network for test use. This network must have internet access.
#### Power Test specific keys (TelPowerTest):
  - **pass_criteria_call_(3g/volte/2g/wfc)** - The maximum amount of power in mW that can be used in steady state during calling power tests in order to pass the test.
  - **pass_criteria_idle_(3g/volte/2g/wfc)** - The maximum amount of power in mW that can be used in steady state during idle power tests in order to pass the test.
#### Call-Server test specific keys (TelLiveStressCallTest):
  - **phone_call_iteration** - The number of calls to be placed in TelLiveStressCallTest
  - **call_server_number** - the POTS telephone number of a call server used in TelLiveStressCallTest

