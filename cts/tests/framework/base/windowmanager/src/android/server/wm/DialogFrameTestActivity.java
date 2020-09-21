/*
 * Copyright (C) 2018 The Android Open Source Project
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

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_IN_OVERSCAN;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
import static android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.view.Gravity;
import android.view.Window;
import android.view.WindowManager.LayoutParams;

import java.util.function.Consumer;

// TODO: Make this an inner class of {@link DialogFrameTest}.
public class DialogFrameTestActivity extends Activity {

    static final String DIALOG_WINDOW_NAME = "TestDialog";

    // TODO: Passing layout parameters for {@link Dialog} in extra of {@link Intent} instead of
    // using test case name.
    // Extra key for test case name.
    static final String EXTRA_TEST_CASE = "test-case";
    // Value constants for {@link #EXTRA_TEST_CASE}.
    static final String TEST_EXPLICIT_POSITION_MATCH_PARENT = "ExplicitPositionMatchParent";
    static final String TEST_EXPLICIT_POSITION_MATCH_PARENT_NO_LIMITS =
            "ExplicitPositionMatchParentNoLimits";
    static final String TEST_EXPLICIT_SIZE = "ExplicitSize";
    static final String TEST_EXPLICIT_SIZE_BOTTOM_RIGHT_GRAVITY =
            "ExplicitSizeBottomRightGravity";
    static final String TEST_EXPLICIT_SIZE_TOP_LEFT_GRAVITY = "ExplicitSizeTopLeftGravity";
    static final String TEST_MATCH_PARENT = "MatchParent";
    static final String TEST_MATCH_PARENT_LAYOUT_IN_OVERSCAN = "MatchParentLayoutInOverscan";
    static final String TEST_NO_FOCUS = "NoFocus";
    static final String TEST_OVER_SIZED_DIMENSIONS = "OversizedDimensions";
    static final String TEST_OVER_SIZED_DIMENSIONS_NO_LIMITS = "OversizedDimensionsNoLimits";
    static final String TEST_WITH_MARGINS = "WithMargins";

    private AlertDialog mDialog;

    @Override
    protected void onStop() {
        super.onStop();
        mDialog.dismiss();
    }

    @Override
    protected void onResume() {
        super.onResume();
        setupTest(getIntent());
    }

    private void setupTest(Intent intent) {
        final String testCase = intent.getStringExtra(EXTRA_TEST_CASE);
        switch (testCase) {
            case TEST_MATCH_PARENT:
                testMatchParent();
                break;
            case TEST_MATCH_PARENT_LAYOUT_IN_OVERSCAN:
                testMatchParentLayoutInOverscan();
                break;
            case TEST_EXPLICIT_SIZE:
                testExplicitSize();
                break;
            case TEST_EXPLICIT_SIZE_TOP_LEFT_GRAVITY:
                testExplicitSizeTopLeftGravity();
                break;
            case TEST_EXPLICIT_SIZE_BOTTOM_RIGHT_GRAVITY:
                testExplicitSizeBottomRightGravity();
                break;
            case TEST_OVER_SIZED_DIMENSIONS:
                testOversizedDimensions();
                break;
            case TEST_OVER_SIZED_DIMENSIONS_NO_LIMITS:
                testOversizedDimensionsNoLimits();
                break;
            case TEST_EXPLICIT_POSITION_MATCH_PARENT:
                testExplicitPositionMatchParent();
                break;
            case TEST_EXPLICIT_POSITION_MATCH_PARENT_NO_LIMITS:
                testExplicitPositionMatchParentNoLimits();
                break;
            case TEST_NO_FOCUS:
                testNoFocus();
                break;
            case TEST_WITH_MARGINS:
                testWithMargins();
                break;
            default:
                break;
        }
    }

    private void doLayoutParamTest(Consumer<LayoutParams> setUp) {
        mDialog = new AlertDialog.Builder(this).create();

        mDialog.setMessage("Testing is fun!");
        mDialog.setTitle(DIALOG_WINDOW_NAME);
        mDialog.create();

        Window w = mDialog.getWindow();
        final LayoutParams params = w.getAttributes();
        setUp.accept(params);
        w.setAttributes(params);

        mDialog.show();
    }

    private void testMatchParent() {
        doLayoutParamTest(params -> {
            params.width = MATCH_PARENT;
            params.height = MATCH_PARENT;
        });
    }

    private void testMatchParentLayoutInOverscan() {
        doLayoutParamTest(params -> {
            params.width = MATCH_PARENT;
            params.height = MATCH_PARENT;
            params.flags |= FLAG_LAYOUT_IN_SCREEN;
            params.flags |= FLAG_LAYOUT_IN_OVERSCAN;
        });
    }

    private void testExplicitSize() {
        doLayoutParamTest(params -> {
            params.width = 200;
            params.height = 200;
        });
    }

    private void testExplicitSizeTopLeftGravity() {
        doLayoutParamTest(params -> {
            params.width = 200;
            params.height = 200;
            params.gravity = Gravity.TOP | Gravity.LEFT;
        });
    }

    private void testExplicitSizeBottomRightGravity() {
        doLayoutParamTest(params -> {
            params.width = 200;
            params.height = 200;
            params.gravity = Gravity.BOTTOM | Gravity.RIGHT;
        });
    }

    private void testOversizedDimensions() {
        doLayoutParamTest(params -> {
            params.width = 100000;
            params.height = 100000;
        });
    }

    private void testOversizedDimensionsNoLimits() {
        doLayoutParamTest(params -> {
            params.width = 5000;
            params.height = 5000;
            params.flags |= FLAG_LAYOUT_NO_LIMITS;
            params.gravity = Gravity.LEFT | Gravity.TOP;
        });
    }

    private void testExplicitPositionMatchParent() {
        doLayoutParamTest(params -> {
            params.width = MATCH_PARENT;
            params.height = MATCH_PARENT;
            params.x = 100;
            params.y = 100;
        });
    }

    private void testExplicitPositionMatchParentNoLimits() {
        doLayoutParamTest(params -> {
            params.width = MATCH_PARENT;
            params.height = MATCH_PARENT;
            params.gravity = Gravity.LEFT | Gravity.TOP;
            params.flags |= FLAG_LAYOUT_NO_LIMITS;
            params.x = 100;
            params.y = 100;
        });
    }

    private void testNoFocus() {
        doLayoutParamTest(params -> params.flags |= FLAG_NOT_FOCUSABLE);
    }

    private void testWithMargins() {
        doLayoutParamTest(params -> {
            params.gravity = Gravity.LEFT | Gravity.TOP;
            params.horizontalMargin = .25f;
            params.verticalMargin = .35f;
            params.width = 200;
            params.height = 200;
            params.x = 0;
            params.y = 0;
        });
    }
}
