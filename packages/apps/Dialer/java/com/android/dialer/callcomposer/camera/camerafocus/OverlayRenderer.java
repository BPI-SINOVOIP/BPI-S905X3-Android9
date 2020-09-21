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

package com.android.dialer.callcomposer.camera.camerafocus;

import android.content.Context;
import android.graphics.Canvas;
import android.view.MotionEvent;

/** Abstract class that all Camera overlays should implement. */
public abstract class OverlayRenderer implements RenderOverlay.Renderer {

  protected RenderOverlay overlay;

  private int left;
  private int top;
  private int right;
  private int bottom;
  private boolean visible;

  public void setVisible(boolean vis) {
    visible = vis;
    update();
  }

  public boolean isVisible() {
    return visible;
  }

  // default does not handle touch
  @Override
  public boolean handlesTouch() {
    return false;
  }

  @Override
  public boolean onTouchEvent(MotionEvent evt) {
    return false;
  }

  public abstract void onDraw(Canvas canvas);

  @Override
  public void draw(Canvas canvas) {
    if (visible) {
      onDraw(canvas);
    }
  }

  @Override
  public void setOverlay(RenderOverlay overlay) {
    this.overlay = overlay;
  }

  @Override
  public void layout(int left, int top, int right, int bottom) {
    this.left = left;
    this.right = right;
    this.top = top;
    this.bottom = bottom;
  }

  protected Context getContext() {
    if (overlay != null) {
      return overlay.getContext();
    } else {
      return null;
    }
  }

  public int getWidth() {
    return right - left;
  }

  public int getHeight() {
    return bottom - top;
  }

  protected void update() {
    if (overlay != null) {
      overlay.update();
    }
  }
}
