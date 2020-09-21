/*
 * Copyright (C) 2012 The Android Open Source Project
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
package com.android.gallery3d.ui;

import android.graphics.RectF;

import com.android.gallery3d.glrenderer.GLCanvas;

public interface ScreenNail {
    public int getWidth();
    public int getHeight();
    public void draw(GLCanvas canvas, int x, int y, int width, int height);

    // We do not need to draw this ScreenNail in this frame.
    public void noDraw();

    // This ScreenNail will not be used anymore. Release related resources.
    public void recycle();

    // This is only used by TileImageView to back up the tiles not yet loaded.
    public void draw(GLCanvas canvas, RectF source, RectF dest);
}
