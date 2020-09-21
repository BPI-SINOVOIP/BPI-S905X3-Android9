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

package com.android.server.cts.device.statsd;

import android.view.View;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.FontMetrics;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.util.Log;


public class DaveyView extends View {

    private static final String TAG = "statsdDaveyView";

    private static final long DAVEY_TIME_MS = 750; // A bit more than 700ms to be safe.
    private boolean mCauseDavey;
    private Paint mPaint;
    private int mTexty;

    public DaveyView(Context context, AttributeSet attrs) {
        super(context, attrs);
        TypedArray a = context.getTheme().obtainStyledAttributes(
                attrs,
                R.styleable.DaveyView,
                0, 0);

        try {
            mCauseDavey = a.getBoolean(R.styleable.DaveyView_causeDavey, false);
        } finally {
            a.recycle();
        }

        mPaint = new Paint();
        mPaint.setColor(Color.BLACK);
        mPaint.setTextSize(20);
        FontMetrics metric = mPaint.getFontMetrics();
        int textHeight = (int) Math.ceil(metric.descent - metric.ascent);
        mTexty = textHeight - (int) metric.descent;
    }

    public void causeDavey(boolean cause) {
        mCauseDavey = cause;
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (mCauseDavey) {
            canvas.drawText("Davey!", 0, mTexty, mPaint);
            SystemClock.sleep(DAVEY_TIME_MS);
            mCauseDavey = false;
        } else {
            canvas.drawText("No Davey", 0, mTexty, mPaint);
        }
    }
}