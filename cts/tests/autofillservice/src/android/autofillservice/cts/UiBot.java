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

package android.autofillservice.cts;

import static android.autofillservice.cts.Timeouts.DATASET_PICKER_NOT_SHOWN_NAPTIME_MS;
import static android.autofillservice.cts.Timeouts.SAVE_NOT_SHOWN_NAPTIME_MS;
import static android.autofillservice.cts.Timeouts.SAVE_TIMEOUT;
import static android.autofillservice.cts.Timeouts.UI_DATASET_PICKER_TIMEOUT;
import static android.autofillservice.cts.Timeouts.UI_SCREEN_ORIENTATION_TIMEOUT;
import static android.autofillservice.cts.Timeouts.UI_TIMEOUT;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_ADDRESS;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_CREDIT_CARD;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_EMAIL_ADDRESS;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_GENERIC;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_PASSWORD;
import static android.service.autofill.SaveInfo.SAVE_DATA_TYPE_USERNAME;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.os.SystemClock;
import android.service.autofill.SaveInfo;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.text.Html;
import android.util.Log;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityWindowInfo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * Helper for UI-related needs.
 */
final class UiBot {

    private static final String TAG = "AutoFillCtsUiBot";

    private static final String RESOURCE_ID_DATASET_PICKER = "autofill_dataset_picker";
    private static final String RESOURCE_ID_DATASET_HEADER = "autofill_dataset_header";
    private static final String RESOURCE_ID_SAVE_SNACKBAR = "autofill_save";
    private static final String RESOURCE_ID_SAVE_ICON = "autofill_save_icon";
    private static final String RESOURCE_ID_SAVE_TITLE = "autofill_save_title";
    private static final String RESOURCE_ID_CONTEXT_MENUITEM = "floating_toolbar_menu_item_text";
    private static final String RESOURCE_ID_SAVE_BUTTON_NO = "autofill_save_no";

    private static final String RESOURCE_STRING_SAVE_TITLE = "autofill_save_title";
    private static final String RESOURCE_STRING_SAVE_TITLE_WITH_TYPE =
            "autofill_save_title_with_type";
    private static final String RESOURCE_STRING_SAVE_TYPE_PASSWORD = "autofill_save_type_password";
    private static final String RESOURCE_STRING_SAVE_TYPE_ADDRESS = "autofill_save_type_address";
    private static final String RESOURCE_STRING_SAVE_TYPE_CREDIT_CARD =
            "autofill_save_type_credit_card";
    private static final String RESOURCE_STRING_SAVE_TYPE_USERNAME = "autofill_save_type_username";
    private static final String RESOURCE_STRING_SAVE_TYPE_EMAIL_ADDRESS =
            "autofill_save_type_email_address";
    private static final String RESOURCE_STRING_SAVE_BUTTON_NOT_NOW = "save_password_notnow";
    private static final String RESOURCE_STRING_SAVE_BUTTON_NO_THANKS = "autofill_save_no";

    private static final String RESOURCE_STRING_AUTOFILL = "autofill";
    private static final String RESOURCE_STRING_DATASET_PICKER_ACCESSIBILITY_TITLE =
            "autofill_picker_accessibility_title";
    private static final String RESOURCE_STRING_SAVE_SNACKBAR_ACCESSIBILITY_TITLE =
            "autofill_save_accessibility_title";

    static final BySelector DATASET_PICKER_SELECTOR = By.res("android", RESOURCE_ID_DATASET_PICKER);
    private static final BySelector SAVE_UI_SELECTOR = By.res("android", RESOURCE_ID_SAVE_SNACKBAR);
    private static final BySelector DATASET_HEADER_SELECTOR =
            By.res("android", RESOURCE_ID_DATASET_HEADER);

    private static final boolean DUMP_ON_ERROR = true;

    /** Pass to {@link #setScreenOrientation(int)} to change the display to portrait mode */
    public static int PORTRAIT = 0;

