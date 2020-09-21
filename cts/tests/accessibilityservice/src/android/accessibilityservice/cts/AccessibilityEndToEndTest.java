/*
 * Copyright (C) 2010 The Android Open Source Project
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

package android.accessibilityservice.cts;

import static android.accessibilityservice.cts.utils.AccessibilityEventFilterUtils
        .filterForEventType;
import static android.accessibilityservice.cts.utils.RunOnMainUtils.getOnMain;
import static android.view.accessibility.AccessibilityNodeInfo.AccessibilityAction
        .ACTION_HIDE_TOOLTIP;
import static android.view.accessibility.AccessibilityNodeInfo.AccessibilityAction
        .ACTION_SHOW_TOOLTIP;

import static org.hamcrest.core.IsEqual.equalTo;
import static org.hamcrest.core.IsNull.nullValue;
import static org.hamcrest.core.IsNull.notNullValue;
import static org.hamcrest.Matchers.in;
import static org.hamcrest.Matchers.not;
import static org.junit.Assert.assertThat;

import android.accessibilityservice.cts.activities.AccessibilityEndToEndActivity;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Instrumentation;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.app.UiAutomation;
import android.appwidget.AppWidgetHost;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProviderInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.os.Process;
import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.Presubmit;
import android.test.suitebuilder.annotation.MediumTest;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;

import java.util.Iterator;
import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * This class performs end-to-end testing of the accessibility feature by
 * creating an {@link Activity} and poking around so {@link AccessibilityEvent}s
 * are generated and their correct dispatch verified.
 */
