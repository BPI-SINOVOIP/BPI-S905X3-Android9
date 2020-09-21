package com.android.tradefed.targetprep;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.TestDeviceState;
import com.android.tradefed.testtype.DeviceTestCase;

public class DeviceWiperFuncTest extends DeviceTestCase {

    public void testWipe() throws DeviceNotAvailableException, TargetSetupError {
        getDevice().enableAdbRoot();
        getDevice().executeShellCommand("rm /data/local/tmp/foo.txt");
        getDevice().pushString("blargh", "/data/local/tmp/foo.txt");
        assertTrue(getDevice().doesFileExist("/data/local/tmp/foo.txt"));
        new DeviceWiper().setUp(getDevice(), null);
        assertTrue(getDevice().getDeviceState().equals(TestDeviceState.ONLINE));
        assertFalse(getDevice().doesFileExist("/data/local/tmp/foo.txt"));

    }
}
