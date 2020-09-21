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

import static android.autofillservice.cts.InstrumentedAutoFillService.SERVICE_NAME;
import static android.autofillservice.cts.UiBot.PORTRAIT;
import static android.autofillservice.cts.common.ShellHelper.runShellCommand;
import static android.provider.Settings.Secure.AUTOFILL_SERVICE;
import static android.provider.Settings.Secure.USER_SETUP_COMPLETE;
import static android.service.autofill.FillEventHistory.Event.TYPE_AUTHENTICATION_SELECTED;
import static android.service.autofill.FillEventHistory.Event.TYPE_CONTEXT_COMMITTED;
import static android.service.autofill.FillEventHistory.Event.TYPE_DATASET_AUTHENTICATION_SELECTED;
import static android.service.autofill.FillEventHistory.Event.TYPE_DATASET_SELECTED;
import static android.service.autofill.FillEventHistory.Event.TYPE_SAVE_SHOWN;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.app.Activity;
import android.app.assist.AssistStructure;
import android.app.assist.AssistStructure.ViewNode;
import android.app.assist.AssistStructure.WindowNode;
import android.autofillservice.cts.common.SettingsHelper;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.icu.util.Calendar;
import android.os.Bundle;
import android.os.Environment;
import android.service.autofill.FieldClassification;
import android.service.autofill.FieldClassification.Match;
import android.service.autofill.FillContext;
import android.service.autofill.FillEventHistory;
import android.support.test.InstrumentationRegistry;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStructure.HtmlInfo;
import android.view.autofill.AutofillId;
import android.view.autofill.AutofillManager.AutofillCallback;
import android.view.autofill.AutofillValue;
import android.webkit.WebView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.compatibility.common.util.BitmapUtils;
import com.android.compatibility.common.util.RequiredFeatureRule;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.function.Function;

/**
 * Helper for common funcionalities.
 */
final class Helper {

    static final String TAG = "AutoFillCtsHelper";

    static final boolean VERBOSE = false;

    static final String MY_PACKAGE = "android.autofillservice.cts";

    static final String ID_USERNAME_LABEL = "username_label";
    static final String ID_USERNAME = "username";
    static final String ID_PASSWORD_LABEL = "password_label";
    static final String ID_PASSWORD = "password";
    static final String ID_LOGIN = "login";
    static final String ID_OUTPUT = "output";
    static final String ID_STATIC_TEXT = "static_text";

    public static final String NULL_DATASET_ID = null;

    /**
     * Can be used in cases where the autofill values is required by irrelevant (like adding a
     * value to an authenticated dataset).
     */
    public static final String UNUSED_AUTOFILL_VALUE = null;

    private static final String CMD_LIST_SESSIONS = "cmd autofill list sessions";

    private static final String ACCELLEROMETER_CHANGE =
            "content insert --uri content://settings/system --bind name:s:accelerometer_rotation "
                    + "--bind value:i:%d";

    private static final String LOCAL_DIRECTORY = Environment.getExternalStorageDirectory()
            + "/CtsAutoFillServiceTestCases";

    /**
     * Helper interface used to filter nodes.
     *
     * @param <T> node type
     */
    interface NodeFilter<T> {
        /**
         * Returns whether the node passes the filter for such given id.
         */
        boolean matches(T node, Object id);
    }

    private static final NodeFilter<ViewNode> RESOURCE_ID_FILTER = (node, id) -> {
        return id.equals(node.getIdEntry());
    };

    private static final NodeFilter<ViewNode> AUTOFILL_ID_FILTER = (node, id) -> {
        return id.equals(node.getAutofillId());
    };

    private static final NodeFilter<ViewNode> HTML_NAME_FILTER = (node, id) -> {
        return id.equals(getHtmlName(node));
    };

    private static final NodeFilter<ViewNode> HTML_NAME_OR_RESOURCE_ID_FILTER = (node, id) -> {
        return id.equals(getHtmlName(node)) || id.equals(node.getIdEntry());
    };

    private static final NodeFilter<ViewNode> TEXT_FILTER = (node, id) -> {
        return id.equals(node.getText());
    };

    private static final NodeFilter<ViewNode> AUTOFILL_HINT_FILTER = (node, id) -> {
        return hasHint(node.getAutofillHints(), id);
    };

    private static final NodeFilter<ViewNode> WEBVIEW_FORM_FILTER = (node, id) -> {
        final String className = node.getClassName();
        if (!className.equals("android.webkit.WebView")) return false;

        final HtmlInfo htmlInfo = assertHasHtmlTag(node, "form");
        final String formName = getAttributeValue(htmlInfo, "name");
        return id.equals(formName);
    };

    private static final NodeFilter<View> AUTOFILL_HINT_VIEW_FILTER = (view, id) -> {
        return hasHint(view.getAutofillHints(), id);
    };

    /**
     * Dump the assist structure on logcat.
     */
    static void dumpStructure(String message, AssistStructure structure) {
        final StringBuffer buffer = new StringBuffer(message)
                .append(": component=")
                .append(structure.getActivityComponent());
        final int nodes = structure.getWindowNodeCount();
        for (int i = 0; i < nodes; i++) {
            final WindowNode windowNode = structure.getWindowNodeAt(i);
            dump(buffer, windowNode.getRootViewNode(), " ", 0);
        }
        Log.i(TAG, buffer.toString());
    }

    /**
     * Dump the contexts on logcat.
     */
    static void dumpStructure(String message, List<FillContext> contexts) {
        for (FillContext context : contexts) {
            dumpStructure(message, context.getStructure());
        }
    }

