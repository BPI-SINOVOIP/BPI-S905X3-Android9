// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webview_shell;

import android.os.Environment;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Tests running end-to-end layout tests.
 */
public class WebViewLayoutTest
        extends ActivityInstrumentationTestCase2<WebViewLayoutTestActivity> {

    private static final String TAG = "WebViewLayoutTest";

    private static final String EXTERNAL_PREFIX =
            Environment.getExternalStorageDirectory().getAbsolutePath() + "/";
    private static final String BASE_WEBVIEW_TEST_PATH = "android_webview/tools/WebViewShell/test/";
    private static final String BASE_BLINK_TEST_PATH = "third_party/WebKit/LayoutTests/";
    private static final String PATH_WEBVIEW_PREFIX = EXTERNAL_PREFIX + BASE_WEBVIEW_TEST_PATH;
    private static final String PATH_BLINK_PREFIX = EXTERNAL_PREFIX + BASE_BLINK_TEST_PATH;

    private static final long TIMEOUT_SECONDS = 20;

    private WebViewLayoutTestActivity mTestActivity;

    public WebViewLayoutTest() {
        super(WebViewLayoutTestActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTestActivity = (WebViewLayoutTestActivity) getActivity();
    }

    @Override
    protected void tearDown() throws Exception {
        mTestActivity.finish();
        super.tearDown();
    }

    @Override
    public WebViewLayoutTestRunner getInstrumentation() {
        return (WebViewLayoutTestRunner) super.getInstrumentation();
    }

    public void testSimple() throws Exception {
        runWebViewLayoutTest("experimental/basic-logging.html",
                             "experimental/basic-logging-expected.txt");
    }

    public void testGlobalInterface() throws Exception {
        runBlinkLayoutTest("webexposed/global-interface-listing.html",
                           "webexposed/global-interface-listing-expected.txt");
    }

    // test helper methods

    private void runWebViewLayoutTest(final String fileName, final String fileNameExpected)
            throws Exception {
        runTest(PATH_WEBVIEW_PREFIX + fileName, PATH_WEBVIEW_PREFIX + fileNameExpected);
    }

    private void runBlinkLayoutTest(final String fileName, final String fileNameExpected)
            throws Exception {
        ensureJsTestCopied();
        runTest(PATH_BLINK_PREFIX + fileName, PATH_WEBVIEW_PREFIX + fileNameExpected);
    }

    private void runTest(final String fileName, final String fileNameExpected)
            throws FileNotFoundException, IOException, InterruptedException, TimeoutException {
        loadUrlWebViewAsync("file://" + fileName, mTestActivity);

        if (getInstrumentation().isRebaseline()) {
            // this is the rebaseline process
            mTestActivity.waitForFinish(TIMEOUT_SECONDS, TimeUnit.SECONDS);
            String result = mTestActivity.getTestResult();
            writeFile(fileNameExpected, result, true);
            Log.i(TAG, "file: " + fileNameExpected + " --> rebaselined, length=" + result.length());
        } else {
            String expected = readFile(fileNameExpected);
            mTestActivity.waitForFinish(TIMEOUT_SECONDS, TimeUnit.SECONDS);
            String result = mTestActivity.getTestResult();
            assertEquals(expected, result);
        }
    }

    private void loadUrlWebViewAsync(final String fileUrl,
            final WebViewLayoutTestActivity activity) {
        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                activity.loadUrl(fileUrl);
            }
        });
    }

    private static void ensureJsTestCopied() throws IOException {
        File jsTestFile = new File(PATH_BLINK_PREFIX + "resources/js-test.js");
        if (jsTestFile.exists()) return;
        String original = readFile(PATH_WEBVIEW_PREFIX + "resources/js-test.js");
        writeFile(PATH_BLINK_PREFIX + "resources/js-test.js", original, false);
    }

    /**
     * Reads a file and returns it's contents as string.
     */
    private static String readFile(String fileName) throws IOException {
        FileInputStream inputStream = new FileInputStream(new File(fileName));
        try {
            BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
            try {
                StringBuilder contents = new StringBuilder();
                String line;

                while ((line = reader.readLine()) != null) {
                    contents.append(line);
                    contents.append("\n");
                }
                return contents.toString();
            } finally {
                reader.close();
            }
        } finally {
            inputStream.close();
        }
    }

    /**
     * Writes a file with the given fileName and contents. If overwrite is true overwrites any
     * exisiting file with the same file name. If the file does not exist any intermediate
     * required directories are created.
     */
    private static void writeFile(final String fileName, final String contents, boolean overwrite)
            throws FileNotFoundException, IOException {
        File fileOut = new File(fileName);

        if (fileOut.exists() && !overwrite) {
            return;
        }

        String absolutePath = fileOut.getAbsolutePath();
        File filePath = new File(absolutePath.substring(0, absolutePath.lastIndexOf("/")));

        if (!filePath.exists()) {
            if (!filePath.mkdirs())
                throw new IOException("failed to create directories: " + filePath);
        }

        FileOutputStream outputStream = new FileOutputStream(fileOut);
        try {
            outputStream.write(contents.getBytes());
        } finally {
            outputStream.close();
        }
    }
}
