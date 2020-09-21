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

package android.view.cts;

import android.content.Context;
import androidx.annotation.Nullable;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.widget.LinearLayout;

public class PointerCaptureGroup extends LinearLayout {
    private boolean mCalledDispatchPointerCaptureChanged;
    private boolean mCalledOnCapturedPointerEvent;
    private boolean mCalledDispatchCapturedPointerEvent;
    private boolean mCalledOnPointerCaptureChange;

    public PointerCaptureGroup(Context context) {
        super(context);
    }

    public PointerCaptureGroup(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public PointerCaptureGroup(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public PointerCaptureGroup(Context context, @Nullable AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    @Override
    public void dispatchPointerCaptureChanged(boolean hasCapture) {
        mCalledDispatchPointerCaptureChanged = true;
        super.dispatchPointerCaptureChanged(hasCapture);
    }

    @Override
    public void onPointerCaptureChange(boolean hasCapture) {
        mCalledOnPointerCaptureChange = true;
        super.onPointerCaptureChange(hasCapture);
    }

    @Override
    public boolean dispatchCapturedPointerEvent(MotionEvent event) {
        mCalledDispatchCapturedPointerEvent = true;
        return super.dispatchCapturedPointerEvent(event);
    }

    @Override
    public boolean onCapturedPointerEvent(MotionEvent event) {
        mCalledOnCapturedPointerEvent = true;
        return super.onCapturedPointerEvent(event);
    }

    void reset() {
        mCalledDispatchPointerCaptureChanged = false;
        mCalledDispatchCapturedPointerEvent = false;
        mCalledOnPointerCaptureChange = false;
        mCalledOnCapturedPointerEvent = false;
    }

    boolean hasCalledDispatchPointerCaptureChanged() {
        return mCalledDispatchPointerCaptureChanged;
    }

    boolean hasCalledOnCapturedPointerEvent() {
        return mCalledOnCapturedPointerEvent;
    }

    boolean hasCalledDispatchCapturedPointerEvent() {
        return mCalledDispatchCapturedPointerEvent;
    }

    boolean hasCalledOnPointerCaptureChange() {
        return mCalledOnPointerCaptureChange;
    }
}