    /** Pass to {@link #setScreenOrientation(int)} to change the display to landscape mode */
    public static int LANDSCAPE = 1;

    private final UiDevice mDevice;
    private final Context mContext;
    private final String mPackageName;
    private final UiAutomation mAutoman;
    private final Timeout mDefaultTimeout;

    private boolean mOkToCallAssertNoDatasets;

    UiBot() {
        this(UI_TIMEOUT);
    }

    UiBot(Timeout defaultTimeout) {
        mDefaultTimeout = defaultTimeout;
        final Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        mDevice = UiDevice.getInstance(instrumentation);
        mContext = instrumentation.getContext();
        mPackageName = mContext.getPackageName();
        mAutoman = instrumentation.getUiAutomation();
    }

    void reset() {
        mOkToCallAssertNoDatasets = false;
    }

    UiDevice getDevice() {
        return mDevice;
    }

    /**
     * Asserts the dataset picker is not shown anymore.
     *
     * @throws IllegalStateException if called *before* an assertion was made to make sure the
     * dataset picker is shown - if that's not the case, call
     * {@link #assertNoDatasetsEver()} instead.
     */
    void assertNoDatasets() throws Exception {
        if (!mOkToCallAssertNoDatasets) {
            throw new IllegalStateException(
                    "Cannot call assertNoDatasets() without calling assertDatasets first");
        }
        mDevice.wait(Until.gone(DATASET_PICKER_SELECTOR), UI_DATASET_PICKER_TIMEOUT.ms());
        mOkToCallAssertNoDatasets = false;
    }

    /**
     * Asserts the dataset picker was never shown.
     *
     * <p>This method is slower than {@link #assertNoDatasets()} and should only be called in the
     * cases where the dataset picker was not previous shown.
     */
    void assertNoDatasetsEver() throws Exception {
        assertNeverShown("dataset picker", DATASET_PICKER_SELECTOR,
                DATASET_PICKER_NOT_SHOWN_NAPTIME_MS);
    }

    /**
     * Asserts the dataset chooser is shown and contains exactly the given datasets.
     *
     * @return the dataset picker object.
     */
    UiObject2 assertDatasets(String...names) throws Exception {
        // TODO: change run() so it can rethrow the original message
        return UI_DATASET_PICKER_TIMEOUT.run("assertDatasets: " + Arrays.toString(names), () -> {
            final UiObject2 picker = findDatasetPicker(UI_DATASET_PICKER_TIMEOUT);
            try {
                // TODO: use a library to check it contains, instead of asserThat + catch exception
                assertWithMessage("wrong dataset names").that(getChildrenAsText(picker))
                        .containsExactlyElementsIn(Arrays.asList(names)).inOrder();
                return picker;
            } catch (AssertionError e) {
                // Value mismatch - most likely UI didn't change yet, try again
                Log.w(TAG, "datasets don't match yet: " + e.getMessage());
                return null;
            }
        });
    }

    /**
     * Asserts the dataset chooser is shown and contains the given datasets.
     *
     * @return the dataset picker object.
     */
    UiObject2 assertDatasetsContains(String...names) throws Exception {
        // TODO: change run() so it can rethrow the original message
        return UI_DATASET_PICKER_TIMEOUT.run("assertDatasets: " + Arrays.toString(names), () -> {
            final UiObject2 picker = findDatasetPicker(UI_DATASET_PICKER_TIMEOUT);
            try {
                // TODO: use a library to check it contains, instead of asserThat + catch exception
                assertWithMessage("wrong dataset names").that(getChildrenAsText(picker))
                .containsAllIn(Arrays.asList(names)).inOrder();
                return picker;
            } catch (AssertionError e) {
                // Value mismatch - most likely UI didn't change yet, try again
                Log.w(TAG, "datasets don't match yet: " + e.getMessage());
                return null;
            }
        });
    }

