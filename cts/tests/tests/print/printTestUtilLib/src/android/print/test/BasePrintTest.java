/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.print.test;

import static android.content.pm.PackageManager.GET_META_DATA;
import static android.content.pm.PackageManager.GET_SERVICES;
import static android.print.test.Utils.getPrintManager;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.mockito.hamcrest.MockitoHamcrest.argThat;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.pdf.PdfDocument;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.print.PageRange;
import android.print.PrintAttributes;
import android.print.PrintDocumentAdapter;
import android.print.PrintDocumentAdapter.LayoutResultCallback;
import android.print.PrintDocumentAdapter.WriteResultCallback;
import android.print.PrintDocumentInfo;
import android.print.PrintManager;
import android.print.PrinterId;
import android.print.pdf.PrintedPdfDocument;
import android.print.test.services.PrintServiceCallbacks;
import android.print.test.services.PrinterDiscoverySessionCallbacks;
import android.print.test.services.StubbablePrintService;
import android.print.test.services.StubbablePrinterDiscoverySession;
import android.printservice.CustomPrinterIconCallback;
import android.printservice.PrintJob;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiSelector;
import android.util.Log;
import android.util.SparseArray;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.compatibility.common.util.SystemUtil;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.rules.TestRule;
import org.junit.runners.model.Statement;
import org.mockito.InOrder;
import org.mockito.stubbing.Answer;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.annotation.Annotation;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * This is the base class for print tests.
 */
public abstract class BasePrintTest {
    private static final String LOG_TAG = "BasePrintTest";

    protected static final long OPERATION_TIMEOUT_MILLIS = 60000;
    protected static final String PRINT_JOB_NAME = "Test";
    static final String TEST_ID = "BasePrintTest.EXTRA_TEST_ID";

    private static final String PRINT_SPOOLER_PACKAGE_NAME = "com.android.printspooler";
    private static final String PM_CLEAR_SUCCESS_OUTPUT = "Success";
    private static final String COMMAND_LIST_ENABLED_IME_COMPONENTS = "ime list -s";
    private static final String COMMAND_PREFIX_ENABLE_IME = "ime enable ";
    private static final String COMMAND_PREFIX_DISABLE_IME = "ime disable ";
    private static final int CURRENT_USER_ID = -2; // Mirrors UserHandle.USER_CURRENT
    private static final String PRINTSPOOLER_PACKAGE = "com.android.printspooler";

    private static final AtomicInteger sLastTestID = new AtomicInteger();
    private int mTestId;
    private PrintDocumentActivity mActivity;

    private static String sDisabledPrintServicesBefore;

    private static final SparseArray<BasePrintTest> sIdToTest = new SparseArray<>();

    public final @Rule ShouldStartActivity mShouldStartActivityRule = new ShouldStartActivity();

    /**
     * Return the UI device
     *
     * @return the UI device
     */
    public static UiDevice getUiDevice() {
        return UiDevice.getInstance(getInstrumentation());
    }

    private CallCounter mCancelOperationCounter;
    private CallCounter mLayoutCallCounter;
    private CallCounter mWriteCallCounter;
    private CallCounter mWriteCancelCallCounter;
    private CallCounter mStartCallCounter;
    private CallCounter mFinishCallCounter;
    private CallCounter mPrintJobQueuedCallCounter;
    private CallCounter mCreateSessionCallCounter;
    private CallCounter mDestroySessionCallCounter;
    private CallCounter mDestroyActivityCallCounter = new CallCounter();
    private CallCounter mCreateActivityCallCounter = new CallCounter();

    private static String[] sEnabledImes;

