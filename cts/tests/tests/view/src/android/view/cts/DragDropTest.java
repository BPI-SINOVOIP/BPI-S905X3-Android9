/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.view.cts;

import static junit.framework.TestCase.assertFalse;
import static junit.framework.TestCase.assertTrue;
import static junit.framework.TestCase.fail;

import android.app.Instrumentation;
import android.app.UiAutomation;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.pm.PackageManager;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.DragEvent;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.stream.IntStream;

@RunWith(AndroidJUnit4.class)
public class DragDropTest {
    static final String TAG = "DragDropTest";

    final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
    final UiAutomation mAutomation = mInstrumentation.getUiAutomation();

    @Rule
    public ActivityTestRule<DragDropActivity> mActivityRule =
            new ActivityTestRule<>(DragDropActivity.class);

    private DragDropActivity mActivity;

    private CountDownLatch mStartReceived;
    private CountDownLatch mEndReceived;

    private AssertionError mMainThreadAssertionError;

    /**
     * Check whether two objects have the same binary data when dumped into Parcels
     * @return True if the objects are equal
     */
    private static boolean compareParcelables(Parcelable obj1, Parcelable obj2) {
        if (obj1 == null && obj2 == null) {
            return true;
        }
        if (obj1 == null || obj2 == null) {
            return false;
        }
        Parcel p1 = Parcel.obtain();
        obj1.writeToParcel(p1, 0);
        Parcel p2 = Parcel.obtain();
        obj2.writeToParcel(p2, 0);
        boolean result = Arrays.equals(p1.marshall(), p2.marshall());
        p1.recycle();
        p2.recycle();
        return result;
    }

    private static final ClipDescription sClipDescription =
            new ClipDescription("TestLabel", new String[]{"text/plain"});
    private static final ClipData sClipData =
            new ClipData(sClipDescription, new ClipData.Item("TestText"));
    private static final Object sLocalState = new Object(); // just check if null or not

    class LogEntry {
        public View view;

        // Public DragEvent fields
        public int action; // DragEvent.getAction()
        public float x; // DragEvent.getX()
        public float y; // DragEvent.getY()
        public ClipData clipData; // DragEvent.getClipData()
        public ClipDescription clipDescription; // DragEvent.getClipDescription()
        public Object localState; // DragEvent.getLocalState()
        public boolean result; // DragEvent.getResult()