    /**
     * Asserts the dataset chooser is shown and contains the given datasets, header, and footer.
     * <p>In fullscreen, header view is not under R.id.autofill_dataset_picker.
     *
     * @return the dataset picker object.
     */
    UiObject2 assertDatasetsWithBorders(String header, String footer, String...names)
            throws Exception {
        final UiObject2 picker = findDatasetPicker(UI_DATASET_PICKER_TIMEOUT);
        final List<String> expectedChild = new ArrayList<>();
        if (header != null) {
            if (Helper.isAutofillWindowFullScreen(mContext)) {
                final UiObject2 headerView = waitForObject(DATASET_HEADER_SELECTOR,
                        UI_DATASET_PICKER_TIMEOUT);
                assertWithMessage("fullscreen wrong dataset header")
                        .that(getChildrenAsText(headerView))
                        .containsExactlyElementsIn(Arrays.asList(header)).inOrder();
            } else {
                expectedChild.add(header);
            }
        }
        expectedChild.addAll(Arrays.asList(names));
        if (footer != null) {
            expectedChild.add(footer);
        }
        assertWithMessage("wrong elements on dataset picker").that(getChildrenAsText(picker))
                .containsExactlyElementsIn(expectedChild).inOrder();
        return picker;
    }

    /**
     * Gets the text of this object children.
     */
    List<String> getChildrenAsText(UiObject2 object) {
        final List<String> list = new ArrayList<>();
        getChildrenAsText(object, list);
        return list;
    }

    private static void getChildrenAsText(UiObject2 object, List<String> children) {
        final String text = object.getText();
        if (text != null) {
            children.add(text);
        }
        for (UiObject2 child : object.getChildren()) {
            getChildrenAsText(child, children);
        }
    }

    /**
     * Selects a dataset that should be visible in the floating UI.
     */
    void selectDataset(String name) throws Exception {
        final UiObject2 picker = findDatasetPicker(UI_DATASET_PICKER_TIMEOUT);
        selectDataset(picker, name);
    }

    /**
     * Selects a dataset that should be visible in the floating UI.
     */
    void selectDataset(UiObject2 picker, String name) {
        final UiObject2 dataset = picker.findObject(By.text(name));
        if (dataset == null) {
            throw new AssertionError("no dataset " + name + " in " + getChildrenAsText(picker));
        }
        dataset.click();
    }

    /**
     * Selects a view by text.
     *
     * <p><b>NOTE:</b> when selecting an option in dataset picker is shown, prefer
     * {@link #selectDataset(String)}.
     */
    void selectByText(String name) throws Exception {
        Log.v(TAG, "selectByText(): " + name);

        final UiObject2 object = waitForObject(By.text(name));
        object.click();
    }

    /**
     * Asserts a text is shown.
     *
     * <p><b>NOTE:</b> when asserting the dataset picker is shown, prefer
     * {@link #assertDatasets(String...)}.
     */
    public UiObject2 assertShownByText(String text) throws Exception {
        return assertShownByText(text, mDefaultTimeout);
    }

    public UiObject2 assertShownByText(String text, Timeout timeout) throws Exception {
        final UiObject2 object = waitForObject(By.text(text), timeout);
        assertWithMessage("No node with text '%s'", text).that(object).isNotNull();
        return object;
    }

    /**
     * Asserts that the text is not showing for sure in the screen "as is", i.e., without waiting
     * for it.
     *
     * <p>Typically called after another assertion that waits for a condition to be shown.
     */
    public void assertNotShowingForSure(String text) throws Exception {
        final UiObject2 object = mDevice.findObject(By.text(text));
        assertWithMessage("Find node with text '%s'", text).that(object).isNull();
    }

    /**
     * Asserts a node with the given content description is shown.
     *
     */
    public UiObject2 assertShownByContentDescription(String contentDescription) throws Exception {
        final UiObject2 object = waitForObject(By.desc(contentDescription));
        assertWithMessage("No node with content description '%s'", contentDescription).that(object)
                .isNotNull();
        return object;
    }