public class AccessibilityEndToEndTest extends
        AccessibilityActivityTestCase<AccessibilityEndToEndActivity> {

    private static final String LOG_TAG = "AccessibilityEndToEndTest";

    private static final String GRANT_BIND_APP_WIDGET_PERMISSION_COMMAND =
            "appwidget grantbind --package android.accessibilityservice.cts --user 0";

    private static final String REVOKE_BIND_APP_WIDGET_PERMISSION_COMMAND =
            "appwidget revokebind --package android.accessibilityservice.cts --user 0";

    private static final String APP_WIDGET_PROVIDER_PACKAGE = "foo.bar.baz";

    /**
     * Creates a new instance for testing {@link AccessibilityEndToEndActivity}.
     */
    public AccessibilityEndToEndTest() {
        super(AccessibilityEndToEndActivity.class);
    }

    @MediumTest
    @Presubmit
    public void testTypeViewSelectedAccessibilityEvent() throws Throwable {
        // create and populate the expected event
        final AccessibilityEvent expected = AccessibilityEvent.obtain();
        expected.setEventType(AccessibilityEvent.TYPE_VIEW_SELECTED);
        expected.setClassName(ListView.class.getName());
        expected.setPackageName(getActivity().getPackageName());
        expected.getText().add(getActivity().getString(R.string.second_list_item));
        expected.setItemCount(2);
        expected.setCurrentItemIndex(1);
        expected.setEnabled(true);
        expected.setScrollable(false);
        expected.setFromIndex(0);
        expected.setToIndex(1);

        final ListView listView = (ListView) getActivity().findViewById(R.id.listview);

        AccessibilityEvent awaitedEvent =
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(
                new Runnable() {
            @Override
            public void run() {
                // trigger the event
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        listView.setSelection(1);
                    }
                });
            }},
            new UiAutomation.AccessibilityEventFilter() {
                // check the received event
                @Override
                public boolean accept(AccessibilityEvent event) {
                    return equalsAccessiblityEvent(event, expected);
                }
            },
            TIMEOUT_ASYNC_PROCESSING);
        assertNotNull("Did not receive expected event: " + expected, awaitedEvent);
    }

    @MediumTest
    @Presubmit
    public void testTypeViewClickedAccessibilityEvent() throws Throwable {
        // create and populate the expected event
        final AccessibilityEvent expected = AccessibilityEvent.obtain();
        expected.setEventType(AccessibilityEvent.TYPE_VIEW_CLICKED);
        expected.setClassName(Button.class.getName());
        expected.setPackageName(getActivity().getPackageName());
        expected.getText().add(getActivity().getString(R.string.button_title));
        expected.setEnabled(true);

        final Button button = (Button) getActivity().findViewById(R.id.button);

        AccessibilityEvent awaitedEvent =
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(
                new Runnable() {
            @Override
            public void run() {
                // trigger the event
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        button.performClick();
                    }
                });
            }},
            new UiAutomation.AccessibilityEventFilter() {
                // check the received event
                @Override
                public boolean accept(AccessibilityEvent event) {
                    return equalsAccessiblityEvent(event, expected);
                }
            },
            TIMEOUT_ASYNC_PROCESSING);
        assertNotNull("Did not receive expected event: " + expected, awaitedEvent);
    }

    @MediumTest
    @Presubmit
    public void testTypeViewLongClickedAccessibilityEvent() throws Throwable {
        // create and populate the expected event
        final AccessibilityEvent expected = AccessibilityEvent.obtain();
        expected.setEventType(AccessibilityEvent.TYPE_VIEW_LONG_CLICKED);
        expected.setClassName(Button.class.getName());
        expected.setPackageName(getActivity().getPackageName());
        expected.getText().add(getActivity().getString(R.string.button_title));
        expected.setEnabled(true);

        final Button button = (Button) getActivity().findViewById(R.id.button);

        AccessibilityEvent awaitedEvent =
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(
                new Runnable() {
            @Override
            public void run() {
                // trigger the event
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        button.performLongClick();
                    }
                });
            }},
            new UiAutomation.AccessibilityEventFilter() {
                // check the received event
                @Override
                public boolean accept(AccessibilityEvent event) {
                    return equalsAccessiblityEvent(event, expected);
                }
            },
            TIMEOUT_ASYNC_PROCESSING);
        assertNotNull("Did not receive expected event: " + expected, awaitedEvent);
    }

    @MediumTest
    @Presubmit
    public void testTypeViewFocusedAccessibilityEvent() throws Throwable {
        // create and populate the expected event
        final AccessibilityEvent expected = AccessibilityEvent.obtain();
        expected.setEventType(AccessibilityEvent.TYPE_VIEW_FOCUSED);
        expected.setClassName(Button.class.getName());
        expected.setPackageName(getActivity().getPackageName());
        expected.getText().add(getActivity().getString(R.string.button_title));
        expected.setItemCount(4);
        expected.setCurrentItemIndex(3);
        expected.setEnabled(true);

        final Button button = (Button) getActivity().findViewById(R.id.buttonWithTooltip);

        AccessibilityEvent awaitedEvent =
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(
                    () -> getActivity().runOnUiThread(() -> button.requestFocus()),
                    (event) -> equalsAccessiblityEvent(event, expected),
                    TIMEOUT_ASYNC_PROCESSING);
        assertNotNull("Did not receive expected event: " + expected, awaitedEvent);
    }

    @MediumTest
    @Presubmit
    public void testTypeViewTextChangedAccessibilityEvent() throws Throwable {
        // focus the edit text
        final EditText editText = (EditText) getActivity().findViewById(R.id.edittext);

        AccessibilityEvent awaitedFocusEvent =
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(
                new Runnable() {
            @Override
            public void run() {
                // trigger the event
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        editText.requestFocus();
                    }
                });
            }},
            new UiAutomation.AccessibilityEventFilter() {
                // check the received event
                @Override
                public boolean accept(AccessibilityEvent event) {
                    return event.getEventType() == AccessibilityEvent.TYPE_VIEW_FOCUSED;
                }
            },
            TIMEOUT_ASYNC_PROCESSING);
        assertNotNull("Did not receive expected focuss event.", awaitedFocusEvent);

        final String beforeText = getActivity().getString(R.string.text_input_blah);
        final String newText = getActivity().getString(R.string.text_input_blah_blah);
        final String afterText = beforeText.substring(0, 3) + newText;

        // create and populate the expected event
        final AccessibilityEvent expected = AccessibilityEvent.obtain();
        expected.setEventType(AccessibilityEvent.TYPE_VIEW_TEXT_CHANGED);
        expected.setClassName(EditText.class.getName());
        expected.setPackageName(getActivity().getPackageName());
        expected.getText().add(afterText);
        expected.setBeforeText(beforeText);
        expected.setFromIndex(3);
        expected.setAddedCount(9);
        expected.setRemovedCount(1);
        expected.setEnabled(true);

        AccessibilityEvent awaitedTextChangeEvent =
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(
                new Runnable() {
            @Override
            public void run() {
                // trigger the event
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        editText.getEditableText().replace(3, 4, newText);
                    }
                });
            }},
            new UiAutomation.AccessibilityEventFilter() {
                // check the received event
                @Override
                public boolean accept(AccessibilityEvent event) {
                    return equalsAccessiblityEvent(event, expected);
                }
            },
            TIMEOUT_ASYNC_PROCESSING);
        assertNotNull("Did not receive expected event: " + expected, awaitedTextChangeEvent);
    }

    @MediumTest
    @Presubmit
    public void testTypeWindowStateChangedAccessibilityEvent() throws Throwable {
        // create and populate the expected event
        final AccessibilityEvent expected = AccessibilityEvent.obtain();
        expected.setEventType(AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED);
        expected.setClassName(AlertDialog.class.getName());
        expected.setPackageName(getActivity().getPackageName());
        expected.getText().add(getActivity().getString(R.string.alert_title));
        expected.getText().add(getActivity().getString(R.string.alert_message));
        expected.setEnabled(true);

        AccessibilityEvent awaitedEvent =
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(
                new Runnable() {
            @Override
            public void run() {
                // trigger the event
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        (new AlertDialog.Builder(getActivity()).setTitle(R.string.alert_title)
                                .setMessage(R.string.alert_message)).create().show();
                    }
                });
            }},
            new UiAutomation.AccessibilityEventFilter() {
                // check the received event
                @Override
                public boolean accept(AccessibilityEvent event) {
                    return equalsAccessiblityEvent(event, expected);
                }
            },
            TIMEOUT_ASYNC_PROCESSING);
        assertNotNull("Did not receive expected event: " + expected, awaitedEvent);
    }

    @MediumTest
    @AppModeFull
    @SuppressWarnings("deprecation")
    @Presubmit
    public void testTypeNotificationStateChangedAccessibilityEvent() throws Throwable {
        // No notification UI on televisions.
        if ((getActivity().getResources().getConfiguration().uiMode
                & Configuration.UI_MODE_TYPE_MASK) == Configuration.UI_MODE_TYPE_TELEVISION) {
            Log.i(LOG_TAG, "Skipping: testTypeNotificationStateChangedAccessibilityEvent" +
                    " - No notification UI on televisions.");
            return;
        }
        PackageManager pm = getInstrumentation().getTargetContext().getPackageManager();
        if (pm.hasSystemFeature(pm.FEATURE_WATCH)) {
            Log.i(LOG_TAG, "Skipping: testTypeNotificationStateChangedAccessibilityEvent" +
                    " - Watches have different notification system.");
            return;
        }

        String message = getActivity().getString(R.string.notification_message);

        final NotificationManager notificationManager =
                (NotificationManager) getActivity().getSystemService(Service.NOTIFICATION_SERVICE);
        final NotificationChannel channel =
                new NotificationChannel("id", "name", NotificationManager.IMPORTANCE_DEFAULT);
        try {
            // create the notification to send
            channel.enableVibration(true);
            channel.enableLights(true);
            channel.setBypassDnd(true);
            notificationManager.createNotificationChannel(channel);
            NotificationChannel created =
                    notificationManager.getNotificationChannel(channel.getId());
            final int notificationId = 1;
            final Notification notification =
                    new Notification.Builder(getActivity(), channel.getId())
                            .setSmallIcon(android.R.drawable.stat_notify_call_mute)
                            .setContentIntent(PendingIntent.getActivity(getActivity(), 0,
                                    new Intent(),
                                    PendingIntent.FLAG_CANCEL_CURRENT))
                            .setTicker(message)
                            .setContentTitle("")
                            .setContentText("")
                            .setPriority(Notification.PRIORITY_MAX)
                            // Mark the notification as "interruptive" by specifying a vibration
                            // pattern. This ensures it's announced properly on watch-type devices.
                            .setVibrate(new long[]{})
                            .build();

            // create and populate the expected event
            final AccessibilityEvent expected = AccessibilityEvent.obtain();
            expected.setEventType(AccessibilityEvent.TYPE_NOTIFICATION_STATE_CHANGED);
            expected.setClassName(Notification.class.getName());
            expected.setPackageName(getActivity().getPackageName());
            expected.getText().add(message);
            expected.setParcelableData(notification);

            AccessibilityEvent awaitedEvent =
                    getInstrumentation().getUiAutomation().executeAndWaitForEvent(
                            new Runnable() {
                                @Override
                                public void run() {
                                    // trigger the event
                                    getActivity().runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            // trigger the event
                                            notificationManager
                                                    .notify(notificationId, notification);
                                            getActivity().finish();
                                        }
                                    });
                                }
                            },
                            new UiAutomation.AccessibilityEventFilter() {
                                // check the received event
                                @Override
                                public boolean accept(AccessibilityEvent event) {
                                    return equalsAccessiblityEvent(event, expected);
                                }
                            },
                            TIMEOUT_ASYNC_PROCESSING);
            assertNotNull("Did not receive expected event: " + expected, awaitedEvent);
        } finally {
            notificationManager.deleteNotificationChannel(channel.getId());
        }
    }

    @MediumTest
    public void testInterrupt_notifiesService() {
        getInstrumentation()
                .getUiAutomation(UiAutomation.FLAG_DONT_SUPPRESS_ACCESSIBILITY_SERVICES);
        InstrumentedAccessibilityService service = InstrumentedAccessibilityService.enableService(
                getInstrumentation(), InstrumentedAccessibilityService.class);
        try {
            assertFalse(service.wasOnInterruptCalled());

            getActivity().runOnUiThread(() -> {
                AccessibilityManager accessibilityManager = (AccessibilityManager) getActivity()
                        .getSystemService(Service.ACCESSIBILITY_SERVICE);
                accessibilityManager.interrupt();
            });

            Object waitObject = service.getInterruptWaitObject();
            synchronized (waitObject) {
                if (!service.wasOnInterruptCalled()) {
                    try {
                        waitObject.wait(TIMEOUT_ASYNC_PROCESSING);
                    } catch (InterruptedException e) {
                        // Do nothing
                    }
                }
            }
            assertTrue(service.wasOnInterruptCalled());
        } finally {
            service.disableSelfAndRemove();
        }
    }

    @MediumTest
    public void testPackageNameCannotBeFaked() throws Exception {
        getActivity().runOnUiThread(() -> {
            // Set the activity to report fake package for events and nodes
            getActivity().setReportedPackageName("foo.bar.baz");

            // Make sure node package cannot be faked
            AccessibilityNodeInfo root = getInstrumentation().getUiAutomation()
                    .getRootInActiveWindow();
            assertPackageName(root, getActivity().getPackageName());
        });

        // Make sure event package cannot be faked
        try {
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(() ->
                getInstrumentation().runOnMainSync(() ->
                    getActivity().findViewById(R.id.button).requestFocus())
                , (AccessibilityEvent event) ->
                    event.getEventType() == AccessibilityEvent.TYPE_VIEW_FOCUSED
                            && event.getPackageName().equals(getActivity().getPackageName())
                , TIMEOUT_ASYNC_PROCESSING);
        } catch (TimeoutException e) {
            fail("Events from fake package should be fixed to use the correct package");
        }
    }

    @AppModeFull
    @MediumTest
    @Presubmit
    public void testPackageNameCannotBeFakedAppWidget() throws Exception {
        if (!hasAppWidgets()) {
            return;
        }

        getInstrumentation().runOnMainSync(() -> {
            // Set the activity to report fake package for events and nodes
            getActivity().setReportedPackageName(APP_WIDGET_PROVIDER_PACKAGE);

            // Make sure we cannot report nodes as if from the widget package
            AccessibilityNodeInfo root = getInstrumentation().getUiAutomation()
                    .getRootInActiveWindow();
            assertPackageName(root, getActivity().getPackageName());
        });

        // Make sure we cannot send events as if from the widget package
        try {
            getInstrumentation().getUiAutomation().executeAndWaitForEvent(() ->
                getInstrumentation().runOnMainSync(() ->
                    getActivity().findViewById(R.id.button).requestFocus())
                , (AccessibilityEvent event) ->
                    event.getEventType() == AccessibilityEvent.TYPE_VIEW_FOCUSED
                            && event.getPackageName().equals(getActivity().getPackageName())
                , TIMEOUT_ASYNC_PROCESSING);
        } catch (TimeoutException e) {
            fail("Should not be able to send events from a widget package if no widget hosted");
        }

        // Create a host and start listening.
        final AppWidgetHost host = new AppWidgetHost(getInstrumentation().getTargetContext(), 0);
        host.deleteHost();
        host.startListening();

        // Well, app do not have this permission unless explicitly granted
        // by the user. Now we will pretend for the user and grant it.
        grantBindAppWidgetPermission();

        // Allocate an app widget id to bind.
        final int appWidgetId = host.allocateAppWidgetId();
        try {
            // Grab a provider we defined to be bound.
            final AppWidgetProviderInfo provider = getAppWidgetProviderInfo();

            // Bind the widget.
            final boolean widgetBound = getAppWidgetManager().bindAppWidgetIdIfAllowed(
                    appWidgetId, provider.getProfile(), provider.provider, null);
            assertTrue(widgetBound);

            // Make sure the app can use the package of a widget it hosts
            getInstrumentation().runOnMainSync(() -> {
                // Make sure we can report nodes as if from the widget package
                AccessibilityNodeInfo root = getInstrumentation().getUiAutomation()
                        .getRootInActiveWindow();
                assertPackageName(root, APP_WIDGET_PROVIDER_PACKAGE);
            });

            // Make sure we can send events as if from the widget package
            try {
                getInstrumentation().getUiAutomation().executeAndWaitForEvent(() ->
                    getInstrumentation().runOnMainSync(() ->
                        getActivity().findViewById(R.id.button).performClick())
                    , (AccessibilityEvent event) ->
                            event.getEventType() == AccessibilityEvent.TYPE_VIEW_CLICKED
                                    && event.getPackageName().equals(APP_WIDGET_PROVIDER_PACKAGE)
                    , TIMEOUT_ASYNC_PROCESSING);
            } catch (TimeoutException e) {
                fail("Should be able to send events from a widget package if widget hosted");
            }
        } finally {
            // Clean up.
            host.deleteAppWidgetId(appWidgetId);
            host.deleteHost();
            revokeBindAppWidgetPermission();
        }
    }

    @MediumTest
    @Presubmit
    public void testViewHeadingReportedToAccessibility() throws Exception {
        final Instrumentation instrumentation = getInstrumentation();
        final EditText editText = (EditText) getOnMain(instrumentation, () -> {
            return getActivity().findViewById(R.id.edittext);
        });
        // Make sure the edittext was populated properly from xml
        final boolean editTextIsHeading = getOnMain(instrumentation, () -> {
            return editText.isAccessibilityHeading();
        });
        assertTrue("isAccessibilityHeading not populated properly from xml", editTextIsHeading);

        final UiAutomation uiAutomation = instrumentation.getUiAutomation();
        final AccessibilityNodeInfo editTextNode = uiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByViewId(
                        "android.accessibilityservice.cts:id/edittext")
                .get(0);
        assertTrue("isAccessibilityHeading not reported to accessibility",
                editTextNode.isHeading());

        uiAutomation.executeAndWaitForEvent(() -> instrumentation.runOnMainSync(() ->
                        editText.setAccessibilityHeading(false)),
                filterForEventType(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED),
                TIMEOUT_ASYNC_PROCESSING);
        editTextNode.refresh();
        assertFalse("isAccessibilityHeading not reported to accessibility after update",
                editTextNode.isHeading());
    }

    @MediumTest
    @Presubmit
    public void testTooltipTextReportedToAccessibility() {
        final Instrumentation instrumentation = getInstrumentation();
        final UiAutomation uiAutomation = instrumentation.getUiAutomation();
        final AccessibilityNodeInfo buttonNode = uiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByViewId(
                        "android.accessibilityservice.cts:id/buttonWithTooltip")
                .get(0);
        assertEquals("Tooltip text not reported to accessibility",
                instrumentation.getContext().getString(R.string.button_tooltip),
                buttonNode.getTooltipText());
    }

    @MediumTest
    public void testTooltipTextActionsReportedToAccessibility() throws Exception {
        final Instrumentation instrumentation = getInstrumentation();
        final UiAutomation uiAutomation = instrumentation.getUiAutomation();
        final AccessibilityNodeInfo buttonNode = uiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByViewId(
                        "android.accessibilityservice.cts:id/buttonWithTooltip")
                .get(0);
        assertFalse(hasTooltipShowing(R.id.buttonWithTooltip));
        assertThat(ACTION_SHOW_TOOLTIP, in(buttonNode.getActionList()));
        assertThat(ACTION_HIDE_TOOLTIP, not(in(buttonNode.getActionList())));
        uiAutomation.executeAndWaitForEvent(() -> buttonNode.performAction(
                ACTION_SHOW_TOOLTIP.getId()),
                filterForEventType(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED),
                TIMEOUT_ASYNC_PROCESSING);

        // The button should now be showing the tooltip, so it should have the option to hide it.
        buttonNode.refresh();
        assertThat(ACTION_HIDE_TOOLTIP, in(buttonNode.getActionList()));
        assertThat(ACTION_SHOW_TOOLTIP, not(in(buttonNode.getActionList())));
        assertTrue(hasTooltipShowing(R.id.buttonWithTooltip));
    }

    @MediumTest
    public void testTraversalBeforeReportedToAccessibility() throws Exception {
        final Instrumentation instrumentation = getInstrumentation();
        final UiAutomation uiAutomation = instrumentation.getUiAutomation();
        final AccessibilityNodeInfo buttonNode = uiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByViewId(
                        "android.accessibilityservice.cts:id/buttonWithTooltip")
                .get(0);
        final AccessibilityNodeInfo beforeNode = buttonNode.getTraversalBefore();
        assertThat(beforeNode, notNullValue());
        assertThat(beforeNode.getViewIdResourceName(),
                equalTo("android.accessibilityservice.cts:id/edittext"));

        uiAutomation.executeAndWaitForEvent(() -> instrumentation.runOnMainSync(
                () -> getActivity().findViewById(R.id.buttonWithTooltip)
                        .setAccessibilityTraversalBefore(View.NO_ID)),
                filterForEventType(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED),
                TIMEOUT_ASYNC_PROCESSING);

        buttonNode.refresh();
        assertThat(buttonNode.getTraversalBefore(), nullValue());
    }

    @MediumTest
    public void testTraversalAfterReportedToAccessibility() throws Exception {
        final Instrumentation instrumentation = getInstrumentation();
        final UiAutomation uiAutomation = instrumentation.getUiAutomation();
        final AccessibilityNodeInfo editNode = uiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByViewId(
                        "android.accessibilityservice.cts:id/edittext")
                .get(0);
        final AccessibilityNodeInfo afterNode = editNode.getTraversalAfter();
        assertThat(afterNode, notNullValue());
        assertThat(afterNode.getViewIdResourceName(),
                equalTo("android.accessibilityservice.cts:id/buttonWithTooltip"));

        uiAutomation.executeAndWaitForEvent(() -> instrumentation.runOnMainSync(
                () -> getActivity().findViewById(R.id.edittext)
                        .setAccessibilityTraversalAfter(View.NO_ID)),
                filterForEventType(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED),
                TIMEOUT_ASYNC_PROCESSING);

        editNode.refresh();
        assertThat(editNode.getTraversalAfter(), nullValue());
    }

    @MediumTest
    public void testLabelForReportedToAccessibility() throws Exception {
        final Instrumentation instrumentation = getInstrumentation();
        final UiAutomation uiAutomation = instrumentation.getUiAutomation();
        uiAutomation.executeAndWaitForEvent(() -> instrumentation.runOnMainSync(() -> getActivity()
                .findViewById(R.id.edittext).setLabelFor(R.id.buttonWithTooltip)),
                filterForEventType(AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED),
                TIMEOUT_ASYNC_PROCESSING);
        // TODO: b/78022650: This code should move above the executeAndWait event. It's here because
        // the a11y cache doesn't get notified when labelFor changes, so the node with the
        // labledBy isn't updated.
        final AccessibilityNodeInfo editNode = uiAutomation.getRootInActiveWindow()
                .findAccessibilityNodeInfosByViewId(
                        "android.accessibilityservice.cts:id/edittext")
                .get(0);
        editNode.refresh();
        final AccessibilityNodeInfo labelForNode = editNode.getLabelFor();
        assertThat(labelForNode, notNullValue());
        // Labeled node should indicate that it is labeled by the other one
        assertThat(labelForNode.getLabeledBy(), equalTo(editNode));
    }

    private static void assertPackageName(AccessibilityNodeInfo node, String packageName) {
        if (node == null) {
            return;
        }
        assertEquals(packageName, node.getPackageName());
        final int childCount = node.getChildCount();
        for (int i = 0; i < childCount; i++) {
            AccessibilityNodeInfo child = node.getChild(i);
            if (child != null) {
                assertPackageName(child, packageName);
            }
        }
    }

    private AppWidgetProviderInfo getAppWidgetProviderInfo() {
        final ComponentName componentName = new ComponentName(
                "foo.bar.baz", "foo.bar.baz.MyAppWidgetProvider");
        final List<AppWidgetProviderInfo> providers = getAppWidgetManager().getInstalledProviders();
        final int providerCount = providers.size();
        for (int i = 0; i < providerCount; i++) {
            final AppWidgetProviderInfo provider = providers.get(i);
            if (componentName.equals(provider.provider)
                    && Process.myUserHandle().equals(provider.getProfile())) {
                return provider;
            }
        }
        return null;
    }

    private void grantBindAppWidgetPermission() throws Exception {
        ShellCommandBuilder.execShellCommand(getInstrumentation().getUiAutomation(),
                GRANT_BIND_APP_WIDGET_PERMISSION_COMMAND);
    }

    private void revokeBindAppWidgetPermission() throws Exception {
        ShellCommandBuilder.execShellCommand(getInstrumentation().getUiAutomation(),
                REVOKE_BIND_APP_WIDGET_PERMISSION_COMMAND);
    }

    private AppWidgetManager getAppWidgetManager() {
        return (AppWidgetManager) getInstrumentation().getTargetContext()
                .getSystemService(Context.APPWIDGET_SERVICE);
    }

    private boolean hasAppWidgets() {
        return getInstrumentation().getTargetContext().getPackageManager()
                .hasSystemFeature(PackageManager.FEATURE_APP_WIDGETS);
    }

    /**
     * Compares all properties of the <code>first</code> and the
     * <code>second</code>.
     */
    private boolean equalsAccessiblityEvent(AccessibilityEvent first, AccessibilityEvent second) {
         return first.getEventType() == second.getEventType()
            && first.isChecked() == second.isChecked()
            && first.getCurrentItemIndex() == second.getCurrentItemIndex()
            && first.isEnabled() == second.isEnabled()
            && first.getFromIndex() == second.getFromIndex()
            && first.getItemCount() == second.getItemCount()
            && first.isPassword() == second.isPassword()
            && first.getRemovedCount() == second.getRemovedCount()
            && first.isScrollable()== second.isScrollable()
            && first.getToIndex() == second.getToIndex()
            && first.getRecordCount() == second.getRecordCount()
            && first.getScrollX() == second.getScrollX()
            && first.getScrollY() == second.getScrollY()
            && first.getAddedCount() == second.getAddedCount()
            && TextUtils.equals(first.getBeforeText(), second.getBeforeText())
            && TextUtils.equals(first.getClassName(), second.getClassName())
            && TextUtils.equals(first.getContentDescription(), second.getContentDescription())
            && equalsNotificationAsParcelableData(first, second)
            && equalsText(first, second);
    }

    /**
     * Compares the {@link android.os.Parcelable} data of the
     * <code>first</code> and <code>second</code>.
     */
    private boolean equalsNotificationAsParcelableData(AccessibilityEvent first,
            AccessibilityEvent second) {
        Notification firstNotification = (Notification) first.getParcelableData();
        Notification secondNotification = (Notification) second.getParcelableData();
        if (firstNotification == null) {
            return (secondNotification == null);
        } else if (secondNotification == null) {
            return false;
        }
        return TextUtils.equals(firstNotification.tickerText, secondNotification.tickerText);
    }

    /**
     * Compares the text of the <code>first</code> and <code>second</code> text.
     */
    private boolean equalsText(AccessibilityEvent first, AccessibilityEvent second) {
        List<CharSequence> firstText = first.getText();
        List<CharSequence> secondText = second.getText();
        if (firstText.size() != secondText.size()) {
            return false;
        }
        Iterator<CharSequence> firstIterator = firstText.iterator();
        Iterator<CharSequence> secondIterator = secondText.iterator();
        for (int i = 0; i < firstText.size(); i++) {
            if (!firstIterator.next().toString().equals(secondIterator.next().toString())) {
                return false;
            }
        }
        return true;
    }

    private boolean hasTooltipShowing(int id) {
        return getOnMain(getInstrumentation(), () -> {
            final View viewWithTooltip = getActivity().findViewById(id);
            if (viewWithTooltip == null) {
                return false;
            }
            final View tooltipView = viewWithTooltip.getTooltipView();
            return (tooltipView != null) && (tooltipView.getParent() != null);
        });
    }
}