    /**
     * Dumps the state of the autofill service on logcat.
     */
    static void dumpAutofillService() {
        Log.i(TAG, "dumpsys autofill\n\n" + runShellCommand("dumpsys autofill"));
    }

    /**
     * Sets whether the user completed the initial setup.
     */
    static void setUserComplete(Context context, boolean complete) {
        SettingsHelper.syncSet(context, USER_SETUP_COMPLETE, complete ? "1" : null);
    }

    private static void dump(StringBuffer buffer, ViewNode node, String prefix, int childId) {
        final int childrenSize = node.getChildCount();
        buffer.append("\n").append(prefix)
            .append('#').append(childId).append(':')
            .append("resId=").append(node.getIdEntry())
            .append(" class=").append(node.getClassName())
            .append(" text=").append(node.getText())
            .append(" class=").append(node.getClassName())
            .append(" webDomain=").append(node.getWebDomain())
            .append(" #children=").append(childrenSize);

        buffer.append("\n").append(prefix)
            .append("   afId=").append(node.getAutofillId())
            .append(" afType=").append(node.getAutofillType())
            .append(" afValue=").append(node.getAutofillValue())
            .append(" checked=").append(node.isChecked())
            .append(" focused=").append(node.isFocused());

        final HtmlInfo htmlInfo = node.getHtmlInfo();
        if (htmlInfo != null) {
            buffer.append("\nHtmlInfo: tag=").append(htmlInfo.getTag())
                .append(", attrs: ").append(htmlInfo.getAttributes());
        }

        prefix += " ";
        if (childrenSize > 0) {
            for (int i = 0; i < childrenSize; i++) {
                dump(buffer, node.getChildAt(i), prefix, i);
            }
        }
    }

    /**
     * Gets a node if it matches the filter criteria for the given id.
     */
    static ViewNode findNodeByFilter(@NonNull AssistStructure structure, @NonNull Object id,
            @NonNull NodeFilter<ViewNode> filter) {
        Log.v(TAG, "Parsing request for activity " + structure.getActivityComponent());
        final int nodes = structure.getWindowNodeCount();
        for (int i = 0; i < nodes; i++) {
            final WindowNode windowNode = structure.getWindowNodeAt(i);
            final ViewNode rootNode = windowNode.getRootViewNode();
            final ViewNode node = findNodeByFilter(rootNode, id, filter);
            if (node != null) {
                return node;
            }
        }
        return null;
    }

    /**
     * Gets a node if it matches the filter criteria for the given id.
     */
    static ViewNode findNodeByFilter(@NonNull List<FillContext> contexts, @NonNull Object id,
            @NonNull NodeFilter<ViewNode> filter) {
        for (FillContext context : contexts) {
            ViewNode node = findNodeByFilter(context.getStructure(), id, filter);
            if (node != null) {
                return node;
            }
        }
        return null;
    }

    /**
     * Gets a node if it matches the filter criteria for the given id.
     */
    static ViewNode findNodeByFilter(@NonNull ViewNode node, @NonNull Object id,
            @NonNull NodeFilter<ViewNode> filter) {
        if (filter.matches(node, id)) {
            return node;
        }
        final int childrenSize = node.getChildCount();
        if (childrenSize > 0) {
            for (int i = 0; i < childrenSize; i++) {
                final ViewNode found = findNodeByFilter(node.getChildAt(i), id, filter);
                if (found != null) {
                    return found;
                }
            }
        }
        return null;
    }

    /**
     * Gets a node given its Android resource id, or {@code null} if not found.
     */
    static ViewNode findNodeByResourceId(AssistStructure structure, String resourceId) {
        return findNodeByFilter(structure, resourceId, RESOURCE_ID_FILTER);
    }

    /**
     * Gets a node given its Android resource id, or {@code null} if not found.
     */
    static ViewNode findNodeByResourceId(List<FillContext> contexts, String resourceId) {
        return findNodeByFilter(contexts, resourceId, RESOURCE_ID_FILTER);
    }

    /**
     * Gets a node given its Android resource id, or {@code null} if not found.
     */
    static ViewNode findNodeByResourceId(ViewNode node, String resourceId) {
        return findNodeByFilter(node, resourceId, RESOURCE_ID_FILTER);
    }

    /**
     * Gets a node given the name of its HTML INPUT tag, or {@code null} if not found.
     */
    static ViewNode findNodeByHtmlName(AssistStructure structure, String htmlName) {
        return findNodeByFilter(structure, htmlName, HTML_NAME_FILTER);
    }

    /**
     * Gets a node given the name of its HTML INPUT tag, or {@code null} if not found.
     */
    static ViewNode findNodeByHtmlName(List<FillContext> contexts, String htmlName) {
        return findNodeByFilter(contexts, htmlName, HTML_NAME_FILTER);
    }

    /**
     * Gets a node given the name of its HTML INPUT tag, or {@code null} if not found.
     */
    static ViewNode findNodeByHtmlName(ViewNode node, String htmlName) {
        return findNodeByFilter(node, htmlName, HTML_NAME_FILTER);
    }

    /**
     * Gets a node given the value of its (single) autofill hint property, or {@code null} if not
     * found.
     */
    static ViewNode findNodeByAutofillHint(ViewNode node, String hint) {
        return findNodeByFilter(node, hint, AUTOFILL_HINT_FILTER);
    }

    /**
     * Gets a node given the name of its HTML INPUT tag or Android resoirce id, or {@code null} if
     * not found.
     */
    static ViewNode findNodeByHtmlNameOrResourceId(List<FillContext> contexts, String id) {
        return findNodeByFilter(contexts, id, HTML_NAME_OR_RESOURCE_ID_FILTER);
    }

