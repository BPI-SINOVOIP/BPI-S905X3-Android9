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

package android.accessibilityservice.cts.utils;

import android.accessibilityservice.AccessibilityService.GestureResultCallback;
import android.accessibilityservice.GestureDescription;
import android.accessibilityservice.GestureDescription.StrokeDescription;
import android.accessibilityservice.cts.InstrumentedAccessibilityService;
import android.graphics.Path;
import android.graphics.PointF;
import android.view.ViewConfiguration;

import java.util.concurrent.CompletableFuture;

public class GestureUtils {

    private GestureUtils() {}

    public static CompletableFuture<Void> dispatchGesture(
            InstrumentedAccessibilityService service,
            GestureDescription gesture) {
        CompletableFuture<Void> result = new CompletableFuture<>();
        GestureResultCallback callback = new GestureResultCallback() {
            @Override
            public void onCompleted(GestureDescription gestureDescription) {
                result.complete(null);
            }

            @Override
            public void onCancelled(GestureDescription gestureDescription) {
                result.cancel(false);
            }
        };
        service.runOnServiceSync(() -> {
            if (!service.dispatchGesture(gesture, callback, null)) {
                result.completeExceptionally(new IllegalStateException());
            }
        });
        return result;
    }

    public static StrokeDescription pointerDown(PointF point) {
        return new StrokeDescription(path(point), 0, ViewConfiguration.getTapTimeout(), true);
    }

    public static StrokeDescription pointerUp(StrokeDescription lastStroke) {
        return lastStroke.continueStroke(path(lastPointOf(lastStroke)),
                endTimeOf(lastStroke), ViewConfiguration.getTapTimeout(), false);
    }

    public static PointF lastPointOf(StrokeDescription stroke) {
        float[] p = stroke.getPath().approximate(0.3f);
        return new PointF(p[p.length - 2], p[p.length - 1]);
    }

    public static StrokeDescription click(PointF point) {
        return new StrokeDescription(path(point), 0, ViewConfiguration.getTapTimeout());
    }

    public static StrokeDescription longClick(PointF point) {
        return new StrokeDescription(path(point), 0,
                ViewConfiguration.getLongPressTimeout() * 3 / 2);
    }

    public static StrokeDescription swipe(PointF from, PointF to) {
        return swipe(from, to, ViewConfiguration.getTapTimeout());
    }

    public static StrokeDescription swipe(PointF from, PointF to, long duration) {
        return new StrokeDescription(path(from, to), 0, duration);
    }

    public static StrokeDescription drag(StrokeDescription from, PointF to) {
        return from.continueStroke(
                path(lastPointOf(from), to),
                endTimeOf(from), ViewConfiguration.getTapTimeout(), true);
    }

    public static Path path(PointF first, PointF... rest) {
        Path path = new Path();
        path.moveTo(first.x, first.y);
        for (PointF point : rest) {
            path.lineTo(point.x, point.y);
        }
        return path;
    }

    public static StrokeDescription startingAt(long timeMs, StrokeDescription prototype) {
        return new StrokeDescription(
                prototype.getPath(), timeMs, prototype.getDuration(), prototype.willContinue());
    }

    public static long endTimeOf(StrokeDescription stroke) {
        return stroke.getStartTime() + stroke.getDuration();
    }

    public static float distance(PointF a, PointF b) {
        if (a == null) throw new NullPointerException();
        if (b == null) throw new NullPointerException();
        return (float) Math.hypot(a.x - b.x, a.y - b.y);
    }

    public static PointF add(PointF a, float x, float y) {
        return new PointF(a.x + x, a.y + y);
    }

    public static PointF add(PointF a, PointF b) {
        return add(a, b.x, b.y);
    }

    public static PointF diff(PointF a, PointF b) {
        return add(a, -b.x, -b.y);
    }

    public static PointF negate(PointF p) {
        return times(-1, p);
    }

    public static PointF times(float mult, PointF p) {
        return new PointF(p.x * mult, p.y * mult);
    }

    public static float length(PointF p) {
        return (float) Math.hypot(p.x, p.y);
    }

    public static PointF ceil(PointF p) {
        return new PointF((float) Math.ceil(p.x), (float) Math.ceil(p.y));
    }
}