    /**
     * Checks if a View with a certain text exists.
     */
    boolean hasViewWithText(String name) {
        Log.v(TAG, "hasViewWithText(): " + name);

        return mDevice.findObject(By.text(name)) != null;
    }

    /**
     * Selects a view by id.
     */
    UiObject2 selectByRelativeId(String id) throws Exception {
        Log.v(TAG, "selectByRelativeId(): " + id);
        UiObject2 object = waitForObject(By.res(mPackageName, id));
        object.click();
        return object;
    }

    /**
     * Asserts the id is shown on the screen.
     */
    UiObject2 assertShownById(String id) throws Exception {
        final UiObject2 object = waitForObject(By.res(id));
        assertThat(object).isNotNull();
        return object;
    }

    /**
     * Asserts the id is shown on the screen, using a resource id from the test package.
     */
    UiObject2 assertShownByRelativeId(String id) throws Exception {
        return assertShownByRelativeId(id, mDefaultTimeout);
    }

    UiObject2 assertShownByRelativeId(String id, Timeout timeout) throws Exception {
        final UiObject2 obj = waitForObject(By.res(mPackageName, id), timeout);
        assertThat(obj).isNotNull();
        return obj;
    }

    /**
     * Asserts the id is not shown on the screen anymore, using a resource id from the test package.
     *
     * <p><b>Note:</b> this method should only called AFTER the id was previously shown, otherwise
     * it might pass without really asserting anything.
     */
    void assertGoneByRelativeId(String id, Timeout timeout) {
        boolean gone = mDevice.wait(Until.gone(By.res(mPackageName, id)), timeout.ms());
        if (!gone) {
            final String message = "Object with id '" + id + "' should be gone after "
                    + timeout + " ms";
            dumpScreen(message);
            throw new RetryableException(message);
        }
    }

    /**
     * Asserts that a {@code selector} is not showing after {@code timeout} milliseconds.
     */
    private void assertNeverShown(String description, BySelector selector, long timeout)
            throws Exception {
        SystemClock.sleep(timeout);
        final UiObject2 object = mDevice.findObject(selector);
        if (object != null) {
            throw new AssertionError(
                    String.format("Should not be showing %s after %dms, but got %s",
                            description, timeout, getChildrenAsText(object)));
        }
    }

    /**
     * Gets the text set on a view.
     */
    String getTextByRelativeId(String id) throws Exception {
        return waitForObject(By.res(mPackageName, id)).getText();
    }

    /**
     * Focus in the view with the given resource id.
     */
    void focusByRelativeId(String id) throws Exception {
        waitForObject(By.res(mPackageName, id)).click();
    }

    /**
     * Sets a new text on a view.
     */
    void setTextByRelativeId(String id, String newText) throws Exception {
        waitForObject(By.res(mPackageName, id)).setText(newText);
    }

    /**
     * Asserts the save snackbar is showing and returns it.
     */
    UiObject2 assertSaveShowing(int type) throws Exception {
        return assertSaveShowing(SAVE_TIMEOUT, type);
    }

    /**
     * Asserts the save snackbar is showing and returns it.
     */
    UiObject2 assertSaveShowing(Timeout timeout, int type) throws Exception {
        return assertSaveShowing(null, timeout, type);
    }

    /**
     * Presses the Back button.
     */
    void pressBack() {
        Log.d(TAG, "pressBack()");
        mDevice.pressBack();
    }

    /**
     * Presses the Home button.
     */
    void pressHome() {
        Log.d(TAG, "pressHome()");
        mDevice.pressHome();
    }

    /**
     * Asserts the save snackbar is not showing.
     */
    void assertSaveNotShowing(int type) throws Exception {
        assertNeverShown("save UI for type " + type, SAVE_UI_SELECTOR, SAVE_NOT_SHOWN_NAPTIME_MS);
    }