    /**
     * Gets the {@code name} attribute of a node representing an HTML input tag.
     */
    @Nullable
    static String getHtmlName(@NonNull ViewNode node) {
        final HtmlInfo htmlInfo = node.getHtmlInfo();
        if (htmlInfo == null) {
            return null;
        }
        final String tag = htmlInfo.getTag();
        if (!"input".equals(tag)) {
            Log.w(TAG, "getHtmlName(): invalid tag (" + tag + ") on " + htmlInfo);
            return null;
        }
        for (Pair<String, String> attr : htmlInfo.getAttributes()) {
            if ("name".equals(attr.first)) {
                return attr.second;
            }
        }
        Log.w(TAG, "getHtmlName(): no 'name' attribute on " + htmlInfo);
        return null;
    }

    /**
     * Gets a node given its expected text, or {@code null} if not found.
     */
    static ViewNode findNodeByText(AssistStructure structure, String text) {
        return findNodeByFilter(structure, text, TEXT_FILTER);
    }

    /**
     * Gets a node given its expected text, or {@code null} if not found.
     */
    static ViewNode findNodeByText(ViewNode node, String text) {
        return findNodeByFilter(node, text, TEXT_FILTER);
    }

    /**
     * Gets a view that contains the an autofill hint, or {@code null} if not found.
     */
    static View findViewByAutofillHint(Activity activity, String hint) {
        final View rootView = activity.getWindow().getDecorView().getRootView();
        return findViewByAutofillHint(rootView, hint);
    }