    private static String[] getEnabledImes() throws IOException {
        List<String> imeList = new ArrayList<>();

        ParcelFileDescriptor pfd = getInstrumentation().getUiAutomation()
                .executeShellCommand(COMMAND_LIST_ENABLED_IME_COMPONENTS);
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(new FileInputStream(pfd.getFileDescriptor())))) {

            String line;
            while ((line = reader.readLine()) != null) {
                imeList.add(line);
            }
        }

        String[] imeArray = new String[imeList.size()];
        imeList.toArray(imeArray);

        return imeArray;
    }

    private static void disableImes() throws Exception {
        sEnabledImes = getEnabledImes();
        for (String ime : sEnabledImes) {
            String disableImeCommand = COMMAND_PREFIX_DISABLE_IME + ime;
            SystemUtil.runShellCommand(getInstrumentation(), disableImeCommand);
        }
    }

    private static void enableImes() throws Exception {
        for (String ime : sEnabledImes) {
            String enableImeCommand = COMMAND_PREFIX_ENABLE_IME + ime;
            SystemUtil.runShellCommand(getInstrumentation(), enableImeCommand);
        }
        sEnabledImes = null;
    }

    protected static Instrumentation getInstrumentation() {
        return InstrumentationRegistry.getInstrumentation();
    }

    @BeforeClass
    public static void setUpClass() throws Exception {
        Log.d(LOG_TAG, "setUpClass()");

        Instrumentation instrumentation = getInstrumentation();

        // Make sure we start with a clean slate.
        Log.d(LOG_TAG, "clearPrintSpoolerData()");
        clearPrintSpoolerData();
        Log.d(LOG_TAG, "disableImes()");
        disableImes();
        Log.d(LOG_TAG, "disablePrintServices()");
        disablePrintServices(instrumentation.getTargetContext().getPackageName());

        // Workaround for dexmaker bug: https://code.google.com/p/dexmaker/issues/detail?id=2
        // Dexmaker is used by mockito.
        System.setProperty("dexmaker.dexcache", instrumentation
                .getTargetContext().getCacheDir().getPath());

        Log.d(LOG_TAG, "setUpClass() done");
    }

    /**
     * Disable all print services beside the ones we want to leave enabled.
     *
     * @param packageToLeaveEnabled The package of the services to leave enabled.
     */
    private static void disablePrintServices(@NonNull String packageToLeaveEnabled)
            throws IOException {
        Instrumentation instrumentation = getInstrumentation();

        sDisabledPrintServicesBefore = SystemUtil.runShellCommand(instrumentation,
                "settings get secure " + Settings.Secure.DISABLED_PRINT_SERVICES);

        Intent printServiceIntent = new Intent(android.printservice.PrintService.SERVICE_INTERFACE);
        List<ResolveInfo> installedServices = instrumentation.getContext().getPackageManager()
                .queryIntentServices(printServiceIntent, GET_SERVICES | GET_META_DATA);

        StringBuilder builder = new StringBuilder();
        for (ResolveInfo service : installedServices) {
            if (packageToLeaveEnabled.equals(service.serviceInfo.packageName)) {
                continue;
            }
            if (builder.length() > 0) {
                builder.append(":");
            }
            builder.append(new ComponentName(service.serviceInfo.packageName,
                    service.serviceInfo.name).flattenToString());
        }

        SystemUtil.runShellCommand(instrumentation, "settings put secure "
                + Settings.Secure.DISABLED_PRINT_SERVICES + " " + builder);
    }

    /**
     * Revert {@link #disablePrintServices(String)}
     */
    private static  void enablePrintServices() throws IOException {
        SystemUtil.runShellCommand(getInstrumentation(),
                "settings put secure " + Settings.Secure.DISABLED_PRINT_SERVICES + " "
                        + sDisabledPrintServicesBefore);
    }

    @Before
    public void setUp() throws Exception {
        Log.d(LOG_TAG, "setUp()");

        assumeTrue(getInstrumentation().getContext().getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_PRINTING));

        // Prevent rotation
        getUiDevice().freezeRotation();
        while (!getUiDevice().isNaturalOrientation()) {
            getUiDevice().setOrientationNatural();
            getUiDevice().waitForIdle();
        }

        // Initialize the latches.
        Log.d(LOG_TAG, "init counters");
        mCancelOperationCounter = new CallCounter();
        mLayoutCallCounter = new CallCounter();
        mStartCallCounter = new CallCounter();
        mFinishCallCounter = new CallCounter();
        mWriteCallCounter = new CallCounter();
        mWriteCancelCallCounter = new CallCounter();
        mFinishCallCounter = new CallCounter();
        mPrintJobQueuedCallCounter = new CallCounter();
        mCreateSessionCallCounter = new CallCounter();
        mDestroySessionCallCounter = new CallCounter();

        mTestId = sLastTestID.incrementAndGet();
        sIdToTest.put(mTestId, this);

        // Create the activity if needed
        if (!mShouldStartActivityRule.mNoActivity) {
            createActivity();
        }

        Log.d(LOG_TAG, "setUp() done");
    }

    @After
    public void tearDown() throws Exception {
        Log.d(LOG_TAG, "tearDown()");

        finishActivity();

        sIdToTest.remove(mTestId);

        // Allow rotation
        getUiDevice().unfreezeRotation();

        Log.d(LOG_TAG, "tearDown() done");
    }

    @AfterClass
    public static void tearDownClass() throws Exception {
        Log.d(LOG_TAG, "tearDownClass()");

        Instrumentation instrumentation = getInstrumentation();

        Log.d(LOG_TAG, "enablePrintServices()");
        enablePrintServices();

        Log.d(LOG_TAG, "enableImes()");
        enableImes();

        // Make sure the spooler is cleaned, this also un-approves all services
        Log.d(LOG_TAG, "clearPrintSpoolerData()");
        clearPrintSpoolerData();

        SystemUtil.runShellCommand(instrumentation, "settings put secure "
                    + Settings.Secure.DISABLED_PRINT_SERVICES + " null");

        Log.d(LOG_TAG, "tearDownClass() done");
    }

    protected android.print.PrintJob print(@NonNull final PrintDocumentAdapter adapter,
            final PrintAttributes attributes) {
        android.print.PrintJob[] printJob = new android.print.PrintJob[1];
        // Initiate printing as if coming from the app.
        getInstrumentation().runOnMainSync(() -> {
            PrintManager printManager = (PrintManager) getActivity()
                    .getSystemService(Context.PRINT_SERVICE);
            printJob[0] = printManager.print("Print job", adapter, attributes);
        });

        return printJob[0];
    }

    protected void print(PrintDocumentAdapter adapter) {
        print(adapter, (PrintAttributes) null);
    }

    protected void print(PrintDocumentAdapter adapter, String printJobName) {
        print(adapter, printJobName, null);
    }

    /**
     * Start printing
     *
     * @param adapter      Adapter supplying data to print
     * @param printJobName The name of the print job
     */
    protected void print(@NonNull PrintDocumentAdapter adapter, @NonNull String printJobName,
            @Nullable PrintAttributes attributes) {
        // Initiate printing as if coming from the app.
        getInstrumentation()
                .runOnMainSync(() -> getPrintManager(getActivity()).print(printJobName, adapter,
                        attributes));
    }

    protected void onCancelOperationCalled() {
        mCancelOperationCounter.call();
    }

    public void onStartCalled() {
        mStartCallCounter.call();
    }

    protected void onLayoutCalled() {
        mLayoutCallCounter.call();
    }

    protected int getWriteCallCount() {
        return mWriteCallCounter.getCallCount();
    }

    protected void onWriteCalled() {
        mWriteCallCounter.call();
    }

    protected void onWriteCancelCalled() {
        mWriteCancelCallCounter.call();
    }

    protected void onFinishCalled() {
        mFinishCallCounter.call();
    }

    protected void onPrintJobQueuedCalled() {
        mPrintJobQueuedCallCounter.call();
    }

    protected void onPrinterDiscoverySessionCreateCalled() {
        mCreateSessionCallCounter.call();
    }

    protected void onPrinterDiscoverySessionDestroyCalled() {
        mDestroySessionCallCounter.call();
    }

    protected void waitForCancelOperationCallbackCalled() {
        waitForCallbackCallCount(mCancelOperationCounter, 1,
                "Did not get expected call to onCancel for the current operation.");
    }

    protected void waitForPrinterDiscoverySessionCreateCallbackCalled() {
        waitForCallbackCallCount(mCreateSessionCallCounter, 1,
                "Did not get expected call to onCreatePrinterDiscoverySession.");
    }

    public void waitForPrinterDiscoverySessionDestroyCallbackCalled(int count) {
        waitForCallbackCallCount(mDestroySessionCallCounter, count,
                "Did not get expected call to onDestroyPrinterDiscoverySession.");
    }

    protected void waitForServiceOnPrintJobQueuedCallbackCalled(int count) {
        waitForCallbackCallCount(mPrintJobQueuedCallCounter, count,
                "Did not get expected call to onPrintJobQueued.");
    }

    protected void waitForAdapterStartCallbackCalled() {
        waitForCallbackCallCount(mStartCallCounter, 1,
                "Did not get expected call to start.");
    }

    protected void waitForAdapterFinishCallbackCalled() {
        waitForCallbackCallCount(mFinishCallCounter, 1,
                "Did not get expected call to finish.");
    }

    protected void waitForLayoutAdapterCallbackCount(int count) {
        waitForCallbackCallCount(mLayoutCallCounter, count,
                "Did not get expected call to layout.");
    }

    public void waitForWriteAdapterCallback(int count) {
        waitForCallbackCallCount(mWriteCallCounter, count, "Did not get expected call to write.");
    }

    protected void waitForWriteCancelCallback(int count) {
        waitForCallbackCallCount(mWriteCancelCallCounter, count,
                "Did not get expected cancel of write.");
    }

    private static void waitForCallbackCallCount(CallCounter counter, int count, String message) {
        try {
            counter.waitForCount(count, OPERATION_TIMEOUT_MILLIS);
        } catch (TimeoutException te) {
            fail(message);
        }
    }

    /**
     * Indicate the print activity was created.
     */
    static void onActivityCreateCalled(int testId, PrintDocumentActivity activity) {
        synchronized (sIdToTest) {
            BasePrintTest test = sIdToTest.get(testId);
            if (test != null) {
                test.mActivity = activity;
                test.mCreateActivityCallCounter.call();
            }
        }
    }

    /**
     * Indicate the print activity was destroyed.
     */
    static void onActivityDestroyCalled(int testId) {
        synchronized (sIdToTest) {
            BasePrintTest test = sIdToTest.get(testId);
            if (test != null) {
                test.mDestroyActivityCallCounter.call();
            }
        }
    }

    private void finishActivity() {
        Activity activity = mActivity;

        if (activity != null) {
            if (!activity.isFinishing()) {
                activity.finish();
            }

            while (!activity.isDestroyed()) {
                int creates = mCreateActivityCallCounter.getCallCount();
                waitForCallbackCallCount(mDestroyActivityCallCounter, creates,
                        "Activity was not destroyed");
            }
        }
    }

    /**
     * Get the number of ties the print activity was destroyed.
     *
     * @return The number of onDestroy calls on the print activity.
     */
    protected int getActivityDestroyCallbackCallCount() {
        return mDestroyActivityCallCounter.getCallCount();
    }

    /**
     * Get the number of ties the print activity was created.
     *
     * @return The number of onCreate calls on the print activity.
     */
    private int getActivityCreateCallbackCallCount() {
        return mCreateActivityCallCounter.getCallCount();
    }

    /**
     * Wait until create was called {@code count} times.
     *
     * @param count The number of create calls to expect.
     */
    private void waitForActivityCreateCallbackCalled(int count) {
        waitForCallbackCallCount(mCreateActivityCallCounter, count,
                "Did not get expected call to create.");
    }

    /**
     * Reset all counters.
     */
    public void resetCounters() {
        mCancelOperationCounter.reset();
        mLayoutCallCounter.reset();
        mWriteCallCounter.reset();
        mWriteCancelCallCounter.reset();
        mStartCallCounter.reset();
        mFinishCallCounter.reset();
        mPrintJobQueuedCallCounter.reset();
        mCreateSessionCallCounter.reset();
        mDestroySessionCallCounter.reset();
        mDestroyActivityCallCounter.reset();
        mCreateActivityCallCounter.reset();
    }

    /**
     * Wait until the message is shown that indicates that a printer is unavailable.
     *
     * @throws Exception If anything was unexpected.
     */
    protected void waitForPrinterUnavailable() throws Exception {
        final String printerUnavailableMessage = "This printer isn\'t available right now.";

        UiObject message = getUiDevice().findObject(new UiSelector().resourceId(
                "com.android.printspooler:id/message"));
        if (!message.getText().equals(printerUnavailableMessage)) {
            throw new Exception("Wrong message: " + message.getText() + " instead of "
                    + printerUnavailableMessage);
        }
    }

    protected void selectPrinter(String printerName) throws UiObjectNotFoundException, IOException {
        try {
            long delay = 1;
            while (true) {
                try {
                    UiDevice uiDevice = getUiDevice();
                    UiObject destinationSpinner = uiDevice.findObject(new UiSelector()
                            .resourceId("com.android.printspooler:id/destination_spinner"));

                    destinationSpinner.click();
                    getUiDevice().waitForIdle();

                    // Give spinner some time to expand
                    try {
                        Thread.sleep(delay);
                    } catch (InterruptedException e) {
                        // ignore
                    }

                    // try to select printer
                    UiObject printerOption = uiDevice.findObject(
                            new UiSelector().text(printerName));
                    printerOption.click();
                } catch (UiObjectNotFoundException e) {
                    Log.e(LOG_TAG, "Could not select printer " + printerName, e);
                }

                getUiDevice().waitForIdle();

                if (!printerName.equals("All printers…")) {
                    // Make sure printer is selected
                    if (getUiDevice().hasObject(By.text(printerName))) {
                        break;
                    } else {
                        if (delay <= OPERATION_TIMEOUT_MILLIS) {
                            Log.w(LOG_TAG, "Cannot find printer " + printerName + ", retrying.");
                            delay *= 2;
                        } else {
                            throw new UiObjectNotFoundException(
                                    "Could find printer " + printerName
                                            + " even though we retried");
                        }
                    }
                } else {
                    break;
                }
            }
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void answerPrintServicesWarning(boolean confirm) throws UiObjectNotFoundException {
        UiObject button;
        if (confirm) {
            button = getUiDevice().findObject(new UiSelector().resourceId("android:id/button1"));
        } else {
            button = getUiDevice().findObject(new UiSelector().resourceId("android:id/button2"));
        }
        button.click();
    }

    protected void changeOrientation(String orientation) throws UiObjectNotFoundException,
            IOException {
        try {
            UiDevice uiDevice = getUiDevice();
            UiObject orientationSpinner = uiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/orientation_spinner"));
            orientationSpinner.click();
            UiObject orientationOption = uiDevice.findObject(new UiSelector().text(orientation));
            orientationOption.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    public String getOrientation() throws UiObjectNotFoundException, IOException {
        try {
            UiObject orientationSpinner = getUiDevice().findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/orientation_spinner"));
            return orientationSpinner.getText();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void changeMediaSize(String mediaSize) throws UiObjectNotFoundException, IOException {
        try {
            UiDevice uiDevice = getUiDevice();
            UiObject mediaSizeSpinner = uiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/paper_size_spinner"));
            mediaSizeSpinner.click();
            UiObject mediaSizeOption = uiDevice.findObject(new UiSelector().text(mediaSize));
            mediaSizeOption.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void changeColor(String color) throws UiObjectNotFoundException, IOException {
        try {
            UiDevice uiDevice = getUiDevice();
            UiObject colorSpinner = uiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/color_spinner"));
            colorSpinner.click();
            UiObject colorOption = uiDevice.findObject(new UiSelector().text(color));
            colorOption.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    public String getColor() throws UiObjectNotFoundException, IOException {
        try {
            UiObject colorSpinner = getUiDevice().findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/color_spinner"));
            return colorSpinner.getText();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void changeDuplex(String duplex) throws UiObjectNotFoundException, IOException {
        try {
            UiDevice uiDevice = getUiDevice();
            UiObject duplexSpinner = uiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/duplex_spinner"));
            duplexSpinner.click();
            UiObject duplexOption = uiDevice.findObject(new UiSelector().text(duplex));
            duplexOption.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void changeCopies(int newCopies) throws UiObjectNotFoundException, IOException {
        try {
            UiObject copies = getUiDevice().findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/copies_edittext"));
            copies.setText(Integer.valueOf(newCopies).toString());
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected String getCopies() throws UiObjectNotFoundException, IOException {
        try {
            UiObject copies = getUiDevice().findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/copies_edittext"));
            return copies.getText();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void assertNoPrintButton() throws UiObjectNotFoundException, IOException {
        assertFalse(getUiDevice().hasObject(By.res("com.android.printspooler:id/print_button")));
    }

    public void clickPrintButton() throws UiObjectNotFoundException, IOException {
        try {
            UiObject printButton = getUiDevice().findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/print_button"));
            printButton.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void clickRetryButton() throws UiObjectNotFoundException, IOException {
        try {
            UiObject retryButton = getUiDevice().findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/action_button"));
            retryButton.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    public void dumpWindowHierarchy() throws IOException {
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        getUiDevice().dumpWindowHierarchy(os);

        Log.w(LOG_TAG, "Window hierarchy:");
        for (String line : os.toString("UTF-8").split("\n")) {
            Log.w(LOG_TAG, line);
        }
    }

    protected PrintDocumentActivity getActivity() {
        return mActivity;
    }

    protected void createActivity() throws IOException {
        Log.d(LOG_TAG, "createActivity()");

        int createBefore = getActivityCreateCallbackCallCount();

        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.putExtra(TEST_ID, mTestId);

        Instrumentation instrumentation = getInstrumentation();

        // Unlock screen.
        SystemUtil.runShellCommand(instrumentation, "input keyevent KEYCODE_WAKEUP");

        intent.setClassName(instrumentation.getTargetContext().getPackageName(),
                PrintDocumentActivity.class.getName());
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        instrumentation.startActivitySync(intent);

        waitForActivityCreateCallbackCalled(createBefore + 1);
    }

    protected void openPrintOptions() throws UiObjectNotFoundException {
        UiObject expandHandle = getUiDevice().findObject(new UiSelector().resourceId(
                "com.android.printspooler:id/expand_collapse_handle"));
        expandHandle.click();
    }

    protected void openCustomPrintOptions() throws UiObjectNotFoundException {
        UiObject expandHandle = getUiDevice().findObject(new UiSelector().resourceId(
                "com.android.printspooler:id/more_options_button"));
        expandHandle.click();
    }

    protected static void clearPrintSpoolerData() throws Exception {
        if (getInstrumentation().getContext().getPackageManager().hasSystemFeature(
                  PackageManager.FEATURE_PRINTING)) {
            assertTrue("failed to clear print spooler data",
                    SystemUtil.runShellCommand(getInstrumentation(), String.format(
                            "pm clear --user %d %s", CURRENT_USER_ID, PRINT_SPOOLER_PACKAGE_NAME))
                            .contains(PM_CLEAR_SUCCESS_OUTPUT));
        }
    }

    protected void verifyLayoutCall(InOrder inOrder, PrintDocumentAdapter mock,
            PrintAttributes oldAttributes, PrintAttributes newAttributes,
            final boolean forPreview) {
        inOrder.verify(mock).onLayout(eq(oldAttributes), eq(newAttributes),
                any(CancellationSignal.class), any(LayoutResultCallback.class), argThat(
                        new BaseMatcher<Bundle>() {
                            @Override
                            public boolean matches(Object item) {
                                Bundle bundle = (Bundle) item;
                                return forPreview == bundle.getBoolean(
                                        PrintDocumentAdapter.EXTRA_PRINT_PREVIEW);
                            }

                            @Override
                            public void describeTo(Description description) {
                                /* do nothing */
                            }
                        }));
    }

    protected PrintDocumentAdapter createMockPrintDocumentAdapter(Answer<Void> layoutAnswer,
            Answer<Void> writeAnswer, Answer<Void> finishAnswer) {
        // Create a mock print adapter.
        PrintDocumentAdapter adapter = mock(PrintDocumentAdapter.class);
        if (layoutAnswer != null) {
            doAnswer(layoutAnswer).when(adapter).onLayout(any(PrintAttributes.class),
                    any(PrintAttributes.class), any(CancellationSignal.class),
                    any(LayoutResultCallback.class), any(Bundle.class));
        }
        if (writeAnswer != null) {
            doAnswer(writeAnswer).when(adapter).onWrite(any(PageRange[].class),
                    any(ParcelFileDescriptor.class), any(CancellationSignal.class),
                    any(WriteResultCallback.class));
        }
        if (finishAnswer != null) {
            doAnswer(finishAnswer).when(adapter).onFinish();
        }
        return adapter;
    }

    /**
     * Create a mock {@link PrintDocumentAdapter} that provides one empty page.
     *
     * @return The mock adapter
     */
    public @NonNull PrintDocumentAdapter createDefaultPrintDocumentAdapter(int numPages) {
        final PrintAttributes[] printAttributes = new PrintAttributes[1];

        return createMockPrintDocumentAdapter(
                invocation -> {
                    PrintAttributes oldAttributes = (PrintAttributes) invocation.getArguments()[0];
                    printAttributes[0] = (PrintAttributes) invocation.getArguments()[1];
                    LayoutResultCallback callback =
                            (LayoutResultCallback) invocation
                                    .getArguments()[3];

                    callback.onLayoutFinished(new PrintDocumentInfo.Builder(PRINT_JOB_NAME)
                            .setPageCount(numPages).build(),
                            !oldAttributes.equals(printAttributes[0]));

                    oldAttributes = printAttributes[0];

                    onLayoutCalled();
                    return null;
                }, invocation -> {
                    Object[] args = invocation.getArguments();
                    PageRange[] pages = (PageRange[]) args[0];
                    ParcelFileDescriptor fd = (ParcelFileDescriptor) args[1];
                    WriteResultCallback callback =
                            (WriteResultCallback) args[3];

                    writeBlankPages(printAttributes[0], fd, pages[0].getStart(), pages[0].getEnd());
                    fd.close();
                    callback.onWriteFinished(pages);

                    onWriteCalled();
                    return null;
                }, invocation -> {
                    onFinishCalled();
                    return null;
                });
    }

    @SuppressWarnings("unchecked")
    protected static PrinterDiscoverySessionCallbacks createMockPrinterDiscoverySessionCallbacks(
            Answer<Void> onStartPrinterDiscovery, Answer<Void> onStopPrinterDiscovery,
            Answer<Void> onValidatePrinters, Answer<Void> onStartPrinterStateTracking,
            Answer<Void> onRequestCustomPrinterIcon, Answer<Void> onStopPrinterStateTracking,
            Answer<Void> onDestroy) {
        PrinterDiscoverySessionCallbacks callbacks = mock(PrinterDiscoverySessionCallbacks.class);

        doCallRealMethod().when(callbacks).setSession(any(StubbablePrinterDiscoverySession.class));
        when(callbacks.getSession()).thenCallRealMethod();

        if (onStartPrinterDiscovery != null) {
            doAnswer(onStartPrinterDiscovery).when(callbacks).onStartPrinterDiscovery(
                    any(List.class));
        }
        if (onStopPrinterDiscovery != null) {
            doAnswer(onStopPrinterDiscovery).when(callbacks).onStopPrinterDiscovery();
        }
        if (onValidatePrinters != null) {
            doAnswer(onValidatePrinters).when(callbacks).onValidatePrinters(
                    any(List.class));
        }
        if (onStartPrinterStateTracking != null) {
            doAnswer(onStartPrinterStateTracking).when(callbacks).onStartPrinterStateTracking(
                    any(PrinterId.class));
        }
        if (onRequestCustomPrinterIcon != null) {
            doAnswer(onRequestCustomPrinterIcon).when(callbacks).onRequestCustomPrinterIcon(
                    any(PrinterId.class), any(CancellationSignal.class),
                    any(CustomPrinterIconCallback.class));
        }
        if (onStopPrinterStateTracking != null) {
            doAnswer(onStopPrinterStateTracking).when(callbacks).onStopPrinterStateTracking(
                    any(PrinterId.class));
        }
        if (onDestroy != null) {
            doAnswer(onDestroy).when(callbacks).onDestroy();
        }

        return callbacks;
    }

    protected PrintServiceCallbacks createMockPrintServiceCallbacks(
            Answer<PrinterDiscoverySessionCallbacks> onCreatePrinterDiscoverySessionCallbacks,
            Answer<Void> onPrintJobQueued, Answer<Void> onRequestCancelPrintJob) {
        final PrintServiceCallbacks service = mock(PrintServiceCallbacks.class);

        doCallRealMethod().when(service).setService(any(StubbablePrintService.class));
        when(service.getService()).thenCallRealMethod();

        if (onCreatePrinterDiscoverySessionCallbacks != null) {
            doAnswer(onCreatePrinterDiscoverySessionCallbacks).when(service)
                    .onCreatePrinterDiscoverySessionCallbacks();
        }
        if (onPrintJobQueued != null) {
            doAnswer(onPrintJobQueued).when(service).onPrintJobQueued(any(PrintJob.class));
        }
        if (onRequestCancelPrintJob != null) {
            doAnswer(onRequestCancelPrintJob).when(service).onRequestCancelPrintJob(
                    any(PrintJob.class));
        }

        return service;
    }

    protected void writeBlankPages(PrintAttributes constraints, ParcelFileDescriptor output,
            int fromIndex, int toIndex) throws IOException {
        PrintedPdfDocument document = new PrintedPdfDocument(getActivity(), constraints);
        final int pageCount = toIndex - fromIndex + 1;
        for (int i = 0; i < pageCount; i++) {
            PdfDocument.Page page = document.startPage(i);
            document.finishPage(page);
        }
        FileOutputStream fos = new FileOutputStream(output.getFileDescriptor());
        document.writeTo(fos);
        fos.flush();
        document.close();
    }

    protected void selectPages(String pages, int totalPages) throws Exception {
        UiObject pagesSpinner = getUiDevice().findObject(new UiSelector().resourceId(
                "com.android.printspooler:id/range_options_spinner"));
        pagesSpinner.click();

        UiObject rangeOption = getUiDevice().findObject(new UiSelector().textContains("Range of "
                + totalPages));
        rangeOption.click();

        UiObject pagesEditText = getUiDevice().findObject(new UiSelector().resourceId(
                "com.android.printspooler:id/page_range_edittext"));
        pagesEditText.setText(pages);

        // Hide the keyboard.
        getUiDevice().pressBack();
    }

    private static final class CallCounter {
        private final Object mLock = new Object();

        private int mCallCount;

        public void call() {
            synchronized (mLock) {
                mCallCount++;
                mLock.notifyAll();
            }
        }

        int getCallCount() {
            synchronized (mLock) {
                return mCallCount;
            }
        }

        public void reset() {
            synchronized (mLock) {
                mCallCount = 0;
            }
        }

        public void waitForCount(int count, long timeoutMillis) throws TimeoutException {
            synchronized (mLock) {
                final long startTimeMillis = SystemClock.uptimeMillis();
                while (mCallCount < count) {
                    try {
                        final long elapsedTimeMillis = SystemClock.uptimeMillis() - startTimeMillis;
                        final long remainingTimeMillis = timeoutMillis - elapsedTimeMillis;
                        if (remainingTimeMillis <= 0) {
                            throw new TimeoutException();
                        }
                        mLock.wait(timeoutMillis);
                    } catch (InterruptedException ie) {
                        /* ignore */
                    }
                }
            }
        }
    }


    /**
     * Make {@code printerName} the default printer by adding it to the history of printers by
     * printing once.
     *
     * @param adapter The {@link PrintDocumentAdapter} used
     * @throws Exception If the printer could not be made default
     */
    public void makeDefaultPrinter(PrintDocumentAdapter adapter, String printerName)
            throws Exception {
        // Perform a full print operation on the printer
        Log.d(LOG_TAG, "print");
        print(adapter);
        Log.d(LOG_TAG, "waitForWriteAdapterCallback");
        waitForWriteAdapterCallback(1);
        Log.d(LOG_TAG, "selectPrinter");
        selectPrinter(printerName);
        Log.d(LOG_TAG, "clickPrintButton");
        clickPrintButton();
        Log.d(LOG_TAG, "answerPrintServicesWarning");
        answerPrintServicesWarning(true);
        Log.d(LOG_TAG, "waitForPrinterDiscoverySessionDestroyCallbackCalled");
        waitForPrinterDiscoverySessionDestroyCallbackCalled(1);

        // Switch to new activity, which should now use the default printer
        Log.d(LOG_TAG, "getActivity().finish()");
        getActivity().finish();

        createActivity();
    }

    /**
     * Annotation used to signal that a test does not need an activity.
     */
    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.METHOD)
    protected @interface NoActivity { }

    /**
     * Rule that handles the {@link NoActivity} annotation.
     */
    private static class ShouldStartActivity implements TestRule {
        boolean mNoActivity;

        @Override
        public Statement apply(Statement base, org.junit.runner.Description description) {
            for (Annotation annotation : description.getAnnotations()) {
                if (annotation instanceof NoActivity) {
                    mNoActivity = true;
                    break;
                }
            }

            return base;
        }
    }
}
