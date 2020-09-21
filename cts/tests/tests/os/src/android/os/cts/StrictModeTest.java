/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.os.cts;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.net.TrafficStats;
import android.net.Uri;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.StrictMode;
import android.os.StrictMode.ThreadPolicy.Builder;
import android.os.StrictMode.ViolationInfo;
import android.os.strictmode.CustomViolation;
import android.os.strictmode.FileUriExposedViolation;
import android.os.strictmode.UntaggedSocketViolation;
import android.os.strictmode.Violation;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.system.Os;
import android.system.OsConstants;
import android.util.Log;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.Socket;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

/** Tests for {@link StrictMode} */
@RunWith(AndroidJUnit4.class)
public class StrictModeTest {
    private static final String TAG = "StrictModeTest";
    private static final String REMOTE_SERVICE_ACTION = "android.app.REMOTESERVICE";

    private StrictMode.ThreadPolicy mThreadPolicy;
    private StrictMode.VmPolicy mVmPolicy;

    private Context getContext() {
        return InstrumentationRegistry.getContext();
    }

    @Before
    public void setUp() {
        mThreadPolicy = StrictMode.getThreadPolicy();
        mVmPolicy = StrictMode.getVmPolicy();
    }

    @After
    public void tearDown() {
        StrictMode.setThreadPolicy(mThreadPolicy);
        StrictMode.setVmPolicy(mVmPolicy);
    }

    public interface ThrowingRunnable {
        void run() throws Exception;
    }

    @Test
    public void testThreadBuilder() throws Exception {
        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().detectDiskReads().penaltyLog().build();
        StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder(policy).build());