    private String getSaveTypeString(int type) {
        final String typeResourceName;
        switch (type) {
            case SAVE_DATA_TYPE_PASSWORD:
                typeResourceName = RESOURCE_STRING_SAVE_TYPE_PASSWORD;
                break;
            case SAVE_DATA_TYPE_ADDRESS:
                typeResourceName = RESOURCE_STRING_SAVE_TYPE_ADDRESS;
                break;
            case SAVE_DATA_TYPE_CREDIT_CARD:
                typeResourceName = RESOURCE_STRING_SAVE_TYPE_CREDIT_CARD;
                break;
            case SAVE_DATA_TYPE_USERNAME:
                typeResourceName = RESOURCE_STRING_SAVE_TYPE_USERNAME;
                break;
            case SAVE_DATA_TYPE_EMAIL_ADDRESS:
                typeResourceName = RESOURCE_STRING_SAVE_TYPE_EMAIL_ADDRESS;
                break;
            default:
                throw new IllegalArgumentException("Unsupported type: " + type);
        }
        return getString(typeResourceName);
    }

    UiObject2 assertSaveShowing(String description, int... types) throws Exception {
        return assertSaveShowing(SaveInfo.NEGATIVE_BUTTON_STYLE_CANCEL, description,
                SAVE_TIMEOUT, types);
    }

    UiObject2 assertSaveShowing(String description, Timeout timeout, int... types)
            throws Exception {
        return assertSaveShowing(SaveInfo.NEGATIVE_BUTTON_STYLE_CANCEL, description, timeout,
                types);
    }

    UiObject2 assertSaveShowing(int negativeButtonStyle, String description,
            int... types) throws Exception {
        return assertSaveShowing(negativeButtonStyle, description, SAVE_TIMEOUT, types);
    }

    UiObject2 assertSaveShowing(int negativeButtonStyle, String description, Timeout timeout,
            int... types) throws Exception {
        final UiObject2 snackbar = waitForObject(SAVE_UI_SELECTOR, timeout);

        final UiObject2 titleView =
                waitForObject(snackbar, By.res("android", RESOURCE_ID_SAVE_TITLE), timeout);
        assertWithMessage("save title (%s) is not shown", RESOURCE_ID_SAVE_TITLE).that(titleView)
                .isNotNull();

        final UiObject2 iconView =
                waitForObject(snackbar, By.res("android", RESOURCE_ID_SAVE_ICON), timeout);
        assertWithMessage("save icon (%s) is not shown", RESOURCE_ID_SAVE_ICON).that(iconView)
                .isNotNull();

        final String actualTitle = titleView.getText();
        Log.d(TAG, "save title: " + actualTitle);

        final String serviceLabel = InstrumentedAutoFillService.getServiceLabel();
        switch (types.length) {
            case 1:
                final String expectedTitle = (types[0] == SAVE_DATA_TYPE_GENERIC)
                        ? Html.fromHtml(getString(RESOURCE_STRING_SAVE_TITLE,
                                serviceLabel), 0).toString()
                        : Html.fromHtml(getString(RESOURCE_STRING_SAVE_TITLE_WITH_TYPE,
                                getSaveTypeString(types[0]), serviceLabel), 0).toString();
                assertThat(actualTitle).isEqualTo(expectedTitle);
                break;
            case 2:
                // We cannot predict the order...
                assertThat(actualTitle).contains(getSaveTypeString(types[0]));
                assertThat(actualTitle).contains(getSaveTypeString(types[1]));
                break;
            case 3:
                // We cannot predict the order...
                assertThat(actualTitle).contains(getSaveTypeString(types[0]));
                assertThat(actualTitle).contains(getSaveTypeString(types[1]));
                assertThat(actualTitle).contains(getSaveTypeString(types[2]));
                break;
            default:
                throw new IllegalArgumentException("Invalid types: " + Arrays.toString(types));
        }

        if (description != null) {
            final UiObject2 saveSubTitle = snackbar.findObject(By.text(description));
            assertWithMessage("save subtitle(%s)", description).that(saveSubTitle).isNotNull();
        }

        final String negativeButtonStringId =
                (negativeButtonStyle == SaveInfo.NEGATIVE_BUTTON_STYLE_REJECT)
                ? RESOURCE_STRING_SAVE_BUTTON_NOT_NOW
                : RESOURCE_STRING_SAVE_BUTTON_NO_THANKS;
        final String expectedNegativeButtonText = getString(negativeButtonStringId).toUpperCase();
        final UiObject2 negativeButton = waitForObject(snackbar,
                By.res("android", RESOURCE_ID_SAVE_BUTTON_NO), timeout);
        assertWithMessage("wrong text on negative button")
                .that(negativeButton.getText().toUpperCase()).isEqualTo(expectedNegativeButtonText);

        final String expectedAccessibilityTitle =
                getString(RESOURCE_STRING_SAVE_SNACKBAR_ACCESSIBILITY_TITLE);
        assertAccessibilityTitle(snackbar, expectedAccessibilityTitle);

        return snackbar;
    }