    /**
     * Gets a view (or a descendant of it) that contains the an autofill hint, or {@code null} if
     * not found.
     */
    static View findViewByAutofillHint(View view, String hint) {
        if (AUTOFILL_HINT_VIEW_FILTER.matches(view, hint)) return view;
        if ((view instanceof ViewGroup)) {
            final ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) {
                final View child = findViewByAutofillHint(group.getChildAt(i), hint);
                if (child != null) return child;
            }
        }
        return null;
    }

    /**
     * Gets a view (or a descendant of it) that has the given {@code id}, or {@code null} if
     * not found.
     */
    static ViewNode findNodeByAutofillId(AssistStructure structure, AutofillId id) {
        return findNodeByFilter(structure, id, AUTOFILL_ID_FILTER);
    }

    /**
     * Asserts a text-based node is sanitized.
     */
    static void assertTextIsSanitized(ViewNode node) {
        final CharSequence text = node.getText();
        final String resourceId = node.getIdEntry();
        if (!TextUtils.isEmpty(text)) {
            throw new AssertionError("text on sanitized field " + resourceId + ": " + text);
        }

        assertNotFromResources(node);
        assertNodeHasNoAutofillValue(node);
    }

    private static void assertNotFromResources(ViewNode node) {
        assertThat(node.getTextIdEntry()).isNull();
    }

    static void assertNodeHasNoAutofillValue(ViewNode node) {
        final AutofillValue value = node.getAutofillValue();
        if (value != null) {
            final String text = value.isText() ? value.getTextValue().toString() : "N/A";
            throw new AssertionError("node has value: " + value + " text=" + text);
        }
    }

    /**
     * Asserts the contents of a text-based node that is also auto-fillable.
     */
    static void assertTextOnly(ViewNode node, String expectedValue) {
        assertText(node, expectedValue, false);
        assertNotFromResources(node);
    }

    /**
     * Asserts the contents of a text-based node that is also auto-fillable.
     */
    static void assertTextOnly(AssistStructure structure, String resourceId, String expectedValue) {
        final ViewNode node = findNodeByResourceId(structure, resourceId);
        assertText(node, expectedValue, false);
        assertNotFromResources(node);
    }

    /**
     * Asserts the contents of a text-based node that is also auto-fillable.
     */
    static void assertTextAndValue(ViewNode node, String expectedValue) {
        assertText(node, expectedValue, true);
        assertNotFromResources(node);
    }

    /**
     * Asserts a text-based node exists and verify its values.
     */
    static ViewNode assertTextAndValue(AssistStructure structure, String resourceId,
            String expectedValue) {
        final ViewNode node = findNodeByResourceId(structure, resourceId);
        assertTextAndValue(node, expectedValue);
        return node;
    }

    /**
     * Asserts a text-based node exists and is sanitized.
     */
    static ViewNode assertValue(AssistStructure structure, String resourceId,
            String expectedValue) {
        final ViewNode node = findNodeByResourceId(structure, resourceId);
        assertTextValue(node, expectedValue);
        return node;
    }

    /**
     * Asserts the values of a text-based node whose string come from resoruces.
     */
    static ViewNode assertTextFromResouces(AssistStructure structure, String resourceId,
            String expectedValue, boolean isAutofillable, String expectedTextIdEntry) {
        final ViewNode node = findNodeByResourceId(structure, resourceId);
        assertText(node, expectedValue, isAutofillable);
        assertThat(node.getTextIdEntry()).isEqualTo(expectedTextIdEntry);
        return node;
    }

    private static void assertText(ViewNode node, String expectedValue, boolean isAutofillable) {
        assertWithMessage("wrong text on %s", node.getAutofillId()).that(node.getText().toString())
                .isEqualTo(expectedValue);
        final AutofillValue value = node.getAutofillValue();
        final AutofillId id = node.getAutofillId();
        if (isAutofillable) {
            assertWithMessage("null auto-fill value on %s", id).that(value).isNotNull();
            assertWithMessage("wrong auto-fill value on %s", id)
                    .that(value.getTextValue().toString()).isEqualTo(expectedValue);
        } else {
            assertWithMessage("node %s should not have AutofillValue", id).that(value).isNull();
        }
    }

    /**
     * Asserts the auto-fill value of a text-based node.
     */
    static ViewNode assertTextValue(ViewNode node, String expectedText) {
        final AutofillValue value = node.getAutofillValue();
        final AutofillId id = node.getAutofillId();
        assertWithMessage("null autofill value on %s", id).that(value).isNotNull();
        assertWithMessage("wrong autofill type on %s", id).that(value.isText()).isTrue();
        assertWithMessage("wrong autofill value on %s", id).that(value.getTextValue().toString())
                .isEqualTo(expectedText);
        return node;
    }

    /**
     * Asserts the auto-fill value of a list-based node.
     */
    static ViewNode assertListValue(ViewNode node, int expectedIndex) {
        final AutofillValue value = node.getAutofillValue();
        final AutofillId id = node.getAutofillId();
        assertWithMessage("null autofill value on %s", id).that(value).isNotNull();
        assertWithMessage("wrong autofill type on %s", id).that(value.isList()).isTrue();
        assertWithMessage("wrong autofill value on %s", id).that(value.getListValue())
                .isEqualTo(expectedIndex);
        return node;
    }

    /**
     * Asserts the auto-fill value of a toggle-based node.
     */
    static void assertToggleValue(ViewNode node, boolean expectedToggle) {
        final AutofillValue value = node.getAutofillValue();
        final AutofillId id = node.getAutofillId();
        assertWithMessage("null autofill value on %s", id).that(value).isNotNull();
        assertWithMessage("wrong autofill type on %s", id).that(value.isToggle()).isTrue();
        assertWithMessage("wrong autofill value on %s", id).that(value.getToggleValue())
                .isEqualTo(expectedToggle);
    }

    /**
     * Asserts the auto-fill value of a date-based node.
     */
    static void assertDateValue(Object object, AutofillValue value, int year, int month, int day) {
        assertWithMessage("null autofill value on %s", object).that(value).isNotNull();
        assertWithMessage("wrong autofill type on %s", object).that(value.isDate()).isTrue();

        final Calendar cal = Calendar.getInstance();
        cal.setTimeInMillis(value.getDateValue());

        assertWithMessage("Wrong year on AutofillValue %s", value)
            .that(cal.get(Calendar.YEAR)).isEqualTo(year);
        assertWithMessage("Wrong month on AutofillValue %s", value)
            .that(cal.get(Calendar.MONTH)).isEqualTo(month);
        assertWithMessage("Wrong day on AutofillValue %s", value)
             .that(cal.get(Calendar.DAY_OF_MONTH)).isEqualTo(day);
    }

    /**
     * Asserts the auto-fill value of a date-based node.
     */
    static void assertDateValue(ViewNode node, int year, int month, int day) {
        assertDateValue(node, node.getAutofillValue(), year, month, day);
    }

    /**
     * Asserts the auto-fill value of a date-based view.
     */
    static void assertDateValue(View view, int year, int month, int day) {
        assertDateValue(view, view.getAutofillValue(), year, month, day);
    }

    /**
     * Asserts the auto-fill value of a time-based node.
     */
    private static void assertTimeValue(Object object, AutofillValue value, int hour, int minute) {
        assertWithMessage("null autofill value on %s", object).that(value).isNotNull();
        assertWithMessage("wrong autofill type on %s", object).that(value.isDate()).isTrue();

        final Calendar cal = Calendar.getInstance();
        cal.setTimeInMillis(value.getDateValue());

        assertWithMessage("Wrong hour on AutofillValue %s", value)
            .that(cal.get(Calendar.HOUR_OF_DAY)).isEqualTo(hour);
        assertWithMessage("Wrong minute on AutofillValue %s", value)
            .that(cal.get(Calendar.MINUTE)).isEqualTo(minute);
    }

    /**
     * Asserts the auto-fill value of a time-based node.
     */
    static void assertTimeValue(ViewNode node, int hour, int minute) {
        assertTimeValue(node, node.getAutofillValue(), hour, minute);
    }

    /**
     * Asserts the auto-fill value of a time-based view.
     */
    static void assertTimeValue(View view, int hour, int minute) {
        assertTimeValue(view, view.getAutofillValue(), hour, minute);
    }

    /**
     * Asserts a text-based node exists and is sanitized.
     */
    static ViewNode assertTextIsSanitized(AssistStructure structure, String resourceId) {
        final ViewNode node = findNodeByResourceId(structure, resourceId);
        assertWithMessage("no ViewNode with id %s", resourceId).that(node).isNotNull();
        assertTextIsSanitized(node);
        return node;
    }

    /**
     * Asserts a list-based node exists and is sanitized.
     */
    static void assertListValueIsSanitized(AssistStructure structure, String resourceId) {
        final ViewNode node = findNodeByResourceId(structure, resourceId);
        assertWithMessage("no ViewNode with id %s", resourceId).that(node).isNotNull();
        assertTextIsSanitized(node);
    }

    /**
     * Asserts a toggle node exists and is sanitized.
     */
    static void assertToggleIsSanitized(AssistStructure structure, String resourceId) {
        final ViewNode node = findNodeByResourceId(structure, resourceId);
        assertNodeHasNoAutofillValue(node);
        assertWithMessage("ViewNode %s should not be checked", resourceId).that(node.isChecked())
                .isFalse();
    }

    /**
     * Asserts a node exists and has the {@code expected} number of children.
     */
    static void assertNumberOfChildren(AssistStructure structure, String resourceId, int expected) {
        final ViewNode node = findNodeByResourceId(structure, resourceId);
        final int actual = node.getChildCount();
        if (actual != expected) {
            dumpStructure("assertNumberOfChildren()", structure);
            throw new AssertionError("assertNumberOfChildren() for " + resourceId
                    + " failed: expected " + expected + ", got " + actual);
        }
    }

    /**
     * Asserts the number of children in the Assist structure.
     */
    static void assertNumberOfChildren(AssistStructure structure, int expected) {
        assertWithMessage("wrong number of nodes").that(structure.getWindowNodeCount())
                .isEqualTo(1);
        final int actual = getNumberNodes(structure);
        if (actual != expected) {
            dumpStructure("assertNumberOfChildren()", structure);
            throw new AssertionError("assertNumberOfChildren() for structure failed: expected "
                    + expected + ", got " + actual);
        }
    }

    /**
     * Gets the total number of nodes in an structure.
     */
    static int getNumberNodes(AssistStructure structure) {
        int count = 0;
        final int nodes = structure.getWindowNodeCount();
        for (int i = 0; i < nodes; i++) {
            final WindowNode windowNode = structure.getWindowNodeAt(i);
            final ViewNode rootNode = windowNode.getRootViewNode();
            count += getNumberNodes(rootNode);
        }
        return count;
    }

    /**
     * Gets the total number of nodes in an node, including all descendants and the node itself.
     */
    static int getNumberNodes(ViewNode node) {
        int count = 1;
        final int childrenSize = node.getChildCount();
        if (childrenSize > 0) {
            for (int i = 0; i < childrenSize; i++) {
                count += getNumberNodes(node.getChildAt(i));
            }
        }
        return count;
    }

    /**
     * Creates an array of {@link AutofillId} mapped from the {@code structure} nodes with the given
     * {@code resourceIds}.
     */
    static AutofillId[] getAutofillIds(Function<String, ViewNode> nodeResolver,
            String[] resourceIds) {
        if (resourceIds == null) return null;

        final AutofillId[] requiredIds = new AutofillId[resourceIds.length];
        for (int i = 0; i < resourceIds.length; i++) {
            final String resourceId = resourceIds[i];
            final ViewNode node = nodeResolver.apply(resourceId);
            if (node == null) {
                throw new AssertionError("No node with savable resourceId " + resourceId);
            }
            requiredIds[i] = node.getAutofillId();

        }
        return requiredIds;
    }

    /**
     * Prevents the screen to rotate by itself
     */
    public static void disableAutoRotation(UiBot uiBot) throws Exception {
        runShellCommand(ACCELLEROMETER_CHANGE, 0);
        uiBot.setScreenOrientation(PORTRAIT);
    }

    /**
     * Allows the screen to rotate by itself
     */
    public static void allowAutoRotation() {
        runShellCommand(ACCELLEROMETER_CHANGE, 1);
    }

    /**
     * Gets the maximum number of partitions per session.
     */
    public static int getMaxPartitions() {
        return Integer.parseInt(runShellCommand("cmd autofill get max_partitions"));
    }

    /**
     * Sets the maximum number of partitions per session.
     */
    public static void setMaxPartitions(int value) {
        runShellCommand("cmd autofill set max_partitions %d", value);
        assertThat(getMaxPartitions()).isEqualTo(value);
    }

    /**
     * Checks if device supports the Autofill feature.
     */
    public static boolean hasAutofillFeature() {
        return RequiredFeatureRule.hasFeature(PackageManager.FEATURE_AUTOFILL);
    }

    /**
     * Checks if autofill window is fullscreen, see com.android.server.autofill.ui.FillUi.
     */
    public static boolean isAutofillWindowFullScreen(Context context) {
        return context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK);
    }

    /**
     * Checks if screen orientation can be changed.
     */
    public static boolean isRotationSupported(Context context) {
        return !context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK);
    }

    /**
     * Uses Shell command to get the Autofill logging level.
     */
    public static String getLoggingLevel() {
        return runShellCommand("cmd autofill get log_level");
    }

    /**
     * Uses Shell command to set the Autofill logging level.
     */
    public static void setLoggingLevel(String level) {
        runShellCommand("cmd autofill set log_level %s", level);
    }

    /**
     * Uses Settings to enable the given autofill service for the default user, and checks the
     * value was properly check, throwing an exception if it was not.
     */
    public static void enableAutofillService(@NonNull Context context,
            @NonNull String serviceName) {
        if (isAutofillServiceEnabled(serviceName)) return;

        SettingsHelper.syncSet(context, AUTOFILL_SERVICE, serviceName);
    }

    /**
     * Uses Settings to disable the given autofill service for the default user, and checks the
     * value was properly check, throwing an exception if it was not.
     */
    public static void disableAutofillService(@NonNull Context context,
            @NonNull String serviceName) {
        if (!isAutofillServiceEnabled(serviceName)) return;

        SettingsHelper.syncDelete(context, AUTOFILL_SERVICE);
    }

    /**
     * Checks whether the given service is set as the autofill service for the default user.
     */
    private static boolean isAutofillServiceEnabled(@NonNull String serviceName) {
        final String actualName = SettingsHelper.get(AUTOFILL_SERVICE);
        return serviceName.equals(actualName);
    }

    /**
     * Asserts whether the given service is enabled as the autofill service for the default user.
     */
    public static void assertAutofillServiceStatus(@NonNull String serviceName, boolean enabled) {
        final String actual = SettingsHelper.get(AUTOFILL_SERVICE);
        final String expected = enabled ? serviceName : "null";
        assertWithMessage("Invalid value for secure setting %s", AUTOFILL_SERVICE)
                .that(actual).isEqualTo(expected);
    }

    /**
     * Asserts that there is a pending session for the given package.
     */
    public static void assertHasSessions(String packageName) {
        final String result = runShellCommand(CMD_LIST_SESSIONS);
        assertThat(result).contains(packageName);
    }

    /**
     * Gets the instrumentation context.
     */
    public static Context getContext() {
        return InstrumentationRegistry.getInstrumentation().getContext();
    }

    /**
     * Cleans up the autofill state; should be called before pretty much any test.
     */
    public static void preTestCleanup() {
        if (!hasAutofillFeature()) return;

        Log.d(TAG, "preTestCleanup()");

        disableAutofillService(getContext(), SERVICE_NAME);
        InstrumentedAutoFillService.setIgnoreUnexpectedRequests(true);

        InstrumentedAutoFillService.resetStaticState();
        AuthenticationActivity.resetStaticState();
    }

    /**
     * Asserts the node has an {@code HTMLInfo} property, with the given tag.
     */
    public static HtmlInfo assertHasHtmlTag(ViewNode node, String expectedTag) {
        final HtmlInfo info = node.getHtmlInfo();
        assertWithMessage("node doesn't have htmlInfo").that(info).isNotNull();
        assertWithMessage("wrong tag").that(info.getTag()).isEqualTo(expectedTag);
        return info;
    }

    /**
     * Gets the value of an {@code HTMLInfo} attribute.
     */
    @Nullable
    public static String getAttributeValue(HtmlInfo info, String attribute) {
        for (Pair<String, String> pair : info.getAttributes()) {
            if (pair.first.equals(attribute)) {
                return pair.second;
            }
        }
        return null;
    }

    /**
     * Asserts a {@code HTMLInfo} has an attribute with a given value.
     */
    public static void assertHasAttribute(HtmlInfo info, String attribute, String expectedValue) {
        final String actualValue = getAttributeValue(info, attribute);
        assertWithMessage("Attribute %s not found", attribute).that(actualValue).isNotNull();
        assertWithMessage("Wrong value for Attribute %s", attribute)
            .that(actualValue).isEqualTo(expectedValue);
    }

    /**
     * Finds a {@link WebView} node given its expected form name.
     */
    public static ViewNode findWebViewNodeByFormName(AssistStructure structure, String formName) {
        return findNodeByFilter(structure, formName, WEBVIEW_FORM_FILTER);
    }

    private static void assertClientState(Object container, Bundle clientState,
            String key, String value) {
        assertWithMessage("'%s' should have client state", container)
            .that(clientState).isNotNull();
        assertWithMessage("Wrong number of client state extras on '%s'", container)
            .that(clientState.keySet().size()).isEqualTo(1);
        assertWithMessage("Wrong value for client state key (%s) on '%s'", key, container)
            .that(clientState.getString(key)).isEqualTo(value);
    }

    /**
     * Asserts the content of a {@link FillEventHistory#getClientState()}.
     *
     * @param history event to be asserted
     * @param key the only key expected in the client state bundle
     * @param value the only value expected in the client state bundle
     */
    @SuppressWarnings("javadoc")
    public static void assertDeprecatedClientState(@NonNull FillEventHistory history,
            @NonNull String key, @NonNull String value) {
        assertThat(history).isNotNull();
        @SuppressWarnings("deprecation")
        final Bundle clientState = history.getClientState();
        assertClientState(history, clientState, key, value);
    }

    /**
     * Asserts the {@link FillEventHistory#getClientState()} is not set.
     *
     * @param history event to be asserted
     */
    @SuppressWarnings("javadoc")
    public static void assertNoDeprecatedClientState(@NonNull FillEventHistory history) {
        assertThat(history).isNotNull();
        @SuppressWarnings("deprecation")
        final Bundle clientState = history.getClientState();
        assertWithMessage("History '%s' should not have client state", history)
             .that(clientState).isNull();
    }

    /**
     * Asserts the content of a {@link android.service.autofill.FillEventHistory.Event}.
     *
     * @param event event to be asserted
     * @param eventType expected type
     * @param datasetId dataset set id expected in the event
     * @param key the only key expected in the client state bundle (or {@code null} if it shouldn't
     * have client state)
     * @param value the only value expected in the client state bundle (or {@code null} if it
     * shouldn't have client state)
     * @param fieldClassificationResults expected results when asserting field classification
     */
    private static void assertFillEvent(@NonNull FillEventHistory.Event event,
            int eventType, @Nullable String datasetId,
            @Nullable String key, @Nullable String value,
            @Nullable FieldClassificationResult[] fieldClassificationResults) {
        assertThat(event).isNotNull();
        assertWithMessage("Wrong type for %s", event).that(event.getType()).isEqualTo(eventType);
        if (datasetId == null) {
            assertWithMessage("Event %s should not have dataset id", event)
                .that(event.getDatasetId()).isNull();
        } else {
            assertWithMessage("Wrong dataset id for %s", event)
                .that(event.getDatasetId()).isEqualTo(datasetId);
        }
        final Bundle clientState = event.getClientState();
        if (key == null) {
            assertWithMessage("Event '%s' should not have client state", event)
                .that(clientState).isNull();
        } else {
            assertClientState(event, clientState, key, value);
        }
        assertWithMessage("Event '%s' should not have selected datasets", event)
                .that(event.getSelectedDatasetIds()).isEmpty();
        assertWithMessage("Event '%s' should not have ignored datasets", event)
                .that(event.getIgnoredDatasetIds()).isEmpty();
        assertWithMessage("Event '%s' should not have changed fields", event)
                .that(event.getChangedFields()).isEmpty();
        assertWithMessage("Event '%s' should not have manually-entered fields", event)
                .that(event.getManuallyEnteredField()).isEmpty();
        final Map<AutofillId, FieldClassification> detectedFields = event.getFieldsClassification();
        if (fieldClassificationResults == null) {
            assertThat(detectedFields).isEmpty();
        } else {
            assertThat(detectedFields).hasSize(fieldClassificationResults.length);
            int i = 0;
            for (Entry<AutofillId, FieldClassification> entry : detectedFields.entrySet()) {
                assertMatches(i, entry, fieldClassificationResults[i]);
                i++;
            }
        }
    }

    private static void assertMatches(int i, Entry<AutofillId, FieldClassification> actualResult,
            FieldClassificationResult expectedResult) {
        assertWithMessage("Wrong field id at index %s", i).that(actualResult.getKey())
                .isEqualTo(expectedResult.id);
        final List<Match> matches = actualResult.getValue().getMatches();
        assertWithMessage("Wrong number of matches: " + matches).that(matches.size())
                .isEqualTo(expectedResult.remoteIds.length);
        for (int j = 0; j < matches.size(); j++) {
            final Match match = matches.get(j);
            assertWithMessage("Wrong categoryId at (%s, %s): %s", i, j, match)
                .that(match.getCategoryId()).isEqualTo(expectedResult.remoteIds[j]);
            assertWithMessage("Wrong score at (%s, %s): %s", i, j, match)
                .that(match.getScore()).isWithin(0.01f).of(expectedResult.scores[j]);
        }
    }

    /**
     * Asserts the content of a
     * {@link android.service.autofill.FillEventHistory.Event#TYPE_DATASET_SELECTED} event.
     *
     * @param event event to be asserted
     * @param datasetId dataset set id expected in the event
     */
    public static void assertFillEventForDatasetSelected(@NonNull FillEventHistory.Event event,
            @Nullable String datasetId) {
        assertFillEvent(event, TYPE_DATASET_SELECTED, datasetId, null, null, null);
    }

    /**
     * Asserts the content of a
     * {@link android.service.autofill.FillEventHistory.Event#TYPE_DATASET_SELECTED} event.
     *
     * @param event event to be asserted
     * @param datasetId dataset set id expected in the event
     * @param key the only key expected in the client state bundle
     * @param value the only value expected in the client state bundle
     */
    public static void assertFillEventForDatasetSelected(@NonNull FillEventHistory.Event event,
            @Nullable String datasetId, @Nullable String key, @Nullable String value) {
        assertFillEvent(event, TYPE_DATASET_SELECTED, datasetId, key, value, null);
    }

    /**
     * Asserts the content of a
     * {@link android.service.autofill.FillEventHistory.Event#TYPE_SAVE_SHOWN} event.
     *
     * @param event event to be asserted
     * @param datasetId dataset set id expected in the event
     * @param key the only key expected in the client state bundle
     * @param value the only value expected in the client state bundle
     */
    public static void assertFillEventForSaveShown(@NonNull FillEventHistory.Event event,
            @NonNull String datasetId, @NonNull String key, @NonNull String value) {
        assertFillEvent(event, TYPE_SAVE_SHOWN, datasetId, key, value, null);
    }

    /**
     * Asserts the content of a
     * {@link android.service.autofill.FillEventHistory.Event#TYPE_SAVE_SHOWN} event.
     *
     * @param event event to be asserted
     * @param datasetId dataset set id expected in the event
     */
    public static void assertFillEventForSaveShown(@NonNull FillEventHistory.Event event,
            @NonNull String datasetId) {
        assertFillEvent(event, TYPE_SAVE_SHOWN, datasetId, null, null, null);
    }

    /**
     * Asserts the content of a
     * {@link android.service.autofill.FillEventHistory.Event#TYPE_DATASET_AUTHENTICATION_SELECTED}
     * event.
     *
     * @param event event to be asserted
     * @param datasetId dataset set id expected in the event
     * @param key the only key expected in the client state bundle
     * @param value the only value expected in the client state bundle
     */
    public static void assertFillEventForDatasetAuthenticationSelected(
            @NonNull FillEventHistory.Event event,
            @Nullable String datasetId, @NonNull String key, @NonNull String value) {
        assertFillEvent(event, TYPE_DATASET_AUTHENTICATION_SELECTED, datasetId, key, value, null);
    }

    /**
     * Asserts the content of a
     * {@link android.service.autofill.FillEventHistory.Event#TYPE_AUTHENTICATION_SELECTED} event.
     *
     * @param event event to be asserted
     * @param datasetId dataset set id expected in the event
     * @param key the only key expected in the client state bundle
     * @param value the only value expected in the client state bundle
     */
    public static void assertFillEventForAuthenticationSelected(
            @NonNull FillEventHistory.Event event,
            @Nullable String datasetId, @NonNull String key, @NonNull String value) {
        assertFillEvent(event, TYPE_AUTHENTICATION_SELECTED, datasetId, key, value, null);
    }

    public static void assertFillEventForFieldsClassification(@NonNull FillEventHistory.Event event,
            @NonNull AutofillId fieldId, @NonNull String remoteId, float score) {
        assertFillEvent(event, TYPE_CONTEXT_COMMITTED, null, null, null,
                new FieldClassificationResult[] {
                        new FieldClassificationResult(fieldId, remoteId, score)
                });
    }

    public static void assertFillEventForFieldsClassification(@NonNull FillEventHistory.Event event,
            @NonNull FieldClassificationResult[] results) {
        assertFillEvent(event, TYPE_CONTEXT_COMMITTED, null, null, null, results);
    }

    public static void assertFillEventForContextCommitted(@NonNull FillEventHistory.Event event) {
        assertFillEvent(event, TYPE_CONTEXT_COMMITTED, null, null, null, null);
    }

    @NonNull
    public static String getActivityName(List<FillContext> contexts) {
        if (contexts == null) return "N/A (null contexts)";

        if (contexts.isEmpty()) return "N/A (empty contexts)";

        final AssistStructure structure = contexts.get(contexts.size() - 1).getStructure();
        if (structure == null) return "N/A (no AssistStructure)";

        final ComponentName componentName = structure.getActivityComponent();
        if (componentName == null) return "N/A (no component name)";

        return componentName.flattenToShortString();
    }

    public static void assertFloat(float actualValue, float expectedValue) {
        assertThat(actualValue).isWithin(1.0e-10f).of(expectedValue);
    }

    public static void assertHasFlags(int actualFlags, int expectedFlags) {
        assertWithMessage("Flags %s not in %s", expectedFlags, actualFlags)
                .that(actualFlags & expectedFlags).isEqualTo(expectedFlags);
    }

    public static String callbackEventAsString(int event) {
        switch (event) {
            case AutofillCallback.EVENT_INPUT_HIDDEN:
                return "HIDDEN";
            case AutofillCallback.EVENT_INPUT_SHOWN:
                return "SHOWN";
            case AutofillCallback.EVENT_INPUT_UNAVAILABLE:
                return "UNAVAILABLE";
            default:
                return "UNKNOWN:" + event;
        }
    }

    public static String importantForAutofillAsString(int mode) {
        switch (mode) {
            case View.IMPORTANT_FOR_AUTOFILL_AUTO:
                return "IMPORTANT_FOR_AUTOFILL_AUTO";
            case View.IMPORTANT_FOR_AUTOFILL_YES:
                return "IMPORTANT_FOR_AUTOFILL_YES";
            case View.IMPORTANT_FOR_AUTOFILL_YES_EXCLUDE_DESCENDANTS:
                return "IMPORTANT_FOR_AUTOFILL_YES_EXCLUDE_DESCENDANTS";
            case View.IMPORTANT_FOR_AUTOFILL_NO:
                return "IMPORTANT_FOR_AUTOFILL_NO";
            case View.IMPORTANT_FOR_AUTOFILL_NO_EXCLUDE_DESCENDANTS:
                return "IMPORTANT_FOR_AUTOFILL_NO_EXCLUDE_DESCENDANTS";
            default:
                return "UNKNOWN:" + mode;
        }
    }

    public static boolean hasHint(@Nullable String[] hints, @Nullable Object expectedHint) {
        if (hints == null || expectedHint == null) return false;
        for (String actualHint : hints) {
            if (expectedHint.equals(actualHint)) return true;
        }
        return false;
    }

    /**
     * Asserts that 2 bitmaps have are the same. If they aren't throws an exception and dump them
     * locally so their can be visually inspected.
     *
     * @param filename base name of the files generated in case of error
     * @param bitmap1 first bitmap to be compared
     * @param bitmap2 second bitmap to be compared
     */
    public static void assertBitmapsAreSame(@NonNull String filename, @Nullable Bitmap bitmap1,
            @Nullable Bitmap bitmap2) throws IOException {
        assertWithMessage("1st bitmap is null").that(bitmap1).isNotNull();
        assertWithMessage("2nd bitmap is null").that(bitmap2).isNotNull();
        final boolean same = bitmap1.sameAs(bitmap2);
        if (same) {
            Log.v(TAG, "bitmap comparison passed for " + filename);
            return;
        }

        final File dir = new File(LOCAL_DIRECTORY);
        dir.mkdirs();
        if (!dir.exists()) {
            Log.e(TAG, "Could not create directory " + dir);
            throw new AssertionError("bitmap comparison failed for " + filename
                    + ", and bitmaps could not be dumped on " + dir);
        }
        final File dump1 = dumpBitmap(bitmap1, dir, filename + "-1.png");
        final File dump2 = dumpBitmap(bitmap2, dir, filename + "-2.png");
        throw new AssertionError(
                "bitmap comparison failed; check contents of " + dump1 + " and " + dump2);
    }

    @Nullable
    private static File dumpBitmap(@NonNull Bitmap bitmap, @NonNull File dir,
            @NonNull String filename) throws IOException {
        final File file = new File(dir, filename);
        if (file.exists()) {
            file.delete();
        }
        if (!file.createNewFile()) {
            Log.e(TAG, "Could not create file " + file);
            return null;
        }
        Log.d(TAG, "Dumping bitmap at " + file);
        BitmapUtils.saveBitmap(bitmap, file.getParent(), file.getName());
        return file;
    }

    private Helper() {
        throw new UnsupportedOperationException("contain static methods only");
    }

    static class FieldClassificationResult {
        public final AutofillId id;
        public final String[] remoteIds;
        public final float[] scores;

        FieldClassificationResult(@NonNull AutofillId id, @NonNull String remoteId, float score) {
            this(id, new String[] { remoteId }, new float[] { score });
        }

        FieldClassificationResult(@NonNull AutofillId id, @NonNull String[] remoteIds,
                float[] scores) {
            this.id = id;
            this.remoteIds = remoteIds;
            this.scores = scores;
        }
    }
}