        final File test = File.createTempFile("foo", "bar");
        inspectViolation(
                test::exists,
                violation -> {
                    assertThat(violation.getViolationDetails()).isNull();
                    assertThat(violation.getStackTrace()).contains("DiskReadViolation");
                });
    }

    @Test
    public void testUnclosedCloseable() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectLeakedClosableObjects().build());

        inspectViolation(
                () -> leakCloseable("leaked.txt"),
                info -> {
                    assertThat(info.getViolationDetails())
                            .isEqualTo(
                                    "A resource was acquired at attached stack trace but never released. See java.io.Closeable for information on avoiding resource leaks.");
                    assertThat(info.getStackTrace())
                            .contains("Explicit termination method 'close' not called");
                    assertThat(info.getStackTrace()).contains("leakCloseable");
                    assertPolicy(info, StrictMode.DETECT_VM_CLOSABLE_LEAKS);
                });
    }

    private void leakCloseable(String fileName) throws InterruptedException {
        final CountDownLatch finalizedSignal = new CountDownLatch(1);
        try {
            new FileOutputStream(new File(getContext().getFilesDir(), fileName)) {
                @Override
                protected void finalize() throws IOException {
                    super.finalize();
                    finalizedSignal.countDown();
                }
            };
        } catch (FileNotFoundException e) {
            throw new RuntimeException(e);
        }
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        // Sometimes it needs extra prodding.
        if (!finalizedSignal.await(5, TimeUnit.SECONDS)) {
            Runtime.getRuntime().gc();
            Runtime.getRuntime().runFinalization();
        }
    }

    @Test
    public void testClassInstanceLimit() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .setClassInstanceLimit(LimitedClass.class, 1)
                        .build());
        List<LimitedClass> references = new ArrayList<>();
        assertNoViolation(() -> references.add(new LimitedClass()));
        references.add(new LimitedClass());
        inspectViolation(
                StrictMode::conditionallyCheckInstanceCounts,
                info -> assertPolicy(info, StrictMode.DETECT_VM_INSTANCE_LEAKS));
    }

    private static final class LimitedClass {}

    /** Insecure connection should be detected */
    @Test
    public void testCleartextNetwork() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testCleartextNetwork() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectCleartextNetwork().penaltyLog().build());

        inspectViolation(
                () ->
                        ((HttpURLConnection) new URL("http://example.com/").openConnection())
                                .getResponseCode(),
                info -> {
                    assertThat(info.getViolationDetails())
                            .contains("Detected cleartext network traffic from UID");
                    assertThat(info.getViolationDetails())
                            .startsWith(StrictMode.CLEARTEXT_DETECTED_MSG);
                    assertPolicy(info, StrictMode.DETECT_VM_CLEARTEXT_NETWORK);
                });
    }

    /** Secure connection should be ignored */
    @Test
    public void testEncryptedNetwork() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testEncryptedNetwork() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectCleartextNetwork().penaltyLog().build());

        assertNoViolation(
                () ->
                        ((HttpURLConnection) new URL("https://example.com/").openConnection())
                                .getResponseCode());
    }

    @Test
    public void testFileUriExposure() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectFileUriExposure().penaltyLog().build());

        final Uri badUri = Uri.fromFile(new File("/sdcard/meow.jpg"));
        inspectViolation(
                () -> {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setDataAndType(badUri, "image/jpeg");
                    getContext().startActivity(intent);
                },
                violation -> {
                    assertThat(violation.getStackTrace()).contains(badUri + " exposed beyond app");
                });

        final Uri goodUri = Uri.parse("content://com.example/foobar");
        assertNoViolation(
                () -> {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setDataAndType(goodUri, "image/jpeg");
                    getContext().startActivity(intent);
                });
    }

    @Test
    public void testContentUriWithoutPermission() throws Exception {
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder()
                        .detectContentUriWithoutPermission()
                        .penaltyLog()
                        .build());

        final Uri uri = Uri.parse("content://com.example/foobar");
        inspectViolation(
                () -> {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setDataAndType(uri, "image/jpeg");
                    getContext().startActivity(intent);
                },
                violation ->
                        assertThat(violation.getStackTrace())
                                .contains(uri + " exposed beyond app"));

        assertNoViolation(
                () -> {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setDataAndType(uri, "image/jpeg");
                    intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    getContext().startActivity(intent);
                });
    }

    @Test
    public void testUntaggedSocketsHttp() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testUntaggedSockets() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectUntaggedSockets().penaltyLog().build());

        inspectViolation(
                () ->
                        ((HttpURLConnection) new URL("http://example.com/").openConnection())
                                .getResponseCode(),
                violation ->
                        assertThat(violation.getStackTrace())
                                .contains(UntaggedSocketViolation.MESSAGE));

        assertNoViolation(
                () -> {
                    TrafficStats.setThreadStatsTag(0xDECAFBAD);
                    try {
                        ((HttpURLConnection) new URL("http://example.com/").openConnection())
                                .getResponseCode();
                    } finally {
                        TrafficStats.clearThreadStatsTag();
                    }
                });
    }

    @Test
    public void testUntaggedSocketsRaw() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testUntaggedSockets() ignored on device without Internet");
            return;
        }

        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectUntaggedSockets().penaltyLog().build());

        assertNoViolation(
                () -> {
                    TrafficStats.setThreadStatsTag(0xDECAFBAD);
                    try (Socket socket = new Socket("example.com", 80)) {
                        socket.getOutputStream().close();
                    } finally {
                        TrafficStats.clearThreadStatsTag();
                    }
                });

        inspectViolation(
                () -> {
                    try (Socket socket = new Socket("example.com", 80)) {
                        socket.getOutputStream().close();
                    }
                },
                violation ->
                        assertThat(violation.getStackTrace())
                                .contains(UntaggedSocketViolation.MESSAGE));
    }

    private static final int PERMISSION_USER_ONLY = 0600;

    @Test
    public void testRead() throws Exception {
        final File test = File.createTempFile("foo", "bar");
        final File dir = test.getParentFile();

        final FileInputStream is = new FileInputStream(test);
        final FileDescriptor fd =
                Os.open(test.getAbsolutePath(), OsConstants.O_RDONLY, PERMISSION_USER_ONLY);

        StrictMode.setThreadPolicy(
                new StrictMode.ThreadPolicy.Builder().detectDiskReads().penaltyLog().build());
        inspectViolation(
                test::exists,
                violation -> {
                    assertThat(violation.getViolationDetails()).isNull();
                    assertThat(violation.getStackTrace()).contains("DiskReadViolation");
                });

        Consumer<ViolationInfo> assertDiskReadPolicy =
                violation -> assertPolicy(violation, StrictMode.DETECT_DISK_READ);
        inspectViolation(test::exists, assertDiskReadPolicy);
        inspectViolation(test::length, assertDiskReadPolicy);
        inspectViolation(dir::list, assertDiskReadPolicy);
        inspectViolation(is::read, assertDiskReadPolicy);

        inspectViolation(() -> new FileInputStream(test), assertDiskReadPolicy);
        inspectViolation(
                () -> Os.open(test.getAbsolutePath(), OsConstants.O_RDONLY, PERMISSION_USER_ONLY),
                assertDiskReadPolicy);
        inspectViolation(() -> Os.read(fd, new byte[10], 0, 1), assertDiskReadPolicy);
    }

    @Test
    public void testWrite() throws Exception {
        File file = File.createTempFile("foo", "bar");

        final FileOutputStream os = new FileOutputStream(file);
        final FileDescriptor fd =
                Os.open(file.getAbsolutePath(), OsConstants.O_RDWR, PERMISSION_USER_ONLY);

        StrictMode.setThreadPolicy(
                new StrictMode.ThreadPolicy.Builder().detectDiskWrites().penaltyLog().build());

        inspectViolation(
                file::createNewFile,
                violation -> {
                    assertThat(violation.getViolationDetails()).isNull();
                    assertThat(violation.getStackTrace()).contains("DiskWriteViolation");
                });

        Consumer<ViolationInfo> assertDiskWritePolicy =
                violation -> assertPolicy(violation, StrictMode.DETECT_DISK_WRITE);

        inspectViolation(() -> File.createTempFile("foo", "bar"), assertDiskWritePolicy);
        inspectViolation(() -> new FileOutputStream(file), assertDiskWritePolicy);
        inspectViolation(file::delete, assertDiskWritePolicy);
        inspectViolation(file::createNewFile, assertDiskWritePolicy);
        inspectViolation(() -> os.write(32), assertDiskWritePolicy);

        inspectViolation(
                () -> Os.open(file.getAbsolutePath(), OsConstants.O_RDWR, PERMISSION_USER_ONLY),
                assertDiskWritePolicy);
        inspectViolation(() -> Os.write(fd, new byte[10], 0, 1), assertDiskWritePolicy);
        inspectViolation(() -> Os.fsync(fd), assertDiskWritePolicy);
        inspectViolation(
                () -> file.renameTo(new File(file.getParent(), "foobar")), assertDiskWritePolicy);
    }

    @Test
    public void testNetwork() throws Exception {
        if (!hasInternetConnection()) {
            Log.i(TAG, "testUntaggedSockets() ignored on device without Internet");
            return;
        }

        StrictMode.setThreadPolicy(
                new StrictMode.ThreadPolicy.Builder().detectNetwork().penaltyLog().build());

        inspectViolation(
                () -> {
                    try (Socket socket = new Socket("example.com", 80)) {
                        socket.getOutputStream().close();
                    }
                },
                violation -> assertPolicy(violation, StrictMode.DETECT_NETWORK));
        inspectViolation(
                () ->
                        ((HttpURLConnection) new URL("http://example.com/").openConnection())
                                .getResponseCode(),
                violation -> assertPolicy(violation, StrictMode.DETECT_NETWORK));
    }

    @Test
    public void testViolationAcrossBinder() throws Exception {
        runWithRemoteServiceBound(
                getContext(),
                service -> {
                    StrictMode.setThreadPolicy(
                            new Builder().detectDiskWrites().penaltyLog().build());

                    try {
                        inspectViolation(
                                () -> service.performDiskWrite(),
                                (violation) -> {
                                    assertPolicy(violation, StrictMode.DETECT_DISK_WRITE);
                                    assertThat(violation.getViolationDetails())
                                            .isNull(); // Disk write has no message.
                                    assertThat(violation.getStackTrace())
                                            .contains("DiskWriteViolation");
                                    assertThat(violation.getStackTrace())
                                            .contains(
                                                    "at android.os.StrictMode$AndroidBlockGuardPolicy.onWriteToDisk");
                                    assertThat(violation.getStackTrace())
                                            .contains("# via Binder call with stack:");
                                    assertThat(violation.getStackTrace())
                                            .contains(
                                                    "at android.os.cts.ISecondary$Stub$Proxy.performDiskWrite");
                                });
                        assertNoViolation(() -> service.getPid());
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                });
    }

    private void checkNonSdkApiUsageViolation(boolean blacklist, String className,
            String methodName, Class<?>... paramTypes) throws Exception {
        Class<?> clazz = Class.forName(className);
        inspectViolation(
            () -> {
                try {
                    java.lang.reflect.Method m = clazz.getDeclaredMethod(methodName, paramTypes);
                    if (blacklist) {
                        fail();
                    }
                } catch (NoSuchMethodException expected) {
                  if (!blacklist) {
                    fail();
                  }
                }
            },
            violation -> {
                assertThat(violation).isNotNull();
                assertPolicy(violation, StrictMode.DETECT_VM_NON_SDK_API_USAGE);
                assertThat(violation.getViolationDetails()).contains(methodName);
                assertThat(violation.getStackTrace()).contains("checkNonSdkApiUsageViolation");
            }
        );
    }

    @Test
    public void testNonSdkApiUsage() throws Exception {
        StrictMode.VmPolicy oldVmPolicy = StrictMode.getVmPolicy();
        StrictMode.ThreadPolicy oldThreadPolicy = StrictMode.getThreadPolicy();
        try {
            StrictMode.setVmPolicy(
                    new StrictMode.VmPolicy.Builder().detectNonSdkApiUsage().build());
            checkNonSdkApiUsageViolation(
                true, "dalvik.system.VMRuntime", "setHiddenApiExemptions", String[].class);
            // verify that mutliple uses of a light greylist API are detected.
            checkNonSdkApiUsageViolation(false, "dalvik.system.VMRuntime", "getRuntime");
            checkNonSdkApiUsageViolation(false, "dalvik.system.VMRuntime", "getRuntime");

            // Verify that the VM policy is turned off after a call to permitNonSdkApiUsage.
            StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().permitNonSdkApiUsage().build());
            assertNoViolation(() -> {
                  Class<?> clazz = Class.forName("dalvik.system.VMRuntime");
                  try {
                      clazz.getDeclaredMethod("getRuntime");
                  } catch (NoSuchMethodException maybe) {
                  }
            });
        } finally {
            StrictMode.setVmPolicy(oldVmPolicy);
            StrictMode.setThreadPolicy(oldThreadPolicy);
        }
    }

    @Test
    public void testThreadPenaltyListener() throws Exception {
        final BlockingQueue<Violation> violations = new ArrayBlockingQueue<>(1);
        StrictMode.setThreadPolicy(
                new StrictMode.ThreadPolicy.Builder().detectCustomSlowCalls()
                        .penaltyListener(getContext().getMainExecutor(), (v) -> {
                            violations.add(v);
                        }).build());

        StrictMode.noteSlowCall("foo");

        final Violation v = violations.poll(5, TimeUnit.SECONDS);
        assertTrue(v instanceof CustomViolation);
    }

    @Test
    public void testVmPenaltyListener() throws Exception {
        final BlockingQueue<Violation> violations = new ArrayBlockingQueue<>(1);
        StrictMode.setVmPolicy(
                new StrictMode.VmPolicy.Builder().detectFileUriExposure()
                        .penaltyListener(getContext().getMainExecutor(), (v) -> {
                            violations.add(v);
                        }).build());

        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setDataAndType(Uri.fromFile(new File("/sdcard/meow.jpg")), "image/jpeg");
        getContext().startActivity(intent);

        final Violation v = violations.poll(5, TimeUnit.SECONDS);
        assertTrue(v instanceof FileUriExposedViolation);
    }

    private static void runWithRemoteServiceBound(Context context, Consumer<ISecondary> consumer)
            throws ExecutionException, InterruptedException, RemoteException {
        BlockingQueue<IBinder> binderHolder = new ArrayBlockingQueue<>(1);
        ServiceConnection secondaryConnection =
                new ServiceConnection() {
                    public void onServiceConnected(ComponentName className, IBinder service) {
                        binderHolder.add(service);
                    }

                    public void onServiceDisconnected(ComponentName className) {
                        binderHolder.drainTo(new ArrayList<>());
                    }
                };
        Intent intent = new Intent(REMOTE_SERVICE_ACTION);
        intent.setPackage(context.getPackageName());

        Intent secondaryIntent = new Intent(ISecondary.class.getName());
        secondaryIntent.setPackage(context.getPackageName());
        assertThat(
                        context.bindService(
                                secondaryIntent, secondaryConnection, Context.BIND_AUTO_CREATE))
                .isTrue();
        IBinder binder = binderHolder.take();
        assertThat(binder.queryLocalInterface(binder.getInterfaceDescriptor())).isNull();
        consumer.accept(ISecondary.Stub.asInterface(binder));
        context.unbindService(secondaryConnection);
        context.stopService(intent);
    }

    private static void assertViolation(String expected, ThrowingRunnable r) throws Exception {
        inspectViolation(r, violation -> assertThat(violation.getStackTrace()).contains(expected));
    }

    private static void assertNoViolation(ThrowingRunnable r) throws Exception {
        inspectViolation(
                r, violation -> assertWithMessage("Unexpected violation").that(violation).isNull());
    }

    private void assertPolicy(ViolationInfo info, int policy) {
        assertWithMessage("Policy bit incorrect").that(info.getViolationBit()).isEqualTo(policy);
    }

    private static void inspectViolation(
            ThrowingRunnable violating, Consumer<ViolationInfo> consume) throws Exception {
        final LinkedBlockingQueue<ViolationInfo> violations = new LinkedBlockingQueue<>();
        StrictMode.setViolationLogger(violations::add);

        try {
            violating.run();
            consume.accept(violations.poll(5, TimeUnit.SECONDS));
        } finally {
            StrictMode.setViolationLogger(null);
        }
    }

    private boolean hasInternetConnection() {
        final PackageManager pm = getContext().getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)
                || pm.hasSystemFeature(PackageManager.FEATURE_WIFI)
                || pm.hasSystemFeature(PackageManager.FEATURE_ETHERNET);
    }
}
