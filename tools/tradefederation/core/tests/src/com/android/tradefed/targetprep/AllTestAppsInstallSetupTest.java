package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.File;

/**
 * Unit tests for {@link AllTestAppsInstallSetup}
 */
public class AllTestAppsInstallSetupTest extends TestCase {

    private static final String SERIAL = "SERIAL";
    private AllTestAppsInstallSetup mPrep;
    private IDeviceBuildInfo mMockBuildInfo;
    private ITestDevice mMockTestDevice;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp() throws Exception {
        super.setUp();
        mPrep = new AllTestAppsInstallSetup();
        mMockBuildInfo = EasyMock.createMock(IDeviceBuildInfo.class);
        mMockTestDevice = EasyMock.createMock(ITestDevice.class);
        EasyMock.expect(mMockTestDevice.getSerialNumber()).andStubReturn(SERIAL);
        EasyMock.expect(mMockTestDevice.getDeviceDescriptor()).andStubReturn(null);
    }

    public void testNotIDeviceBuildInfo() throws DeviceNotAvailableException {
        IBuildInfo mockBuildInfo = EasyMock.createMock(IBuildInfo.class);
        EasyMock.replay(mockBuildInfo, mMockTestDevice);
        try {
            mPrep.setUp(mMockTestDevice, mockBuildInfo);
            fail("Should have thrown a TargetSetupError");
        } catch (TargetSetupError e) {
            // expected
            assertEquals("Invalid buildInfo, expecting an IDeviceBuildInfo null",
                    e.getMessage());
        }
        EasyMock.verify(mockBuildInfo, mMockTestDevice);
    }

    public void testNoTestDir() throws DeviceNotAvailableException {
        EasyMock.expect(mMockBuildInfo.getTestsDir()).andStubReturn(new File(""));
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        try {
            mPrep.setUp(mMockTestDevice, mMockBuildInfo);
            fail("Should have thrown a TargetSetupError");
        } catch (TargetSetupError e) {
            assertEquals("Failed to find a valid test zip directory. null",
                    e.getMessage());
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    public void testNullTestDir() throws DeviceNotAvailableException {

        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        try {
            mPrep.installApksRecursively(null, mMockTestDevice);
            fail("Should have thrown a TargetSetupError");
        } catch (TargetSetupError e) {
            assertEquals("Invalid test zip directory! null", e.getMessage());
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }

    public void testSetup() throws Exception {
        File testDir = FileUtil.createTempDir("TestAppSetupTest");
        // fake hierarchy of directory and files
        FileUtil.createTempFile("fakeApk", ".apk", testDir);
        FileUtil.createTempFile("fakeApk2", ".apk", testDir);
        FileUtil.createTempFile("notAnApk", ".txt", testDir);
        File subTestDir = FileUtil.createTempDir("SubTestAppSetupTest", testDir);
        FileUtil.createTempFile("subfakeApk", ".apk", subTestDir);
        try {
            EasyMock.expect(mMockBuildInfo.getTestsDir()).andReturn(testDir);
            EasyMock.expect(mMockTestDevice.installPackage((File)EasyMock.anyObject(),
                    EasyMock.eq(true))).andReturn(null).times(3);
            EasyMock.replay(mMockBuildInfo, mMockTestDevice);
            mPrep.setUp(mMockTestDevice, mMockBuildInfo);
            EasyMock.verify(mMockBuildInfo, mMockTestDevice);
        } finally {
            FileUtil.recursiveDelete(testDir);
        }
    }

    public void testInstallFailure() throws DeviceNotAvailableException {
        final String failure = "INSTALL_PARSE_FAILED_MANIFEST_MALFORMED";
        final String file = "TEST";
        EasyMock.expect(mMockTestDevice.installPackage((File)EasyMock.anyObject(),
                EasyMock.eq(true))).andReturn(failure);
        EasyMock.replay(mMockBuildInfo, mMockTestDevice);
        try {
            mPrep.installApk(new File("TEST"), mMockTestDevice);
            fail("Should have thrown an exception");
        } catch (TargetSetupError e) {
            String expected = String.format("Failed to install %s on %s. Reason: '%s' "
                    + "null", file, SERIAL, failure);
            assertEquals(expected, e.getMessage());
        }
        EasyMock.verify(mMockBuildInfo, mMockTestDevice);
    }
}