    /**
     * Taps an option in the save snackbar.
     *
     * @param yesDoIt {@code true} for 'YES', {@code false} for 'NO THANKS'.
     * @param types expected types of save info.
     */
    void saveForAutofill(boolean yesDoIt, int... types) throws Exception {
        final UiObject2 saveSnackBar = assertSaveShowing(
                SaveInfo.NEGATIVE_BUTTON_STYLE_CANCEL, null, types);
        saveForAutofill(saveSnackBar, yesDoIt);
    }

    /**
     * Taps an option in the save snackbar.
     *
     * @param yesDoIt {@code true} for 'YES', {@code false} for 'NO THANKS'.
     * @param types expected types of save info.
     */
    void saveForAutofill(int negativeButtonStyle, boolean yesDoIt, int... types) throws Exception {
        final UiObject2 saveSnackBar = assertSaveShowing(negativeButtonStyle,null, types);
        saveForAutofill(saveSnackBar, yesDoIt);
    }

    /**
     * Taps an option in the save snackbar.
     *
     * @param saveSnackBar Save snackbar, typically obtained through
     *            {@link #assertSaveShowing(int)}.
     * @param yesDoIt {@code true} for 'YES', {@code false} for 'NO THANKS'.
     */
    void saveForAutofill(UiObject2 saveSnackBar, boolean yesDoIt) {
        final String id = yesDoIt ? "autofill_save_yes" : "autofill_save_no";

        final UiObject2 button = saveSnackBar.findObject(By.res("android", id));
        assertWithMessage("save button (%s)", id).that(button).isNotNull();
        button.click();
    }

    /**
     * Gets the AUTOFILL contextual menu by long pressing a text field.
     *
     * <p><b>NOTE:</b> this method should only be called in scenarios where we explicitly want to
     * test the overflow menu. For all other scenarios where we want to test manual autofill, it's
     * better to call {@code AFM.requestAutofill()} directly, because it's less error-prone and
     * faster.
     *
     * @param id resource id of the field.
     */
    UiObject2 getAutofillMenuOption(String id) throws Exception {
        final UiObject2 field = waitForObject(By.res(mPackageName, id));
        // TODO: figure out why obj.longClick() doesn't always work
        field.click(3000);

        final List<UiObject2> menuItems = waitForObjects(
                By.res("android", RESOURCE_ID_CONTEXT_MENUITEM), mDefaultTimeout);
        final String expectedText = getAutofillContextualMenuTitle();
        final StringBuffer menuNames = new StringBuffer();
        for (UiObject2 menuItem : menuItems) {
            final String menuName = menuItem.getText();
            if (menuName.equalsIgnoreCase(expectedText)) {
                return menuItem;
            }
            menuNames.append("'").append(menuName).append("' ");
        }
        throw new RetryableException("no '%s' on '%s'", expectedText, menuNames);
    }

