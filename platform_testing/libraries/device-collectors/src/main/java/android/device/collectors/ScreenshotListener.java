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

import android.device.collectors.annotations.OptionClass;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import org.junit.runner.Description;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;

/**
 * A {@link BaseMetricListener} that captures screenshots at the end of each test case.
 *
 * This class needs external storage permission. See {@link BaseMetricListener} how to grant
 * external storage permission, especially at install time.
 *
 * Options:
 * -e screenshot-quality [0-100]: set screenshot image quality. Default is 75.
 * -e screenshot-format [byte|file:optional/path/to/dir]: set screenshot format. Default is file.
 * Choose "byte" to save as a byte array in result bundle.
 * Choose "file" to save as an image file. Append ":path/to/dir" behind "file" to specify directory
 * to save screenshots, relative to /sdcard/. e.g. "-e screenshot-format file:tmp/sc" will save
 * screenshots to /sdcard/tmp/sc/ directory.
 *
 * Do NOT throw exception anywhere in this class. We don't want to halt the test when metrics
 * collection fails.
 */
@OptionClass(alias = "screenshot-collector")
public class ScreenshotListener extends BaseMetricListener {

    public static final String DEFAULT_DIR = "run_listeners/screenshots";
    public static final String KEY_QUALITY = "screenshot-quality"; // 0 to 100
    public static final String KEY_FORMAT = "screenshot-format"; // byte or file
    public static final int DEFAULT_QUALITY = 75;
    public static final String OPTION_BYTE = "byte";
    private int mQuality = DEFAULT_QUALITY;
    private boolean mToFile;

    private File mDestDir;

    public ScreenshotListener() {
        super();
    }

    /**
     * Constructor to simulate receiving the instrumentation arguments. Should not be used except
     * for testing.
     */
    @VisibleForTesting
    ScreenshotListener(Bundle args) {
        super(args);
    }

    @Override
    public void onTestRunStart(DataRecord runData, Description description) {
        Bundle args = getArgsBundle();
        if (args.containsKey(KEY_QUALITY)) {
            try {
                int quality = Integer.parseInt(args.getString(KEY_QUALITY));
                if (quality >= 0 && quality <= 100) {
                    mQuality = quality;
                } else {
                    Log.e(getTag(), String.format("Invalid screenshot quality: %d.", quality));
                }
            } catch (Exception e) {
                Log.e(getTag(), "Failed to parse screenshot quality", e);
            }
        }
        mToFile = !OPTION_BYTE.equals(args.getString(KEY_FORMAT));
        if (mToFile) {
            String dir = DEFAULT_DIR;
            if (args.containsKey(KEY_FORMAT)) {
                String[] argsArray = args.getString(KEY_FORMAT).split(":", 2);
                if (argsArray.length > 1) {
                    dir = argsArray[1];
                }
            }
            mDestDir = createAndEmptyDirectory(dir);
        }
    }

    @Override
    public void onTestEnd(DataRecord testData, Description description) {
        if (mToFile) {
            if (mDestDir == null) {
                return;
            }
            final String fileName = String.format("%s.%s.png", description.getClassName(),
                    description.getMethodName());
            File img = takeScreenshot(fileName);
            if (img != null) {
                testData.addFileMetric(String.format("%s_%s", getTag(), img.getName()), img);
            }
        } else {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            screenshotToStream(out);
            testData.addBinaryMetric(String.format("%s_%s.%s.bytes", getTag(),
                    description.getClassName(), description.getMethodName()), out.toByteArray());
        }
    }

    /**
     * Public so that Mockito can alter its behavior.
     */
    @VisibleForTesting
    public File takeScreenshot(String fileName){
        File img = new File(mDestDir, fileName);
        if (img.exists()) {
            Log.w(getTag(), String.format("File exists: %s", img.getAbsolutePath()));
            img.delete();
        }
        try (
                OutputStream out = new BufferedOutputStream(new FileOutputStream(img))
        ){
            screenshotToStream(out);
            out.flush();
            return img;
        } catch (Exception e) {
            Log.e(getTag(), "Unable to save screenshot", e);
            img.delete();
            return null;
        }
    }

    /**
     * Public so that Mockito can alter its behavior.
     */
    @VisibleForTesting
    public void screenshotToStream(OutputStream out) {
        getInstrumentation().getUiAutomation()
                .takeScreenshot().compress(Bitmap.CompressFormat.PNG, mQuality, out);
    }
}
