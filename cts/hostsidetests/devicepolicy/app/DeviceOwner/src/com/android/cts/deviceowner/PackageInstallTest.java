package com.android.cts.deviceowner;

import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentSender;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;

import com.android.compatibility.common.util.BlockingBroadcastReceiver;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Collections;

/**
 * Test case for package install and uninstall.
 */
public class PackageInstallTest extends BaseAffiliatedProfileOwnerTest {
    private static final String TEST_APP_LOCATION =
            "/data/local/tmp/cts/packageinstaller/CtsEmptyTestApp.apk";
    private static final String TEST_APP_PKG = "android.packageinstaller.emptytestapp.cts";
    private static final int REQUEST_CODE = 0;
    private static final String ACTION_INSTALL_COMMIT =
            "com.android.cts.deviceowner.INTENT_PACKAGE_INSTALL_COMMIT";
    private static final int PACKAGE_INSTALLER_STATUS_UNDEFINED = -1000;

    private PackageManager mPackageManager;
    private PackageInstaller mPackageInstaller;
    private PackageInstaller.Session mSession;
    private BlockingBroadcastReceiver mBroadcastReceiver;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mPackageManager = mContext.getPackageManager();
        mPackageInstaller = mPackageManager.getPackageInstaller();
        assertNotNull(mPackageInstaller);

        mBroadcastReceiver = new BlockingBroadcastReceiver(mContext, ACTION_INSTALL_COMMIT);
        mBroadcastReceiver.register();
    }

    @Override
    protected void tearDown() throws Exception {
        mBroadcastReceiver.unregisterQuietly();
        if (mSession != null) {
            mSession.abandon();
        }

        super.tearDown();
    }

    public void testPackageInstall() throws Exception {
        assertFalse(isPackageInstalled(TEST_APP_PKG));

        // Install the package.
        installPackage(TEST_APP_LOCATION);

        Intent intent = mBroadcastReceiver.awaitForBroadcast();
        assertNotNull(intent);
        assertEquals(PackageInstaller.STATUS_SUCCESS,
                intent.getIntExtra(PackageInstaller.EXTRA_STATUS,
                        PACKAGE_INSTALLER_STATUS_UNDEFINED));
        assertEquals(TEST_APP_PKG, intent.getStringExtra(
                PackageInstaller.EXTRA_PACKAGE_NAME));
        assertTrue(isPackageInstalled(TEST_APP_PKG));
    }

    public void testPackageUninstall() throws Exception {
        assertTrue(isPackageInstalled(TEST_APP_PKG));

        // Uninstall the package.
        mPackageInstaller.uninstall(TEST_APP_PKG, getCommitCallback());

        Intent intent = mBroadcastReceiver.awaitForBroadcast();
        assertNotNull(intent);
        assertEquals(PackageInstaller.STATUS_SUCCESS,
                intent.getIntExtra(PackageInstaller.EXTRA_STATUS,
                        PACKAGE_INSTALLER_STATUS_UNDEFINED));
        assertEquals(TEST_APP_PKG, intent.getStringExtra(
                PackageInstaller.EXTRA_PACKAGE_NAME));
        assertFalse(isPackageInstalled(TEST_APP_PKG));
    }

    public void testKeepPackageCache() throws Exception {
        // Set keep package cache.
        mDevicePolicyManager.setKeepUninstalledPackages(getWho(),
                Collections.singletonList(TEST_APP_PKG));
    }

    public void testInstallExistingPackage() throws Exception {
        assertFalse(isPackageInstalled(TEST_APP_PKG));

        // Install the existing package.
        assertTrue(mDevicePolicyManager.installExistingPackage(getWho(), TEST_APP_PKG));
        assertTrue(isPackageInstalled(TEST_APP_PKG));
    }

    private void installPackage(String packageLocation) throws Exception {
        PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                PackageInstaller.SessionParams.MODE_FULL_INSTALL);
        params.setAppPackageName(TEST_APP_PKG);
        int sessionId = mPackageInstaller.createSession(params);
        mSession = mPackageInstaller.openSession(sessionId);

        File file = new File(packageLocation);
        InputStream in = new FileInputStream(file);
        OutputStream out = mSession.openWrite("PackageInstallTest", 0, file.length());
        byte[] buffer = new byte[65536];
        int c;
        while ((c = in.read(buffer)) != -1) {
            out.write(buffer, 0, c);
        }
        mSession.fsync(out);
        out.close();
        mSession.commit(getCommitCallback());
        mSession.close();
    }

    private IntentSender getCommitCallback() {
        // Create a PendingIntent and use it to generate the IntentSender
        Intent broadcastIntent = new Intent(ACTION_INSTALL_COMMIT);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(
                mContext,
                REQUEST_CODE,
                broadcastIntent,
                PendingIntent.FLAG_UPDATE_CURRENT);
        return pendingIntent.getIntentSender();
    }

    private boolean isPackageInstalled(String packageName) {
        try {
            return mPackageManager.getPackageInfo(packageName, 0) != null;
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
    }
}
