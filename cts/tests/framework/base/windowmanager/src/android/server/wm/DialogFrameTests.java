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

package android.server.wm;

import static android.server.am.ComponentNameUtils.getWindowName;
import static android.server.wm.DialogFrameTestActivity.DIALOG_WINDOW_NAME;
import static android.server.wm.DialogFrameTestActivity.TEST_EXPLICIT_POSITION_MATCH_PARENT;
import static android.server.wm.DialogFrameTestActivity.TEST_EXPLICIT_POSITION_MATCH_PARENT_NO_LIMITS;
import static android.server.wm.DialogFrameTestActivity.TEST_EXPLICIT_SIZE;
import static android.server.wm.DialogFrameTestActivity.TEST_EXPLICIT_SIZE_BOTTOM_RIGHT_GRAVITY;
import static android.server.wm.DialogFrameTestActivity.TEST_EXPLICIT_SIZE_TOP_LEFT_GRAVITY;
import static android.server.wm.DialogFrameTestActivity.TEST_MATCH_PARENT;
import static android.server.wm.DialogFrameTestActivity.TEST_MATCH_PARENT_LAYOUT_IN_OVERSCAN;
import static android.server.wm.DialogFrameTestActivity.TEST_NO_FOCUS;
import static android.server.wm.DialogFrameTestActivity.TEST_OVER_SIZED_DIMENSIONS;
import static android.server.wm.DialogFrameTestActivity.TEST_OVER_SIZED_DIMENSIONS_NO_LIMITS;
import static android.server.wm.DialogFrameTestActivity.TEST_WITH_MARGINS;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.greaterThan;
import static org.junit.Assert.assertEquals;

import android.content.ComponentName;
import android.graphics.Rect;
import android.platform.test.annotations.AppModeFull;
import android.server.am.WaitForValidActivityState;
import android.server.am.WindowManagerState;
import android.server.am.WindowManagerState.WindowState;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;

import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;

import java.util.List;

/**
 * Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:DialogFrameTests
 *
 * TODO: Consolidate this class with {@link ParentChildTestBase}.
 */
@AppModeFull(reason = "Requires android.permission.MANAGE_ACTIVITY_STACKS")
public class DialogFrameTests extends ParentChildTestBase<DialogFrameTestActivity> {

    private static final ComponentName DIALOG_FRAME_TEST_ACTIVITY = new ComponentName(
            InstrumentationRegistry.getContext(), DialogFrameTestActivity.class);

    @Rule
    public final ActivityTestRule<DialogFrameTestActivity> mDialogTestActivity =
            new ActivityTestRule<>(DialogFrameTestActivity.class, false /* initialTOuchMode */,
                    false /* launchActivity */);

    @Override
    ComponentName activityName() {
        return DIALOG_FRAME_TEST_ACTIVITY;
    }

    @Override
    ActivityTestRule<DialogFrameTestActivity> activityRule() {
        return mDialogTestActivity;
    }

    private WindowState getSingleWindow(final String windowName) {
        final List<WindowState> windowList =
                mAmWmState.getWmState().getMatchingVisibleWindowState(windowName);
        assertThat(windowList.size(), greaterThan(0));
        return windowList.get(0);
    }

    @Override
    void doSingleTest(ParentChildTest t) throws Exception {
        mAmWmState.computeState(WaitForValidActivityState.forWindow(DIALOG_WINDOW_NAME));
        WindowState dialog = getSingleWindow(DIALOG_WINDOW_NAME);
        WindowState parent = getSingleWindow(getWindowName(activityName()));

        t.doTest(parent, dialog);
    }

    // With Width and Height as MATCH_PARENT we should fill
    // the same content frame as the main activity window
    @Test
    public void testMatchParentDialog() throws Exception {
        doParentChildTest(TEST_MATCH_PARENT, (parent, dialog) ->
                assertEquals(parent.getContentFrame(), dialog.getFrame())
        );
    }

    // If we have LAYOUT_IN_SCREEN and LAYOUT_IN_OVERSCAN with MATCH_PARENT,
    // we will not be constrained to the insets and so we will be the same size
    // as the main window main frame.
    @Test
    public void testMatchParentDialogLayoutInOverscan() throws Exception {
        doParentChildTest(TEST_MATCH_PARENT_LAYOUT_IN_OVERSCAN, (parent, dialog) ->
                assertEquals(parent.getFrame(), dialog.getFrame())
        );
    }

    private static final int explicitDimension = 200;

    // The default gravity for dialogs should center them.
    @Test
    public void testExplicitSizeDefaultGravity() throws Exception {
        doParentChildTest(TEST_EXPLICIT_SIZE, (parent, dialog) -> {
            Rect contentFrame = parent.getContentFrame();
            Rect expectedFrame = new Rect(
                    contentFrame.left + (contentFrame.width() - explicitDimension) / 2,
                    contentFrame.top + (contentFrame.height() - explicitDimension) / 2,
                    contentFrame.left + (contentFrame.width() + explicitDimension) / 2,
                    contentFrame.top + (contentFrame.height() + explicitDimension) / 2);
            assertEquals(expectedFrame, dialog.getFrame());
        });
    }

