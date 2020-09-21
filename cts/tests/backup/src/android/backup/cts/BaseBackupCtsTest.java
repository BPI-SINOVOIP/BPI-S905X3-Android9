/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * limitations under the License
 */

package android.backup.cts;

import android.app.Instrumentation;
import android.content.pm.PackageManager;
import android.os.ParcelFileDescriptor;
import android.test.InstrumentationTestCase;

import com.android.compatibility.common.util.LogcatInspector;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

/**
 * Base class for backup instrumentation tests.
 *
 * Ensures that backup is enabled and local transport selected, and provides some utility methods.
 */
public class BaseBackupCtsTest extends InstrumentationTestCase {
    private static final String APP_LOG_TAG = "BackupCTSApp";

    private static final String LOCAL_TRANSPORT =
            "android/com.android.internal.backup.LocalTransport";

    private boolean isBackupSupported;
    private LogcatInspector mLogcatInspector =
            new LogcatInspector() {
                @Override
                protected InputStream executeShellCommand(String command) throws IOException {
                    return executeStreamedShellCommand(getInstrumentation(), command);
                }
            };

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        PackageManager packageManager = getInstrumentation().getContext().getPackageManager();
        isBackupSupported = packageManager != null
                && packageManager.hasSystemFeature(PackageManager.FEATURE_BACKUP);

        if (isBackupSupported) {
            assertTrue("Backup not enabled", isBackupEnabled());
            assertTrue("LocalTransport not selected", isLocalTransportSelected());
            exec("setprop log.tag." + APP_LOG_TAG +" VERBOSE");
        }
    }

    public boolean isBackupSupported() {
        return isBackupSupported;
    }

    private boolean isBackupEnabled() throws Exception {
        String output = exec("bmgr enabled");
        return output.contains("currently enabled");
    }

    private boolean isLocalTransportSelected() throws Exception {
        String output = exec("bmgr list transports");
        return output.contains("* " + LOCAL_TRANSPORT);
    }

    /** See {@link LogcatInspector#mark(String)}. */
    protected String markLogcat() throws Exception {
        return mLogcatInspector.mark(APP_LOG_TAG);
    }

    /** See {@link LogcatInspector#assertLogcatContainsInOrder(String, int, String...)}. */
    protected void waitForLogcat(int maxTimeoutInSeconds, String... logcatStrings)
            throws Exception {
        mLogcatInspector.assertLogcatContainsInOrder(
                APP_LOG_TAG + ":* *:S", maxTimeoutInSeconds, logcatStrings);
    }

    protected void createTestFileOfSize(String packageName, int size) throws Exception {
        exec("am start -a android.intent.action.MAIN " +
            "-c android.intent.category.LAUNCHER " +
            "-n " + packageName + "/android.backup.app.MainActivity " +
            "-e file_size " + size);
        waitForLogcat(30, "File created!");
    }

    protected String exec(String command) throws Exception {
        try (InputStream in = executeStreamedShellCommand(getInstrumentation(), command)) {
            BufferedReader br = new BufferedReader(
                    new InputStreamReader(in, StandardCharsets.UTF_8));
            String str;
            StringBuilder out = new StringBuilder();
            while ((str = br.readLine()) != null) {
                out.append(str);
            }
            return out.toString();
        }
    }

    private static FileInputStream executeStreamedShellCommand(
            Instrumentation instrumentation, String command) throws IOException {
        final ParcelFileDescriptor pfd =
                instrumentation.getUiAutomation().executeShellCommand(command);
        return new ParcelFileDescriptor.AutoCloseInputStream(pfd);
    }

    private static void drainAndClose(BufferedReader reader) {
        try {
            while (reader.read() >= 0) {
                // do nothing.
            }
        } catch (IOException ignored) {
        }
        closeQuietly(reader);
    }

    private static void closeQuietly(AutoCloseable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (RuntimeException rethrown) {
                throw rethrown;
            } catch (Exception ignored) {
            }
        }
    }
}