    String getAutofillContextualMenuTitle() {
        return getString(RESOURCE_STRING_AUTOFILL);
    }

    /**
     * Gets a string from the Android resources.
     */
    private String getString(String id) {
        final Resources resources = mContext.getResources();
        final int stringId = resources.getIdentifier(id, "string", "android");
        return resources.getString(stringId);
    }

    /**
     * Gets a string from the Android resources.
     */
    private String getString(String id, Object... formatArgs) {
        final Resources resources = mContext.getResources();
        final int stringId = resources.getIdentifier(id, "string", "android");
        return resources.getString(stringId, formatArgs);
    }

    /**
     * Waits for and returns an object.
     *
     * @param selector {@link BySelector} that identifies the object.
     */
    private UiObject2 waitForObject(BySelector selector) throws Exception {
        return waitForObject(selector, mDefaultTimeout);
    }

    /**
     * Waits for and returns an object.
     *
     * @param parent where to find the object (or {@code null} to use device's root).
     * @param selector {@link BySelector} that identifies the object.
     * @param timeout timeout in ms.
     * @param dumpOnError whether the window hierarchy should be dumped if the object is not found.
     */
    private UiObject2 waitForObject(UiObject2 parent, BySelector selector, Timeout timeout,
            boolean dumpOnError) throws Exception {
        // NOTE: mDevice.wait does not work for the save snackbar, so we need a polling approach.
        try {
            return timeout.run("waitForObject(" + selector + ")", () -> {
                return parent != null
                        ? parent.findObject(selector)
                        : mDevice.findObject(selector);

            });
        } catch (RetryableException e) {
            if (dumpOnError) {
                dumpScreen("waitForObject() for " + selector + "failed");
            }
            throw e;
        }
    }

    private UiObject2 waitForObject(UiObject2 parent, BySelector selector, Timeout timeout)
            throws Exception {
        return waitForObject(parent, selector, timeout, DUMP_ON_ERROR);
    }

    /**
     * Waits for and returns an object.
     *
     * @param selector {@link BySelector} that identifies the object.
     * @param timeout timeout in ms
     */
    private UiObject2 waitForObject(BySelector selector, Timeout timeout) throws Exception {
        return waitForObject(null, selector, timeout);
    }

    /**
     * Execute a Runnable and wait for TYPE_WINDOWS_CHANGED or TYPE_WINDOW_STATE_CHANGED.
     * TODO: No longer need Retry, Refactoring the Timeout (e.g. we probably need two values:
     * one large timeout value that expects window event, one small value that expect no window
     * event)
     */
    public void waitForWindowChange(Runnable runnable, long timeoutMillis) throws TimeoutException {
        mAutoman.executeAndWaitForEvent(runnable, (AccessibilityEvent event) -> {
            switch (event.getEventType()) {
                case AccessibilityEvent.TYPE_WINDOWS_CHANGED:
                case AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED:
                    return true;
            }
            return false;
        }, timeoutMillis);
    }

    /**
     * Waits for and returns a list of objects.
     *
     * @param selector {@link BySelector} that identifies the object.
     * @param timeout timeout in ms
     */
    private List<UiObject2> waitForObjects(BySelector selector, Timeout timeout) throws Exception {
        // NOTE: mDevice.wait does not work for the save snackbar, so we need a polling approach.
        try {
            return timeout.run("waitForObject(" + selector + ")", () -> {
                final List<UiObject2> uiObjects = mDevice.findObjects(selector);
                if (uiObjects != null && !uiObjects.isEmpty()) {
                    return uiObjects;
                }
                return null;

            });

        } catch (RetryableException e) {
            dumpScreen("waitForObjects() for " + selector + "failed");
            throw e;
        }
    }

