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
package com.android.framesequence.samples;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.support.rastermill.FrameSequence;
import android.support.rastermill.FrameSequenceDrawable;
import android.view.View;
import android.widget.Toast;

import java.io.InputStream;
import java.util.HashSet;

public class FrameSequenceTest extends Activity {
    FrameSequenceDrawable mDrawable;
    int mResourceId;

    // This provider is entirely unnecessary, just here to validate the acquire/release process
    private class CheckingProvider implements FrameSequenceDrawable.BitmapProvider {
        HashSet<Bitmap> mBitmaps = new HashSet<Bitmap>();
        @Override
        public Bitmap acquireBitmap(int minWidth, int minHeight) {
            Bitmap bitmap =
                    Bitmap.createBitmap(minWidth + 1, minHeight + 4, Bitmap.Config.ARGB_8888);
            mBitmaps.add(bitmap);
            return bitmap;
        }

        @Override
        public void releaseBitmap(Bitmap bitmap) {
            if (!mBitmaps.contains(bitmap)) throw new IllegalStateException();
            mBitmaps.remove(bitmap);
            bitmap.recycle();
        }

        public boolean isEmpty() {
            return mBitmaps.isEmpty();
        }
    }

    final CheckingProvider mProvider = new CheckingProvider();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mResourceId = getIntent().getIntExtra("resourceId", R.raw.animated_gif);

        setContentView(R.layout.basic_test_activity);
        findViewById(R.id.start).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mDrawable.start();
            }
        });
        findViewById(R.id.stop).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mDrawable.stop();
            }
        });
        findViewById(R.id.vis).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mDrawable.setVisible(true, true);
            }
        });
        findViewById(R.id.invis).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mDrawable.setVisible(false, true);
            }
        });
        findViewById(R.id.circle_mask).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mDrawable.setCircleMaskEnabled(!mDrawable.getCircleMaskEnabled());
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();

        View drawableView = findViewById(R.id.drawableview);
        InputStream is = getResources().openRawResource(mResourceId);

        FrameSequence fs = FrameSequence.decodeStream(is);
        mDrawable = new FrameSequenceDrawable(fs, mProvider);
        mDrawable.setOnFinishedListener(new FrameSequenceDrawable.OnFinishedListener() {
            @Override
            public void onFinished(FrameSequenceDrawable drawable) {
                Toast.makeText(getApplicationContext(),
                        "The animation has finished", Toast.LENGTH_SHORT).show();
            }
        });
        drawableView.setBackgroundDrawable(mDrawable);
    }

    @Override
    protected void onPause() {
        super.onPause();
        View drawableView = findViewById(R.id.drawableview);

        mDrawable.destroy();
        if (!mProvider.isEmpty()) throw new IllegalStateException("All bitmaps not recycled");

        mDrawable = null;
        drawableView.setBackgroundDrawable(null);
    }
}
