/*
 * Copyright (C) 2008 The Android Open Source Project
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
package android.app.cts;

import android.app.Dialog;
import android.app.Instrumentation;
import android.app.stubs.DialogStubActivity;
import android.app.stubs.OrientationTestUtils;
import android.app.stubs.TestDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnDismissListener;
import android.content.DialogInterface.OnKeyListener;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.support.test.annotation.UiThreadTest;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.LinearLayout;

import android.app.stubs.R;

import com.android.compatibility.common.util.PollingCheck;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.lang.ref.WeakReference;

@RunWith(AndroidJUnit4.class)
public class DialogTest {

    protected static final long SLEEP_TIME = 200;
    private static final long TEST_TIMEOUT = 1000L;

    /**
     *  please refer to Dialog
     */
    private static final int DISMISS = 0x43;
    private static final int CANCEL = 0x44;

    private boolean mCalledCallback;
    private boolean mIsKey0Listened;
    private boolean mIsKey1Listened;
    private boolean mOnCancelListenerCalled;

    private Instrumentation mInstrumentation;
    private Context mContext;
    private DialogStubActivity mActivity;

    @Rule
    public ActivityTestRule<DialogStubActivity> mActivityRule =
            new ActivityTestRule<>(DialogStubActivity.class, false, false);

    @Before
    public void setup() throws Throwable {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mContext = mInstrumentation.getContext();
        mInstrumentation.waitForIdleSync();
    }

    private void startDialogActivity(int dialogNumber) {
        mActivity = DialogStubActivity.startDialogActivity(mActivityRule, dialogNumber);
    }

    @UiThreadTest
    @Test
    public void testConstructor() {
        new Dialog(mContext);
        Dialog d = new Dialog(mContext, 0);
        // According to javadoc of constructors, it will set theme to system default theme,
        // when we set no theme id or set it theme id to 0.
        // But CTS can no assert dialog theme equals system internal theme.

        d = new Dialog(mContext, R.style.TextAppearance);
        TypedArray ta =
            d.getContext().getTheme().obtainStyledAttributes(R.styleable.TextAppearance);
        assertTextAppearanceStyle(ta);

        final Window w = d.getWindow();
        ta = w.getContext().getTheme().obtainStyledAttributes(R.styleable.TextAppearance);
        assertTextAppearanceStyle(ta);
    }

    @Test
    public void testConstructor_protectedCancellable() {
        startDialogActivity(DialogStubActivity.TEST_PROTECTED_CANCELABLE);
        mActivity.onCancelListenerCalled = false;
        sendKeys(KeyEvent.KEYCODE_BACK);
        assertTrue(mActivity.onCancelListenerCalled);
    }

    @Test
    public void testConstructor_protectedNotCancellable() {
        startDialogActivity(DialogStubActivity.TEST_PROTECTED_NOT_CANCELABLE);
        mActivity.onCancelListenerCalled = false;
        sendKeys(KeyEvent.KEYCODE_BACK);
        assertFalse(mActivity.onCancelListenerCalled);
    }

    @Test
    public void testConstructor_protectedCancellableEsc() {
        startDialogActivity(DialogStubActivity.TEST_PROTECTED_CANCELABLE);
        mActivity.onCancelListenerCalled = false;
        sendKeys(KeyEvent.KEYCODE_ESCAPE);
        assertTrue(mActivity.onCancelListenerCalled);
    }

    @Test
    public void testConstructor_protectedNotCancellableEsc() {
        startDialogActivity(DialogStubActivity.TEST_PROTECTED_NOT_CANCELABLE);
        mActivity.onCancelListenerCalled = false;
        sendKeys(KeyEvent.KEYCODE_ESCAPE);
        assertFalse(mActivity.onCancelListenerCalled);
    }

    private void assertTextAppearanceStyle(TypedArray ta) {
        final int defValue = -1;
        // get Theme and assert
        final Resources.Theme expected = mContext.getResources().newTheme();
        expected.setTo(mContext.getTheme());
        expected.applyStyle(R.style.TextAppearance, true);
        TypedArray expectedTa = expected.obtainStyledAttributes(R.styleable.TextAppearance);
        assertEquals(expectedTa.getIndexCount(), ta.getIndexCount());
        assertEquals(expectedTa.getColor(R.styleable.TextAppearance_textColor, defValue),
                ta.getColor(R.styleable.TextAppearance_textColor, defValue));
        assertEquals(expectedTa.getColor(R.styleable.TextAppearance_textColorHint, defValue),
                ta.getColor(R.styleable.TextAppearance_textColorHint, defValue));
        assertEquals(expectedTa.getColor(R.styleable.TextAppearance_textColorLink, defValue),
                ta.getColor(R.styleable.TextAppearance_textColorLink, defValue));
        assertEquals(expectedTa.getColor(R.styleable.TextAppearance_textColorHighlight, defValue),
                ta.getColor(R.styleable.TextAppearance_textColorHighlight, defValue));
        assertEquals(expectedTa.getDimension(R.styleable.TextAppearance_textSize, defValue),
                ta.getDimension(R.styleable.TextAppearance_textSize, defValue), Float.MIN_VALUE);
        assertEquals(expectedTa.getInt(R.styleable.TextAppearance_textStyle, defValue),
                ta.getInt(R.styleable.TextAppearance_textStyle, defValue));
    }

    @Test
    public void testOnStartCreateStop(){
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();

        assertTrue(d.isOnStartCalled);
        assertTrue(d.isOnCreateCalled);

        assertFalse(d.isOnStopCalled);
        sendKeys(KeyEvent.KEYCODE_BACK);
        assertTrue(d.isOnStopCalled);
    }

    @Test
    public void testOnStartCreateStopEsc(){
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();

        assertTrue(d.isOnStartCalled);
        assertTrue(d.isOnCreateCalled);

        assertFalse(d.isOnStopCalled);
        sendKeys(KeyEvent.KEYCODE_ESCAPE);
        assertTrue(d.isOnStopCalled);
    }

    @Test
    public void testAccessOwnerActivity() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        Dialog d = mActivity.getDialog();
        assertNotNull(d);
        assertSame(mActivity, d.getOwnerActivity());
        d.setVolumeControlStream(d.getVolumeControlStream() + 1);
        assertEquals(d.getOwnerActivity().getVolumeControlStream() + 1, d.getVolumeControlStream());

        try {
            d.setOwnerActivity(null);
            fail("Should throw NullPointerException");
        } catch (NullPointerException e) {
            // expected
        }

        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                Dialog dialog = new Dialog(mContext);
                assertNull(dialog.getOwnerActivity());
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    @Test
    public void testShow() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();
        final View decor = d.getWindow().getDecorView();

        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.hide();
            }
        });
        mInstrumentation.waitForIdleSync();

        assertEquals(View.GONE, decor.getVisibility());
        assertTrue(d.isShowing());

        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.show();
            }
        });
        mInstrumentation.waitForIdleSync();

        assertEquals(View.VISIBLE, decor.getVisibility());
        assertTrue(d.isShowing());
        dialogDismiss(d);
        assertFalse(d.isShowing());
    }

    @Test
    public void testOnSaveInstanceState() throws InterruptedException {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();

        //skip if the device doesn't support both of portrait and landscape orientation screens.
        final PackageManager pm = mContext.getPackageManager();
        if(!(pm.hasSystemFeature(PackageManager.FEATURE_SCREEN_LANDSCAPE)
                && pm.hasSystemFeature(PackageManager.FEATURE_SCREEN_PORTRAIT))){
            return;
        }

        d.onSaveInstanceStateObserver.startObserving();
        TestDialog.onRestoreInstanceStateObserver.startObserving();
        OrientationTestUtils.toggleOrientation(mActivity);
        d.onSaveInstanceStateObserver.await();
        TestDialog.onRestoreInstanceStateObserver.await();
    }

    @Test
    public void testGetCurrentFocus() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();
        assertNull(d.getCurrentFocus());

        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.takeKeyEvents(true);
                d.setContentView(R.layout.alert_dialog_text_entry);
            }
        });
        mInstrumentation.waitForIdleSync();

        sendKeys(KeyEvent.KEYCODE_0);
        // When mWindow is not null getCurrentFocus is the view in dialog
        assertEquals(d.getWindow().getCurrentFocus(), d.getCurrentFocus());
    }

    @Test
    public void testSetContentView() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();
        assertNotNull(d);

        // set content view to a four elements layout
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.setContentView(R.layout.alert_dialog_text_entry);
            }
        });
        mInstrumentation.waitForIdleSync();

        // check if four elements are right there
        assertNotNull(d.findViewById(R.id.username_view));
        assertNotNull(d.findViewById(R.id.username_edit));
        assertNotNull(d.findViewById(R.id.password_view));
        assertNotNull(d.findViewById(R.id.password_edit));

        final LayoutInflater inflate1 = d.getLayoutInflater();

        // set content view to a two elements layout
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.setContentView(inflate1.inflate(R.layout.alert_dialog_text_entry_2, null));
            }
        });
        mInstrumentation.waitForIdleSync();

        // check if only two elements are right there
        assertNotNull(d.findViewById(R.id.username_view));
        assertNotNull(d.findViewById(R.id.username_edit));
        assertNull(d.findViewById(R.id.password_view));
        assertNull(d.findViewById(R.id.password_edit));

        final WindowManager.LayoutParams lp = d.getWindow().getAttributes();
        final LayoutInflater inflate2 = mActivity.getLayoutInflater();

        // set content view to a four elements layout
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.setContentView(inflate2.inflate(R.layout.alert_dialog_text_entry, null), lp);
            }
        });
        mInstrumentation.waitForIdleSync();

        // check if four elements are right there
        assertNotNull(d.findViewById(R.id.username_view));
        assertNotNull(d.findViewById(R.id.username_edit));
        assertNotNull(d.findViewById(R.id.password_view));
        assertNotNull(d.findViewById(R.id.password_edit));

        final WindowManager.LayoutParams lp2 = d.getWindow().getAttributes();
        final LayoutInflater inflate3 = mActivity.getLayoutInflater();
        lp2.height = ViewGroup.LayoutParams.WRAP_CONTENT;
        lp2.width = ViewGroup.LayoutParams.WRAP_CONTENT;

        // add a check box view
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.addContentView(inflate3.inflate(R.layout.checkbox_layout, null), lp2);
            }
        });
        mInstrumentation.waitForIdleSync();

        // check if four elements are right there, and new add view there.
        assertNotNull(d.findViewById(R.id.check_box));
        assertNotNull(d.findViewById(R.id.username_view));
        assertNotNull(d.findViewById(R.id.username_edit));
        assertNotNull(d.findViewById(R.id.password_view));
        assertNotNull(d.findViewById(R.id.password_edit));
    }

    @Test
    public void testRequireViewById() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();
        assertNotNull(d);

        // set content view to a four elements layout
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.setContentView(R.layout.alert_dialog_text_entry);
            }
        });
        mInstrumentation.waitForIdleSync();

        // check if four elements are right there
        assertNotNull(d.requireViewById(R.id.username_view));
        assertNotNull(d.requireViewById(R.id.username_edit));
        assertNotNull(d.requireViewById(R.id.password_view));
        assertNotNull(d.requireViewById(R.id.password_edit));
        try {
            d.requireViewById(R.id.check_box); // not present
            fail("should not get here, check_box should not be found");
        } catch (IllegalArgumentException e) {
            // expected
        }
        try {
            d.requireViewById(View.NO_ID); // invalid
            fail("should not get here, NO_ID should not be found");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }


    @Test
    public void testSetTitle() {
        final String expectedTitle = "Test Dialog Without theme";
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);

        assertNotNull(mActivity.getDialog());
        mActivity.setUpTitle(expectedTitle);
        mInstrumentation.waitForIdleSync();

        final Dialog d = mActivity.getDialog();
        assertEquals(expectedTitle, (String) d.getWindow().getAttributes().getTitle());

        mActivity.setUpTitle(R.string.hello_android);
        mInstrumentation.waitForIdleSync();
        assertEquals(mActivity.getResources().getString(R.string.hello_android),
                (String) d.getWindow().getAttributes().getTitle());
    }

    @Test
    public void testOnKeyDownKeyUp() {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();
        assertFalse(d.isOnKeyDownCalled);
        assertFalse(d.isOnKeyUpCalled);

        // send key 0 down and up events, onKeyDown return false
        mInstrumentation.sendKeyDownUpSync(KeyEvent.KEYCODE_0);
        assertTrue(d.isOnKeyDownCalled);
        assertTrue(d.isOnKeyUpCalled);
        assertEquals(KeyEvent.KEYCODE_0, d.keyDownCode);
        assertFalse(d.onKeyDownReturn);

        // send key back down and up events, onKeyDown return true
        mInstrumentation.sendKeyDownUpSync(KeyEvent.KEYCODE_BACK);
        assertEquals(KeyEvent.KEYCODE_BACK, d.keyDownCode);
        assertTrue(d.onKeyDownReturn);
    }

    @Test
    public void testOnKeyMultiple() {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();

        assertNull(d.keyMultipleEvent);
        d.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_MULTIPLE, KeyEvent.KEYCODE_UNKNOWN));
        assertTrue(d.isOnKeyMultipleCalled);
        assertFalse(d.onKeyMultipleReturn);
        assertEquals(KeyEvent.KEYCODE_UNKNOWN, d.keyMultipleEvent.getKeyCode());
        assertEquals(KeyEvent.ACTION_MULTIPLE, d.keyMultipleEvent.getAction());
    }

    private MotionEvent sendTouchEvent(long downTime, int action, float x, float y) {
        long eventTime = downTime;
        if (action != MotionEvent.ACTION_DOWN) {
            eventTime += 1;
        }
        MotionEvent event = MotionEvent.obtain(downTime, eventTime, action, x, y, 0);
        event.setSource(InputDevice.SOURCE_TOUCHSCREEN);
        mInstrumentation.getUiAutomation().injectInputEvent(event, true);
        mInstrumentation.waitForIdleSync();
        return event;
    }

    @Test
    public void testTouchEvent() {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();
        final Rect containingRect = new Rect();
        mActivity.getWindow().getDecorView().getWindowVisibleDisplayFrame(containingRect);
        final int x = containingRect.left + 1;
        final int y = containingRect.top + 1;

        assertNull(d.onTouchEvent);
        assertNull(d.touchEvent);
        assertFalse(d.isOnTouchEventCalled);

        // Send a touch event outside the dialog window.  Expect the event to be ignored
        // because closeOnTouchOutside is false.
        d.setCanceledOnTouchOutside(false);

        long now = SystemClock.uptimeMillis();
        MotionEvent touchMotionEvent = sendTouchEvent(now, MotionEvent.ACTION_DOWN, x, y);

        new PollingCheck(TEST_TIMEOUT) {
            protected boolean check() {
                return !d.dispatchTouchEventResult;
            }
        }.run();

        assertMotionEventEquals(touchMotionEvent, d.touchEvent);
        assertTrue(d.isOnTouchEventCalled);
        assertMotionEventEquals(touchMotionEvent, d.onTouchEvent);
        d.isOnTouchEventCalled = false;
        assertTrue(d.isShowing());
        touchMotionEvent.recycle();
        // Send ACTION_UP to keep the event stream consistent
        sendTouchEvent(now, MotionEvent.ACTION_UP, x, y).recycle();

        if (mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            // Watch activities cover the entire screen, so there is no way to touch outside.
            return;
        }

        // Send a touch event outside the dialog window. Expect the dialog to be dismissed
        // because closeOnTouchOutside is true.

        d.setCanceledOnTouchOutside(true);
        now = SystemClock.uptimeMillis();
        touchMotionEvent = sendTouchEvent(now, MotionEvent.ACTION_DOWN, x, y);

        new PollingCheck(TEST_TIMEOUT) {
            protected boolean check() {
                return d.dispatchTouchEventResult;
            }
        }.run();

        assertMotionEventEquals(touchMotionEvent, d.touchEvent);
        assertTrue(d.isOnTouchEventCalled);
        assertMotionEventEquals(touchMotionEvent, d.onTouchEvent);
        assertFalse(d.isShowing());
        touchMotionEvent.recycle();
        sendTouchEvent(now, MotionEvent.ACTION_UP, x, y).recycle();
    }

    @Test
    public void testTrackballEvent() {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();
        long eventTime = SystemClock.uptimeMillis();
        final MotionEvent trackBallEvent = MotionEvent.obtain(eventTime, eventTime,
                MotionEvent.ACTION_DOWN, 0.0f, 0.0f, 0);

        assertNull(d.trackballEvent);
        assertNull(d.onTrackballEvent);

        assertFalse(d.isOnTrackballEventCalled);
        mInstrumentation.sendTrackballEventSync(trackBallEvent);
        assertTrue(d.isOnTrackballEventCalled);
        assertMotionEventEquals(trackBallEvent, d.trackballEvent);
        assertMotionEventEquals(trackBallEvent, d.onTrackballEvent);

    }

    private void assertMotionEventEquals(final MotionEvent expected, final MotionEvent actual) {
        assertNotNull(actual);
        assertEquals(expected.getDownTime(), actual.getDownTime());
        assertEquals(expected.getEventTime(), actual.getEventTime());
        assertEquals(expected.getAction(), actual.getAction());
        assertEquals(expected.getMetaState(), actual.getMetaState());
        assertEquals(expected.getSize(), actual.getSize(), Float.MIN_VALUE);
        // As MotionEvent doc says the value of X and Y coordinate may have
        // a fraction for input devices that are sub-pixel precise,
        // so we won't assert them here.
    }

    @Test
    public void testOnWindowAttributesChanged() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();

        assertTrue(d.isOnWindowAttributesChangedCalled);
        d.isOnWindowAttributesChangedCalled = false;

        final WindowManager.LayoutParams lp = d.getWindow().getAttributes();
        lp.setTitle("test OnWindowAttributesChanged");
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.getWindow().setAttributes(lp);
            }
        });
        mInstrumentation.waitForIdleSync();

        assertTrue(d.isOnWindowAttributesChangedCalled);
        assertSame(lp, d.getWindow().getAttributes());
    }

    @Test
    public void testOnContentChanged() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();
        assertNotNull(d);

        assertFalse(d.isOnContentChangedCalled);

        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.setContentView(R.layout.alert_dialog_text_entry);
            }
        });
        mInstrumentation.waitForIdleSync();

        assertTrue(d.isOnContentChangedCalled);
    }

    @Test
    public void testOnWindowFocusChanged() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();
        assertTrue(d.isOnWindowFocusChangedCalled);
        d.isOnWindowFocusChangedCalled = false;

        // show a new dialog, the new dialog get focus
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                mActivity.showDialog(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
            }
        });
        mInstrumentation.waitForIdleSync();

        // Wait until TestDialog#OnWindowFocusChanged() is called
        new PollingCheck(TEST_TIMEOUT) {
            protected boolean check() {
                return d.isOnWindowFocusChangedCalled;
            }
        }.run();
    }

    @Test
    public void testDispatchKeyEvent() {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();

        sendKeys(KeyEvent.KEYCODE_0);
        assertFalse(d.dispatchKeyEventResult);
        assertEquals(KeyEvent.KEYCODE_0, d.keyEvent.getKeyCode());

        d.setOnKeyListener(new OnKeyListener() {
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                if (KeyEvent.ACTION_DOWN == event.getAction()) {
                    if (KeyEvent.KEYCODE_0 == keyCode) {
                        mIsKey0Listened = true;
                        return true;
                    }

                    if (KeyEvent.KEYCODE_1 == keyCode) {
                        mIsKey1Listened = true;
                        return true;
                    }
                }

                return false;
            }
        });

        mIsKey1Listened = false;
        sendKeys(KeyEvent.KEYCODE_1);
        assertTrue(mIsKey1Listened);

        mIsKey0Listened = false;
        sendKeys(KeyEvent.KEYCODE_0);
        assertTrue(mIsKey0Listened);
    }

    /*
     * Test point
     * 1. registerForContextMenu() will OnCreateContextMenuListener on the view to this activity,
     * so onCreateContextMenu() will be called when it is time to show the context menu.
     * 2. Close context menu will make onPanelClosed to be called,
     * and onPanelClosed will calls through to the new onPanelClosed method.
     * 3. unregisterForContextMenu() will remove the OnCreateContextMenuListener on the view,
     * so onCreateContextMenu() will not be called when try to open context menu.
     * 4. Selected a item of context menu will make onMenuItemSelected() to be called,
     * and onMenuItemSelected will calls through to the new onContextItemSelected method.
     * 5. onContextMenuClosed is called whenever the context menu is being closed (either by
     * the user canceling the menu with the back/menu button, or when an item is selected).
     */
    @Test
    public void testContextMenu() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();
        final LinearLayout parent = new LinearLayout(mContext);
        final MockView v = new MockView(mContext);
        parent.addView(v);
        assertFalse(v.isShowContextMenuCalled);
        // Register for context menu and open it
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.addContentView(parent, new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT));
                d.registerForContextMenu(v);
                d.openContextMenu(v);
            }
        });
        mInstrumentation.waitForIdleSync();

        assertTrue(v.isShowContextMenuCalled);
        assertTrue(d.isOnCreateContextMenuCalled);

        assertFalse(d.isOnPanelClosedCalled);
        assertFalse(d.isOnContextMenuClosedCalled);
        // Closed context menu
        sendKeys(KeyEvent.KEYCODE_BACK);
        assertTrue(d.isOnPanelClosedCalled);
        // Here isOnContextMenuClosedCalled should be true, see bug 1716918.
        assertFalse(d.isOnContextMenuClosedCalled);

        v.isShowContextMenuCalled = false;
        d.isOnCreateContextMenuCalled = false;
        // Unregister for context menu, and try to open it
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.unregisterForContextMenu(v);
            }
        });
        mInstrumentation.waitForIdleSync();

        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.openContextMenu(v);
            }
        });
        mInstrumentation.waitForIdleSync();

        assertTrue(v.isShowContextMenuCalled);
        assertFalse(d.isOnCreateContextMenuCalled);

        // Register for context menu and open it again
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.registerForContextMenu(v);
                d.openContextMenu(v);
            }
        });
        mInstrumentation.waitForIdleSync();

        assertFalse(d.isOnContextItemSelectedCalled);
        assertFalse(d.isOnMenuItemSelectedCalled);
        d.isOnPanelClosedCalled = false;
        assertFalse(d.isOnContextMenuClosedCalled);
        // select a context menu item
        sendKeys(KeyEvent.KEYCODE_DPAD_CENTER);
        assertTrue(d.isOnMenuItemSelectedCalled);
        // Here isOnContextItemSelectedCalled should be true, see bug 1716918.
        assertFalse(d.isOnContextItemSelectedCalled);
        assertTrue(d.isOnPanelClosedCalled);
        // Here isOnContextMenuClosedCalled should be true, see bug 1716918.
        assertFalse(d.isOnContextMenuClosedCalled);
    }

    @Test
    public void testOnSearchRequested() {
    }

    @Test
    public void testTakeKeyEvents() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();
        final View v = d.getWindow().getDecorView();
        assertNull(d.getCurrentFocus());
        takeKeyEvents(d, true);
        assertTrue(v.isFocusable());
        sendKeys(KeyEvent.KEYCODE_0);
        assertEquals(KeyEvent.KEYCODE_0, d.keyEvent.getKeyCode());
        d.keyEvent = null;

        takeKeyEvents(d, false);
        assertNull(d.getCurrentFocus());
        assertFalse(v.isFocusable());
        sendKeys(KeyEvent.KEYCODE_0);
        // d.keyEvent should be null
    }

    private void takeKeyEvents(final Dialog d, final boolean get) throws Throwable {
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.takeKeyEvents(get);
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    @Test
    public void testRequestWindowFeature() {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        // called requestWindowFeature at TestDialog onCreate method
        assertTrue(((TestDialog) mActivity.getDialog()).isRequestWindowFeature);
    }

    @Test
    public void testSetFeatureDrawableResource() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                mActivity.getDialog().setFeatureDrawableResource(Window.FEATURE_LEFT_ICON,
                        R.drawable.robot);
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    @Test
    public void testSetFeatureDrawableUri() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                mActivity.getDialog().setFeatureDrawableUri(Window.FEATURE_LEFT_ICON,
                        Uri.parse("http://www.google.com"));
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    @Test
    public void testSetFeatureDrawable() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                mActivity.getDialog().setFeatureDrawable(Window.FEATURE_LEFT_ICON, 
                        new MockDrawable());
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    @Test
    public void testSetFeatureDrawableAlpha() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                mActivity.getDialog().setFeatureDrawableAlpha(Window.FEATURE_LEFT_ICON, 0);
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    @Test
    public void testGetLayoutInflater() {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();
        assertEquals(d.getWindow().getLayoutInflater(), d.getLayoutInflater());
    }

    @Test
    public void testSetCancellable_true() {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();

        d.setCancelable(true);
        assertTrue(d.isShowing());
        sendKeys(KeyEvent.KEYCODE_BACK);
        assertFalse(d.isShowing());
    }

    @Test
    public void testSetCancellable_false() {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();

        d.setCancelable(false);
        assertTrue(d.isShowing());
        sendKeys(KeyEvent.KEYCODE_BACK);
        assertTrue(d.isShowing());
    }

    @Test
    public void testSetCancellableEsc_true() {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();

        d.setCancelable(true);
        assertTrue(d.isShowing());
        sendKeys(KeyEvent.KEYCODE_ESCAPE);
        assertFalse(d.isShowing());
    }

    @Test
    public void testSetCancellableEsc_false() {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();

        d.setCancelable(false);
        assertTrue(d.isShowing());
        sendKeys(KeyEvent.KEYCODE_ESCAPE);
        assertTrue(d.isShowing());
    }

    /*
     * Test point
     * 1. Cancel the dialog.
     * 2. Set a listener to be invoked when the dialog is canceled.
     */
    @Test
    public void testCancel_listener() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();

        assertTrue(d.isShowing());
        mOnCancelListenerCalled = false;

        d.setOnCancelListener(new OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
                mOnCancelListenerCalled = true;
            }
        });
        dialogCancel(d);

        assertFalse(d.isShowing());
        assertTrue(mOnCancelListenerCalled);
    }

    @Test
    public void testCancel_noListener() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();

        assertTrue(d.isShowing());
        mOnCancelListenerCalled = false;
        d.setOnCancelListener(null);
        dialogCancel(d);

        assertFalse(d.isShowing());
        assertFalse(mOnCancelListenerCalled);
    }

    @Test
    public void testSetCancelMessage() throws Exception {
        mCalledCallback = false;
        startDialogActivity(DialogStubActivity.TEST_ONSTART_AND_ONSTOP);
        final TestDialog d = (TestDialog) mActivity.getDialog();
        final HandlerThread ht = new HandlerThread("DialogTest");
        ht.start();

        d.setCancelMessage(new MockDismissCancelHandler(d, ht.getLooper()).obtainMessage(CANCEL,
                new OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {
                        mCalledCallback = true;
                    }
                }));
        assertTrue(d.isShowing());
        assertFalse(mCalledCallback);
        sendKeys(KeyEvent.KEYCODE_BACK);
        assertTrue(mCalledCallback);
        assertFalse(d.isShowing());

        ht.join(100);
    }

    /*
     * Test point
     * 1. Set a listener to be invoked when the dialog is dismissed.
     * 2. set onDismissListener to null, it will not changed flag after dialog dismissed.
     */
    @Test
    public void testSetOnDismissListener_listener() throws Throwable {
        mCalledCallback = false;
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();

        d.setOnDismissListener(new OnDismissListener() {
            public void onDismiss(DialogInterface dialog) {
                mCalledCallback = true;
            }
        });

        assertTrue(d.isShowing());
        assertFalse(mCalledCallback);
        dialogDismiss(d);
        assertTrue(mCalledCallback);
        assertFalse(d.isShowing());
    }

    @Test
    public void testSetOnDismissListener_noListener() throws Throwable {
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();
        assertTrue(d.isShowing());
        mCalledCallback = false;
        d.setOnDismissListener(null);
        dialogDismiss(d);
        assertFalse(mCalledCallback);
        assertFalse(d.isShowing());
    }

    @Test
    public void testSetDismissMessage() throws Throwable {
        mCalledCallback = false;
        startDialogActivity(DialogStubActivity.TEST_DIALOG_WITHOUT_THEME);
        final Dialog d = mActivity.getDialog();

        final HandlerThread ht = new HandlerThread("DialogTest");
        ht.start();

        d.setDismissMessage(new MockDismissCancelHandler(d, ht.getLooper()).obtainMessage(DISMISS,
                new OnDismissListener() {
                    public void onDismiss(DialogInterface dialog) {
                        mCalledCallback = true;
                    }
                }));
        assertTrue(d.isShowing());
        assertFalse(mCalledCallback);
        dialogDismiss(d);
        ht.join(100);
        assertTrue(mCalledCallback);
        assertFalse(d.isShowing());
    }

    private void dialogDismiss(final Dialog d) throws Throwable {
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.dismiss();
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    private void dialogCancel(final Dialog d) throws Throwable {
        mActivityRule.runOnUiThread(new Runnable() {
            public void run() {
                d.cancel();
            }
        });
        mInstrumentation.waitForIdleSync();
    }

    private void sendKeys(int keyCode) {
        mInstrumentation.sendKeyDownUpSync(keyCode);
        mInstrumentation.waitForIdleSync();
    }

    private static class MockDismissCancelHandler extends Handler {
        private WeakReference<DialogInterface> mDialog;

        public MockDismissCancelHandler(Dialog dialog, Looper looper) {
            super(looper);

            mDialog = new WeakReference<DialogInterface>(dialog);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case DISMISS:
                ((OnDismissListener) msg.obj).onDismiss(mDialog.get());
                break;
            case CANCEL:
                ((OnCancelListener) msg.obj).onCancel(mDialog.get());
                break;
            }
        }
    }

    private static class MockDrawable extends Drawable {
        @Override
        public void draw(Canvas canvas) {
        }

        @Override
        public int getOpacity() {
            return 0;
        }

        @Override
        public void setAlpha(int alpha) {
        }

        @Override
        public void setColorFilter(ColorFilter cf) {
        }
    }

    private static class MockView extends View {
        public boolean isShowContextMenuCalled;
        protected OnCreateContextMenuListener mOnCreateContextMenuListener;

        public MockView(Context context) {
            super(context);
        }

        public void setOnCreateContextMenuListener(OnCreateContextMenuListener l) {
            super.setOnCreateContextMenuListener(l);
            mOnCreateContextMenuListener = l;
        }

        @Override
        public boolean showContextMenu() {
            isShowContextMenuCalled = true;
            return super.showContextMenu();
        }
    }
}