    private UiObject2 findDatasetPicker(Timeout timeout) throws Exception {
        final UiObject2 picker = waitForObject(DATASET_PICKER_SELECTOR, timeout);

        final String expectedTitle = getString(RESOURCE_STRING_DATASET_PICKER_ACCESSIBILITY_TITLE);
        assertAccessibilityTitle(picker, expectedTitle);

        if (picker != null) {
            mOkToCallAssertNoDatasets = true;
        }

        return picker;
    }

    /**
     * Asserts a given object has the expected accessibility title.
     */
    private void assertAccessibilityTitle(UiObject2 object, String expectedTitle) {
        // TODO: ideally it should get the AccessibilityWindowInfo from the object, but UiAutomator
        // does not expose that.
        for (AccessibilityWindowInfo window : mAutoman.getWindows()) {
            final CharSequence title = window.getTitle();
            if (title != null && title.toString().equals(expectedTitle)) {
                return;
            }
        }
        throw new RetryableException("Title '%s' not found for %s", expectedTitle, object);
    }

    /**
     * Sets the the screen orientation.
     *
     * @param orientation typically {@link #LANDSCAPE} or {@link #PORTRAIT}.
     *
     * @throws RetryableException if value didn't change.
     */
    public void setScreenOrientation(int orientation) throws Exception {
        mAutoman.setRotation(orientation);

        UI_SCREEN_ORIENTATION_TIMEOUT.run("setScreenOrientation(" + orientation + ")", () -> {
            return getScreenOrientation() == orientation ? Boolean.TRUE : null;
        });
    }

    /**
     * Gets the value of the screen orientation.
     *
     * @return typically {@link #LANDSCAPE} or {@link #PORTRAIT}.
     */
    public int getScreenOrientation() {
        return mDevice.getDisplayRotation();
    }

    /**
     * Dumps the current view hierarchy int the output stream.
     */
    public void dumpScreen(String cause) {
        new Exception("dumpScreen(cause=" + cause + ") stacktrace").printStackTrace(System.out);
        try {
            try (ByteArrayOutputStream os = new ByteArrayOutputStream()) {
                mDevice.dumpWindowHierarchy(os);
                os.flush();
                Log.w(TAG, "Dumping window hierarchy because " + cause);
                for (String line : os.toString("UTF-8").split("\n")) {
                    Log.w(TAG, line);
                    // Sleep a little bit to avoid logs being ignored due to spam
                    SystemClock.sleep(100);
                }
            }
        } catch (IOException e) {
            // Just ignore it...
            Log.e(TAG, "exception dumping window hierarchy", e);
            return;
        }
    }

    // TODO(b/74358143): ideally we should take a screenshot limited by the boundaries of the
    // activity window, so external elements (such as the clock) are filtered out and don't cause
    // test flakiness when the contents are compared.
    public Bitmap takeScreenshot() {
        final long before = SystemClock.elapsedRealtime();
        final Bitmap bitmap = mAutoman.takeScreenshot();
        final long delta = SystemClock.elapsedRealtime() - before;
        Log.v(TAG, "Screenshot taken in " + delta + "ms");
        return bitmap;
    }

    /**
     * Asserts the contents of a child element.
     *
     * @param parent parent object
     * @param childId (relative) resource id of the child
     * @param assertion if {@code null}, asserts the child does not exist; otherwise, asserts the
     * child with it.
     */
    public void assertChild(@NonNull UiObject2 parent, @NonNull String childId,
            @Nullable Visitor<UiObject2> assertion) {
        final UiObject2 child = parent.findObject(By.res(mPackageName, childId));
        if (assertion != null) {
            assertWithMessage("Didn't find child with id '%s'", childId).that(child).isNotNull();
            try {
                assertion.visit(child);
            } catch (Throwable t) {
                throw new AssertionError("Error on child '" + childId + "'", t);
            }
        } else {
            assertWithMessage("Shouldn't find child with id '%s'", childId).that(child).isNull();
        }
    }
}
