/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.gallery3d.filtershow.info;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.os.AsyncTask;
import android.util.AttributeSet;
import android.view.View;

public class HistogramView extends View {

    private Bitmap mBitmap;
    private Paint mPaint = new Paint();
    private int[] redHistogram = new int[256];
    private int[] greenHistogram = new int[256];
    private int[] blueHistogram = new int[256];
    private Path mHistoPath = new Path();

    class ComputeHistogramTask extends AsyncTask<Bitmap, Void, int[]> {
        @Override
        protected int[] doInBackground(Bitmap... params) {
            int[] histo = new int[256 * 3];
            Bitmap bitmap = params[0];
            int w = bitmap.getWidth();
            int h = bitmap.getHeight();
            int[] pixels = new int[w * h];
            bitmap.getPixels(pixels, 0, w, 0, 0, w, h);
            for (int i = 0; i < w; i++) {
                for (int j = 0; j < h; j++) {
                    int index = j * w + i;
                    int r = Color.red(pixels[index]);
                    int g = Color.green(pixels[index]);
                    int b = Color.blue(pixels[index]);
                    histo[r]++;
                    histo[256 + g]++;
                    histo[512 + b]++;
                }
            }
            return histo;
        }

        @Override
        protected void onPostExecute(int[] result) {
            System.arraycopy(result, 0, redHistogram, 0, 256);
            System.arraycopy(result, 256, greenHistogram, 0, 256);
            System.arraycopy(result, 512, blueHistogram, 0, 256);
            invalidate();
        }
    }

    public HistogramView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void setBitmap(Bitmap bitmap) {
        mBitmap = bitmap.copy(Bitmap.Config.ARGB_8888, true);
        new ComputeHistogramTask().execute(mBitmap);
    }

    private void drawHistogram(Canvas canvas, int[] histogram, int color, PorterDuff.Mode mode) {
        int max = 0;
        for (int i = 0; i < histogram.length; i++) {
            if (histogram[i] > max) {
                max = histogram[i];
            }
        }
        float w = getWidth(); // - Spline.curveHandleSize();
        float h = getHeight(); // - Spline.curveHandleSize() / 2.0f;
        float dx = 0; // Spline.curveHandleSize() / 2.0f;
        float wl = w / histogram.length;
        float wh = h / max;

        mPaint.reset();
        mPaint.setAntiAlias(true);
        mPaint.setARGB(100, 255, 255, 255);
        mPaint.setStrokeWidth((int) Math.ceil(wl));

        // Draw grid
        mPaint.setStyle(Paint.Style.STROKE);
        canvas.drawRect(dx, 0, dx + w, h, mPaint);
        canvas.drawLine(dx + w / 3, 0, dx + w / 3, h, mPaint);
        canvas.drawLine(dx + 2 * w / 3, 0, dx + 2 * w / 3, h, mPaint);

        mPaint.setStyle(Paint.Style.FILL);
        mPaint.setColor(color);
        mPaint.setStrokeWidth(6);
        mPaint.setXfermode(new PorterDuffXfermode(mode));
        mHistoPath.reset();
        mHistoPath.moveTo(dx, h);
        boolean firstPointEncountered = false;
        float prev = 0;
        float last = 0;
        for (int i = 0; i < histogram.length; i++) {
            float x = i * wl + dx;
            float l = histogram[i] * wh;
            if (l != 0) {
                float v = h - (l + prev) / 2.0f;
                if (!firstPointEncountered) {
                    mHistoPath.lineTo(x, h);
                    firstPointEncountered = true;
                }
                mHistoPath.lineTo(x, v);
                prev = l;
                last = x;
            }
        }
        mHistoPath.lineTo(last, h);
        mHistoPath.lineTo(w, h);
        mHistoPath.close();
        canvas.drawPath(mHistoPath, mPaint);
        mPaint.setStrokeWidth(2);
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setARGB(255, 200, 200, 200);
        canvas.drawPath(mHistoPath, mPaint);
    }

    public void onDraw(Canvas canvas) {
        canvas.drawARGB(0, 0, 0, 0);
        drawHistogram(canvas, redHistogram, Color.RED, PorterDuff.Mode.SCREEN);
        drawHistogram(canvas, greenHistogram, Color.GREEN, PorterDuff.Mode.SCREEN);
        drawHistogram(canvas, blueHistogram, Color.BLUE, PorterDuff.Mode.SCREEN);
    }
}
