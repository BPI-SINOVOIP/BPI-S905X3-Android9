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
 * limitations under the License.
 */
package android.device.collectors;

import android.app.UiAutomation;
import android.device.collectors.annotations.OptionClass;
import android.os.Build;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import org.junit.runner.Description;
import org.junit.runner.Result;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

/**
 * A {@link BaseMetricListener} that captures BatteryStats for the entire test class in proto format
 * and record it to a file.
 *
 * This class needs external storage permission. See {@link BaseMetricListener} how to grant
 * external storage permission, especially at install time.
 *
 * Options:
 * -e batterystats-format [byte|file:optional/path/to/dir] : set storage format. Default is file.
 * Choose "byte" to save as byte array in result bundle.
 * Choose "file" to save as proto files. Append ":path/to/dir" behind "file" to specify directory
 * to save the files, relative to /sdcard/. e.g. "-e batterystats-format file:tmp/bs" will save
 * batterystats protobuf to /sdcard/tmp/bs/ directory.
 *
 * Do NOT throw exception anywhere in this class. We don't want to halt the test when metrics
 * collection fails.
 */
@OptionClass(alias = "battery-stats-collector")
public class BatteryStatsListener extends BaseMetricListener {
    private static final String MSG_DUMPSYS_RESET_SUCCESS = "Battery stats reset.";
    public  static final String CMD_DUMPSYS = "dumpsys batterystats --proto";
    private static final String CMD_DUMPSYS_RESET = "dumpsys batterystats --reset";
    public static final String OPTION_BYTE = "byte";
    static final String DEFAULT_DIR = "run_listeners/battery_stats";
    static final String KEY_PER_RUN = "batterystats-per-run";
    static final String KEY_FORMAT = "batterystats-format";

    private File mDestDir;
    private String mTestClassName;
    private boolean mPerRun;
    private boolean mBatteryStatReset;
    private boolean mToFile;

    public BatteryStatsListener() {
        super();
    }

    /**
     * Constructor to simulate receiving the instrumentation arguments. Should not be used except
     * for testing.
     */
    @VisibleForTesting
    BatteryStatsListener(Bundle argsBundle) {
        super(argsBundle);
    }

    @Override
    public void onTestRunStart(DataRecord runData, Description description) {
        if(Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            Log.w(getTag(), "dumpsys batterystats requires API version >= 21");
            return;
        }
        Bundle args = getArgsBundle();
        mPerRun = "true".equals(args.getString(KEY_PER_RUN));
        mToFile = !OPTION_BYTE.equals(args.getString(KEY_FORMAT));
        if (mToFile) {
            String dir = DEFAULT_DIR;
            if (args.containsKey(KEY_FORMAT)) {
                String[] argsArray = args.getString(KEY_FORMAT).split(":", 2);
                if (argsArray.length > 1) dir = argsArray[1];
            }
            mDestDir = createAndEmptyDirectory(dir);
        }

        // set charging state as unplugged
        executeCommandBlocking("dumpsys battery unplug");
        if (mPerRun) {
            mBatteryStatReset = resetBatteryStats();
        }
    }

    @Override
    public void onTestStart(DataRecord testData, Description description) {
        if (mToFile && mDestDir == null) {
            return;
        }
        // a workaround to get test class name. description provided to onTestRunStart is null.
        if (mTestClassName == null) {
            mTestClassName = description.getClassName();
        }
        if (mPerRun) {
            return;
        }
        mBatteryStatReset = resetBatteryStats();
    }

    @Override
    public void onTestEnd(DataRecord testData, Description description) {
        if ((mToFile && mDestDir == null) || mPerRun || !mBatteryStatReset) {
            return;
        }
        mBatteryStatReset = false;
        if (mToFile) {
            String fileName = String.format("%s.%s.batterystatsproto", description.getClassName(),
                    description.getMethodName());
            File logFile = dumpBatteryStats(fileName);
            if (logFile != null) {
                testData.addFileMetric(String.format("%s_%s", getTag(), logFile.getName()), logFile);
            }
        } else {
            String key = String.format("%s_%s.%s.bytes", getTag(), description.getClassName(),
                    description.getMethodName());
            byte[] proto = executeCommandBlocking(CMD_DUMPSYS);
            if (proto != null) {
                testData.addBinaryMetric(key, proto);
            }
        }
    }

    @Override
    public void onTestRunEnd(DataRecord runData, Result result) {
        if (mToFile && mDestDir == null) {
            return;
        }

        if (mPerRun && mBatteryStatReset) {
            mBatteryStatReset = false;
            if (mToFile) {
                File logFile = dumpBatteryStats(String.format("%s.batterystatsproto", mTestClassName));
                if (logFile != null) {
                    runData.addFileMetric(String.format("%s_%s", getTag(), logFile.getName()), logFile);
                }
            } else {
                String key = String.format("%s_%s.bytes", getTag(), mTestClassName);
                byte[] proto = executeCommandBlocking(CMD_DUMPSYS);
                if (proto != null) {
                    runData.addBinaryMetric(key, proto);
                }
            }
        }
        // reset charging state
        executeCommandBlocking("dumpsys battery reset");
    }

    /**
     * Call "dumpsys batterystats --proto" to dump batterystats to a proto file.
     * Public so that Mockito can alter its behavior.
     *
     * @param fileName the name of the proto file
     * @return the proto file containing the dumpsys batterystats
     */
    @VisibleForTesting
    public File dumpBatteryStats(String fileName) {
        UiAutomation automation = getInstrumentation().getUiAutomation();
        File logFile = new File(mDestDir, fileName);
        try (
                InputStream is = new ParcelFileDescriptor.AutoCloseInputStream(
                        automation.executeShellCommand(CMD_DUMPSYS));
                OutputStream out = new BufferedOutputStream(new FileOutputStream(logFile))
        ){
            byte[] buf = new byte[BUFFER_SIZE];
            int bytes;
            while((bytes = is.read(buf)) != -1) {
                out.write(buf, 0, bytes);
            }
            return logFile;
        } catch (Exception e) {
            Log.e(getTag(), "Unable to dump batterystats", e);
            return null;
        }
    }

    /**
     * Call "dumpsys batterystats --reset" to reset batterystats.
     * Public so that Mockito can alter its behavior.
     *
     * @return true only if reset successfully without error
     */
    @VisibleForTesting
    public boolean resetBatteryStats() {
        UiAutomation automation = getInstrumentation().getUiAutomation();
        try (
                InputStream is = new ParcelFileDescriptor.AutoCloseInputStream(
                        automation.executeShellCommand(CMD_DUMPSYS_RESET));
                BufferedReader reader = new BufferedReader(new InputStreamReader(is))
        ){
            String line;
            while(null != (line = reader.readLine())) {
                if(line.contains(MSG_DUMPSYS_RESET_SUCCESS)) {
                    return true;
                }
            }
            Log.e(getTag(), "Unable to reset batterystats");
            return false;
        } catch (IOException ex) {
            Log.e(getTag(), "Unable to reset batterystats", ex);
            return false;
        }
    }
}