        LogEntry(View v, int action, float x, float y, ClipData clipData,
                ClipDescription clipDescription, Object localState, boolean result) {
            this.view = v;
            this.action = action;
            this.x = x;
            this.y = y;
            this.clipData = clipData;
            this.clipDescription = clipDescription;
            this.localState = localState;
            this.result = result;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof LogEntry)) {
                return false;
            }
            final LogEntry other = (LogEntry) obj;
            return view == other.view && action == other.action
                    && x == other.x && y == other.y
                    && compareParcelables(clipData, other.clipData)
                    && compareParcelables(clipDescription, other.clipDescription)
                    && localState == other.localState
                    && result == other.result;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("DragEvent {action=").append(action).append(" x=").append(x).append(" y=")
                    .append(y).append(" result=").append(result).append("}")
                    .append(" @ ").append(view);
            return sb.toString();
        }
    }

    // Actual and expected sequences of events.
    // While the test is running, logs should be accessed only from the main thread.
    final private ArrayList<LogEntry> mActual = new ArrayList<LogEntry> ();
    final private ArrayList<LogEntry> mExpected = new ArrayList<LogEntry> ();

    private static ClipData obtainClipData(int action) {
        if (action == DragEvent.ACTION_DROP) {
            return sClipData;
        }
        return null;
    }

    private static ClipDescription obtainClipDescription(int action) {
        if (action == DragEvent.ACTION_DRAG_ENDED) {
            return null;
        }
        return sClipDescription;
    }

    private void logEvent(View v, DragEvent ev) {
        if (ev.getAction() == DragEvent.ACTION_DRAG_STARTED) {
            mStartReceived.countDown();
        }
        if (ev.getAction() == DragEvent.ACTION_DRAG_ENDED) {
            mEndReceived.countDown();
        }
        mActual.add(new LogEntry(v, ev.getAction(), ev.getX(), ev.getY(), ev.getClipData(),
                ev.getClipDescription(), ev.getLocalState(), ev.getResult()));
    }

    // Add expected event for a view, with zero coordinates.
    private void expectEvent5(int action, int viewId) {
        View v = mActivity.findViewById(viewId);
        mExpected.add(new LogEntry(v, action, 0, 0, obtainClipData(action),
                obtainClipDescription(action), sLocalState, false));
    }

    // Add expected event for a view.
    private void expectEndEvent(int viewId, float x, float y, boolean result) {
        View v = mActivity.findViewById(viewId);
        int action = DragEvent.ACTION_DRAG_ENDED;
        mExpected.add(new LogEntry(v, action, x, y, obtainClipData(action),
                obtainClipDescription(action), sLocalState, result));
    }

    // Add expected successful-end event for a view.
    private void expectEndEventSuccess(int viewId) {
        expectEndEvent(viewId, 0, 0, true);
    }

    // Add expected failed-end event for a view, with the release coordinates shifted by 6 relative
    // to the left-upper corner of a view with id releaseViewId.
    private void expectEndEventFailure6(int viewId, int releaseViewId) {
        View v = mActivity.findViewById(viewId);
        View release = mActivity.findViewById(releaseViewId);
        int [] releaseLoc = new int[2];
        release.getLocationOnScreen(releaseLoc);
        int action = DragEvent.ACTION_DRAG_ENDED;
        mExpected.add(new LogEntry(v, action,
                releaseLoc[0] + 6, releaseLoc[1] + 6, obtainClipData(action),
                obtainClipDescription(action), sLocalState, false));
    }

    // Add expected event for a view, with coordinates over view locationViewId, with the specified
    // offset from the location view's upper-left corner.
    private void expectEventWithOffset(int action, int viewId, int locationViewId, int offset) {
        View v = mActivity.findViewById(viewId);
        View locationView = mActivity.findViewById(locationViewId);
        int [] viewLocation = new int[2];
        v.getLocationOnScreen(viewLocation);
        int [] locationViewLocation = new int[2];
        locationView.getLocationOnScreen(locationViewLocation);
        mExpected.add(new LogEntry(v, action,
                locationViewLocation[0] - viewLocation[0] + offset,
                locationViewLocation[1] - viewLocation[1] + offset, obtainClipData(action),
                obtainClipDescription(action), sLocalState, false));
    }

    private void expectEvent5(int action, int viewId, int locationViewId) {
        expectEventWithOffset(action, viewId, locationViewId, 5);
    }

    // See comment for injectMouse6 on why we need both *5 and *6 methods.
    private void expectEvent6(int action, int viewId, int locationViewId) {
        expectEventWithOffset(action, viewId, locationViewId, 6);
    }

    // Inject mouse event over a given view, with specified offset from its left-upper corner.
    private void injectMouseWithOffset(int viewId, int action, int offset) {
        runOnMain(() -> {
            View v = mActivity.findViewById(viewId);
            int [] destLoc = new int [2];
            v.getLocationOnScreen(destLoc);
            long downTime = SystemClock.uptimeMillis();
            MotionEvent event = MotionEvent.obtain(downTime, downTime, action,
                    destLoc[0] + offset, destLoc[1] + offset, 1);
            event.setSource(InputDevice.SOURCE_TOUCHSCREEN);
            mAutomation.injectInputEvent(event, false);
        });

        // Wait till the mouse event generates drag events. Also, some waiting needed because the
        // system seems to collapse too frequent mouse events.
        try {
            Thread.sleep(100);
        } catch (Exception e) {
            fail("Exception while wait: " + e);
        }
    }

    // Inject mouse event over a given view, with offset 5 from its left-upper corner.
    private void injectMouse5(int viewId, int action) {
        injectMouseWithOffset(viewId, action, 5);
    }

    // Inject mouse event over a given view, with offset 6 from its left-upper corner.
    // We need both injectMouse5 and injectMouse6 if we want to inject 2 events in a row in the same
    // view, and want them to produce distinct drag events or simply drag events with different
    // coordinates.
    private void injectMouse6(int viewId, int action) {
        injectMouseWithOffset(viewId, action, 6);
    }

    private String logToString(ArrayList<LogEntry> log) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < log.size(); ++i) {
            LogEntry e = log.get(i);
            sb.append("#").append(i + 1).append(": ").append(e).append('\n');
        }
        return sb.toString();
    }

    private void failWithLogs(String message) {
        fail(message + ":\nExpected event sequence:\n" + logToString(mExpected) +
                "\nActual event sequence:\n" + logToString(mActual));
    }

    private void verifyEventLog() {
        try {
            assertTrue("Timeout while waiting for END event",
                    mEndReceived.await(1, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            fail("Got InterruptedException while waiting for END event");
        }

        // Verify the log.
        runOnMain(() -> {
            if (mExpected.size() != mActual.size()) {
                failWithLogs("Actual log has different size than expected");
            }

            for (int i = 0; i < mActual.size(); ++i) {
                if (!mActual.get(i).equals(mExpected.get(i))) {
                    failWithLogs("Actual event #" + (i + 1) + " is different from expected");
                }
            }
        });
    }

    private boolean init() {
        // Only run for non-watch devices
        if (mActivity.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            return false;
        }
        return true;
    }

    @Before
    public void setUp() {
        mActivity = mActivityRule.getActivity();
        mStartReceived = new CountDownLatch(1);
        mEndReceived = new CountDownLatch(1);

        // Wait for idle
        mInstrumentation.waitForIdleSync();
    }

    @After
    public void tearDown() throws Exception {
        mActual.clear();
        mExpected.clear();
    }

    // Sets handlers on all views in a tree, which log the event and return false.
    private void setRejectingHandlersOnTree(View v) {
        v.setOnDragListener((_v, ev) -> {
            logEvent(_v, ev);
            return false;
        });

        if (v instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) v;
            for (int i = 0; i < group.getChildCount(); ++i) {
                setRejectingHandlersOnTree(group.getChildAt(i));
            }
        }
    }

    private void runOnMain(Runnable runner) throws AssertionError {
        mMainThreadAssertionError = null;
        mInstrumentation.runOnMainSync(() -> {
            try {
                runner.run();
            } catch (AssertionError error) {
                mMainThreadAssertionError = error;
            }
        });
        if (mMainThreadAssertionError != null) {
            throw mMainThreadAssertionError;
        }
    }

    private void startDrag() {
        // Mouse down. Required for the drag to start.
        injectMouse5(R.id.draggable, MotionEvent.ACTION_DOWN);

        runOnMain(() -> {
            // Start drag.
            View v = mActivity.findViewById(R.id.draggable);
            assertTrue("Couldn't start drag",
                    v.startDragAndDrop(sClipData, new View.DragShadowBuilder(v), sLocalState, 0));
        });

        try {
            assertTrue("Timeout while waiting for START event",
                    mStartReceived.await(1, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            fail("Got InterruptedException while waiting for START event");
        }
    }

    /**
     * Tests that no drag-drop events are sent to views that aren't supposed to receive them.
     */
    @Test
    public void testNoExtraEvents() throws Exception {
        if (!init()) {
            return;
        }

        runOnMain(() -> {
            // Tell all views in layout to return false to all events, and log them.
            setRejectingHandlersOnTree(mActivity.findViewById(R.id.drag_drop_activity_main));

            // Override handlers for the inner view and its parent to return true.
            mActivity.findViewById(R.id.inner).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
            mActivity.findViewById(R.id.subcontainer).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
        });

        startDrag();

        // Move mouse to the outmost view. This shouldn't generate any events since it returned
        // false to STARTED.
        injectMouse5(R.id.container, MotionEvent.ACTION_MOVE);
        // Release mouse over the inner view. This produces DROP there.
        injectMouse5(R.id.inner, MotionEvent.ACTION_UP);

        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.inner, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.subcontainer, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.container, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.draggable, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.drag_drop_activity_main, R.id.draggable);

        expectEvent5(DragEvent.ACTION_DRAG_ENTERED, R.id.inner);
        expectEvent5(DragEvent.ACTION_DROP, R.id.inner, R.id.inner);

        expectEndEventSuccess(R.id.inner);
        expectEndEventSuccess(R.id.subcontainer);

        verifyEventLog();
    }

    /**
     * Tests events over a non-accepting view with an accepting child get delivered to that view's
     * parent.
     */
    @Test
    public void testBlackHole() throws Exception {
        if (!init()) {
            return;
        }

        runOnMain(() -> {
            // Accepting child.
            mActivity.findViewById(R.id.inner).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
            // Non-accepting parent of that child.
            mActivity.findViewById(R.id.subcontainer).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return false;
            });
            // Accepting parent of the previous view.
            mActivity.findViewById(R.id.container).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
        });

        startDrag();

        // Move mouse to the non-accepting view.
        injectMouse5(R.id.subcontainer, MotionEvent.ACTION_MOVE);
        // Release mouse over the non-accepting view, with different coordinates.
        injectMouse6(R.id.subcontainer, MotionEvent.ACTION_UP);

        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.inner, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.subcontainer, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.container, R.id.draggable);

        expectEvent5(DragEvent.ACTION_DRAG_ENTERED, R.id.container);
        expectEvent5(DragEvent.ACTION_DRAG_LOCATION, R.id.container, R.id.subcontainer);
        expectEvent6(DragEvent.ACTION_DROP, R.id.container, R.id.subcontainer);

        expectEndEventSuccess(R.id.inner);
        expectEndEventSuccess(R.id.container);

        verifyEventLog();
    }

    /**
     * Tests generation of ENTER/EXIT events.
     */
    @Test
    public void testEnterExit() throws Exception {
        if (!init()) {
            return;
        }

        runOnMain(() -> {
            // The setup is same as for testBlackHole.

            // Accepting child.
            mActivity.findViewById(R.id.inner).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
            // Non-accepting parent of that child.
            mActivity.findViewById(R.id.subcontainer).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return false;
            });
            // Accepting parent of the previous view.
            mActivity.findViewById(R.id.container).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });

        });

        startDrag();

        // Move mouse to the non-accepting view, then to the inner one, then back to the
        // non-accepting view, then release over the inner.
        injectMouse5(R.id.subcontainer, MotionEvent.ACTION_MOVE);
        injectMouse5(R.id.inner, MotionEvent.ACTION_MOVE);
        injectMouse5(R.id.subcontainer, MotionEvent.ACTION_MOVE);
        injectMouse5(R.id.inner, MotionEvent.ACTION_UP);

        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.inner, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.subcontainer, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.container, R.id.draggable);

        expectEvent5(DragEvent.ACTION_DRAG_ENTERED, R.id.container);
        expectEvent5(DragEvent.ACTION_DRAG_LOCATION, R.id.container, R.id.subcontainer);
        expectEvent5(DragEvent.ACTION_DRAG_EXITED, R.id.container);

        expectEvent5(DragEvent.ACTION_DRAG_ENTERED, R.id.inner);
        expectEvent5(DragEvent.ACTION_DRAG_LOCATION, R.id.inner, R.id.inner);
        expectEvent5(DragEvent.ACTION_DRAG_EXITED, R.id.inner);

        expectEvent5(DragEvent.ACTION_DRAG_ENTERED, R.id.container);
        expectEvent5(DragEvent.ACTION_DRAG_LOCATION, R.id.container, R.id.subcontainer);
        expectEvent5(DragEvent.ACTION_DRAG_EXITED, R.id.container);

        expectEvent5(DragEvent.ACTION_DRAG_ENTERED, R.id.inner);
        expectEvent5(DragEvent.ACTION_DROP, R.id.inner, R.id.inner);

        expectEndEventSuccess(R.id.inner);
        expectEndEventSuccess(R.id.container);

        verifyEventLog();
    }
    /**
     * Tests events over a non-accepting view that has no accepting ancestors.
     */
    @Test
    public void testOverNowhere() throws Exception {
        if (!init()) {
            return;
        }

        runOnMain(() -> {
            // Accepting child.
            mActivity.findViewById(R.id.inner).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
            // Non-accepting parent of that child.
            mActivity.findViewById(R.id.subcontainer).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return false;
            });
        });

        startDrag();

        // Move mouse to the non-accepting view, then to accepting view, and back, and drop there.
        injectMouse5(R.id.subcontainer, MotionEvent.ACTION_MOVE);
        injectMouse5(R.id.inner, MotionEvent.ACTION_MOVE);
        injectMouse5(R.id.subcontainer, MotionEvent.ACTION_MOVE);
        injectMouse6(R.id.subcontainer, MotionEvent.ACTION_UP);

        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.inner, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.subcontainer, R.id.draggable);

        expectEvent5(DragEvent.ACTION_DRAG_ENTERED, R.id.inner);
        expectEvent5(DragEvent.ACTION_DRAG_LOCATION, R.id.inner, R.id.inner);
        expectEvent5(DragEvent.ACTION_DRAG_EXITED, R.id.inner);

        expectEndEventFailure6(R.id.inner, R.id.subcontainer);

        verifyEventLog();
    }

    /**
     * Tests that events are properly delivered to a view that is in the middle of the accepting
     * hierarchy.
     */
    @Test
    public void testAcceptingGroupInTheMiddle() throws Exception {
        if (!init()) {
            return;
        }

        runOnMain(() -> {
            // Set accepting handlers to the inner view and its 2 ancestors.
            mActivity.findViewById(R.id.inner).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
            mActivity.findViewById(R.id.subcontainer).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
            mActivity.findViewById(R.id.container).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
        });

        startDrag();

        // Move mouse to the outmost container, then move to the subcontainer and drop there.
        injectMouse5(R.id.container, MotionEvent.ACTION_MOVE);
        injectMouse5(R.id.subcontainer, MotionEvent.ACTION_MOVE);
        injectMouse6(R.id.subcontainer, MotionEvent.ACTION_UP);

        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.inner, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.subcontainer, R.id.draggable);
        expectEvent5(DragEvent.ACTION_DRAG_STARTED, R.id.container, R.id.draggable);

        expectEvent5(DragEvent.ACTION_DRAG_ENTERED, R.id.container);
        expectEvent5(DragEvent.ACTION_DRAG_LOCATION, R.id.container, R.id.container);
        expectEvent5(DragEvent.ACTION_DRAG_EXITED, R.id.container);

        expectEvent5(DragEvent.ACTION_DRAG_ENTERED, R.id.subcontainer);
        expectEvent5(DragEvent.ACTION_DRAG_LOCATION, R.id.subcontainer, R.id.subcontainer);
        expectEvent6(DragEvent.ACTION_DROP, R.id.subcontainer, R.id.subcontainer);

        expectEndEventSuccess(R.id.inner);
        expectEndEventSuccess(R.id.subcontainer);
        expectEndEventSuccess(R.id.container);

        verifyEventLog();
    }

    private boolean drawableStateContains(int resourceId, int attr) {
        return IntStream.of(mActivity.findViewById(resourceId).getDrawableState())
                .anyMatch(x -> x == attr);
    }

    /**
     * Tests that state_drag_hovered and state_drag_can_accept are set correctly.
     */
    @Test
    public void testDrawableState() throws Exception {
        if (!init()) {
            return;
        }

        runOnMain(() -> {
            // Set accepting handler for the inner view.
            mActivity.findViewById(R.id.inner).setOnDragListener((v, ev) -> {
                logEvent(v, ev);
                return true;
            });
            assertFalse(drawableStateContains(R.id.inner, android.R.attr.state_drag_can_accept));
        });

        startDrag();

        runOnMain(() -> {
            assertFalse(drawableStateContains(R.id.inner, android.R.attr.state_drag_hovered));
            assertTrue(drawableStateContains(R.id.inner, android.R.attr.state_drag_can_accept));
        });

        // Move mouse into the view.
        injectMouse5(R.id.inner, MotionEvent.ACTION_MOVE);
        runOnMain(() -> {
            assertTrue(drawableStateContains(R.id.inner, android.R.attr.state_drag_hovered));
        });

        // Move out.
        injectMouse5(R.id.subcontainer, MotionEvent.ACTION_MOVE);
        runOnMain(() -> {
            assertFalse(drawableStateContains(R.id.inner, android.R.attr.state_drag_hovered));
        });

        // Move in.
        injectMouse5(R.id.inner, MotionEvent.ACTION_MOVE);
        runOnMain(() -> {
            assertTrue(drawableStateContains(R.id.inner, android.R.attr.state_drag_hovered));
        });

        // Release there.
        injectMouse5(R.id.inner, MotionEvent.ACTION_UP);
        runOnMain(() -> {
            assertFalse(drawableStateContains(R.id.inner, android.R.attr.state_drag_hovered));
        });
    }
}