    @Test
    public void testExplicitSizeTopLeftGravity() throws Exception {
        doParentChildTest(TEST_EXPLICIT_SIZE_TOP_LEFT_GRAVITY, (parent, dialog) -> {
            Rect contentFrame = parent.getContentFrame();
            Rect expectedFrame = new Rect(
                    contentFrame.left,
                    contentFrame.top,
                    contentFrame.left + explicitDimension,
                    contentFrame.top + explicitDimension);
            assertEquals(expectedFrame, dialog.getFrame());
        });
    }

    @Test
    public void testExplicitSizeBottomRightGravity() throws Exception {
        doParentChildTest(TEST_EXPLICIT_SIZE_BOTTOM_RIGHT_GRAVITY, (parent, dialog) -> {
            Rect contentFrame = parent.getContentFrame();
            Rect expectedFrame = new Rect(
                    contentFrame.left + contentFrame.width() - explicitDimension,
                    contentFrame.top + contentFrame.height() - explicitDimension,
                    contentFrame.left + contentFrame.width(),
                    contentFrame.top + contentFrame.height());
            assertEquals(expectedFrame, dialog.getFrame());
        });
    }

    // TODO(b/30127373): Commented out for now because it doesn't work. We end up insetting the
    // decor on the bottom. I think this is a bug probably in the default dialog flags:
    @Ignore
    @Test
    public void testOversizedDimensions() throws Exception {
        doParentChildTest(TEST_OVER_SIZED_DIMENSIONS, (parent, dialog) ->
                // With the default flags oversize should result in clipping to
                // parent frame.
                assertEquals(parent.getContentFrame(), dialog.getFrame())
        );
    }

    // TODO(b/63993863) : Disabled pending public API to fetch maximum surface size.
    static final int oversizedDimension = 5000;
    // With FLAG_LAYOUT_NO_LIMITS  we should get the size we request, even if its much larger than
    // the screen.
    @Ignore
    @Test
    public void testOversizedDimensionsNoLimits() throws Exception {
        // TODO(b/36890978): We only run this in fullscreen because of the
        // unclear status of NO_LIMITS for non-child surfaces in MW modes
        doFullscreenTest(TEST_OVER_SIZED_DIMENSIONS_NO_LIMITS, (parent, dialog) -> {
            Rect contentFrame = parent.getContentFrame();
            Rect expectedFrame = new Rect(contentFrame.left, contentFrame.top,
                    contentFrame.left + oversizedDimension,
                    contentFrame.top + oversizedDimension);
            assertEquals(expectedFrame, dialog.getFrame());
        });
    }

    // If we request the MATCH_PARENT and a non-zero position, we wouldn't be
    // able to fit all of our content, so we should be adjusted to just fit the
    // content frame.
    @Test
    public void testExplicitPositionMatchParent() throws Exception {
        doParentChildTest(TEST_EXPLICIT_POSITION_MATCH_PARENT, (parent, dialog) ->
                assertEquals(parent.getContentFrame(), dialog.getFrame())
        );
    }

    // Unless we pass NO_LIMITS in which case our requested position should
    // be honored.
    @Test
    public void testExplicitPositionMatchParentNoLimits() throws Exception {
        final int explicitPosition = 100;
        doParentChildTest(TEST_EXPLICIT_POSITION_MATCH_PARENT_NO_LIMITS, (parent, dialog) -> {
            Rect contentFrame = parent.getContentFrame();
            Rect expectedFrame = new Rect(contentFrame);
            expectedFrame.offset(explicitPosition, explicitPosition);
            assertEquals(expectedFrame, dialog.getFrame());
        });
    }

    // We run the two focus tests fullscreen only because switching to the
    // docked stack will strip away focus from the task anyway.
    @Test
    public void testDialogReceivesFocus() throws Exception {
        doFullscreenTest(TEST_MATCH_PARENT, (parent, dialog) ->
                assertEquals(dialog.getName(), mAmWmState.getWmState().getFocusedWindow())
        );
    }

    @Test
    public void testNoFocusDialog() throws Exception {
        doFullscreenTest(TEST_NO_FOCUS, (parent, dialog) ->
                assertEquals(parent.getName(), mAmWmState.getWmState().getFocusedWindow())
        );
    }

    @Test
    public void testMarginsArePercentagesOfContentFrame() throws Exception {
        float horizontalMargin = .25f;
        float verticalMargin = .35f;
        doParentChildTest(TEST_WITH_MARGINS, (parent, dialog) -> {
            Rect frame = parent.getContentFrame();
            Rect expectedFrame = new Rect(
                    (int) (horizontalMargin * frame.width() + frame.left),
                    (int) (verticalMargin * frame.height() + frame.top),
                    (int) (horizontalMargin * frame.width() + frame.left) + explicitDimension,
                    (int) (verticalMargin * frame.height() + frame.top) + explicitDimension);
            assertEquals(expectedFrame, dialog.getFrame());
        });
    }

    @Test
    public void testDialogPlacedAboveParent() throws Exception {
        final WindowManagerState wmState = mAmWmState.getWmState();
        doParentChildTest(TEST_MATCH_PARENT, (parent, dialog) ->
                // Not only should the dialog be higher, but it should be leave multiple layers of
                // space in between for DimLayers, etc...
                assertThat(wmState.getZOrder(dialog), greaterThan(wmState.getZOrder(parent)))
        );
    }
}
