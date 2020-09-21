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

package com.android.amm.test;

import android.app.Activity;
import android.os.Bundle;
import android.widget.LinearLayout;

public class MainActivity extends Activity {

  private BitmapUse mBitmapUse;
  private SoCodeUse mSoCodeUse;
  private TextureViewUse mTextureViewUse;
  private SurfaceViewUse mSurfaceViewUse;
  private ThreadedRendererUse mThreadedRendererUse;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    mBitmapUse = new BitmapUse();
    mSoCodeUse = new SoCodeUse();

    LinearLayout ll = new LinearLayout(this);
    mTextureViewUse = new TextureViewUse(this, ll, 200, 500);
    mSurfaceViewUse = new SurfaceViewUse(this, ll, 240, 250);
    setContentView(ll);

    mThreadedRendererUse = new ThreadedRendererUse(this, 122, 152);
  }
}

