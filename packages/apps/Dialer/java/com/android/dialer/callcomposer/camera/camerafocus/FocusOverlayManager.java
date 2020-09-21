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

import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.RectF;
import android.hardware.Camera.Area;
import android.hardware.Camera.Parameters;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import com.android.dialer.common.Assert;
import com.android.dialer.common.LogUtil;
import java.util.ArrayList;
import java.util.List;

/**
 * A class that handles everything about focus in still picture mode. This also handles the metering
 * area because it is the same as focus area.
 *
 * <p>The test cases: (1) The camera has continuous autofocus. Move the camera. Take a picture when
 * CAF is not in progress. (2) The camera has continuous autofocus. Move the camera. Take a picture
 * when CAF is in progress. (3) The camera has face detection. Point the camera at some faces. Hold
 * the shutter. Release to take a picture. (4) The camera has face detection. Point the camera at
 * some faces. Single tap the shutter to take a picture. (5) The camera has autofocus. Single tap
 * the shutter to take a picture. (6) The camera has autofocus. Hold the shutter. Release to take a
 * picture. (7) The camera has no autofocus. Single tap the shutter and take a picture. (8) The
 * camera has autofocus and supports focus area. Touch the screen to trigger autofocus. Take a
 * picture. (9) The camera has autofocus and supports focus area. Touch the screen to trigger
 * autofocus. Wait until it times out. (10) The camera has no autofocus and supports metering area.
 * Touch the screen to change metering area.
 */
public class FocusOverlayManager {
  private static final String TRUE = "true";
  private static final String AUTO_EXPOSURE_LOCK_SUPPORTED = "auto-exposure-lock-supported";
  private static final String AUTO_WHITE_BALANCE_LOCK_SUPPORTED =
      "auto-whitebalance-lock-supported";

  private static final int RESET_TOUCH_FOCUS = 0;
  private static final int RESET_TOUCH_FOCUS_DELAY = 3000;

  private int state = STATE_IDLE;
  private static final int STATE_IDLE = 0; // Focus is not active.
  private static final int STATE_FOCUSING = 1; // Focus is in progress.
  // Focus is in progress and the camera should take a picture after focus finishes.
  private static final int STATE_FOCUSING_SNAP_ON_FINISH = 2;
  private static final int STATE_SUCCESS = 3; // Focus finishes and succeeds.
  private static final int STATE_FAIL = 4; // Focus finishes and fails.

  private boolean initialized;
  private boolean focusAreaSupported;
  private boolean meteringAreaSupported;
  private boolean lockAeAwbNeeded;
  private boolean aeAwbLock;
  private Matrix matrix;

  private PieRenderer pieRenderer;

  private int previewWidth; // The width of the preview frame layout.
  private int previewHeight; // The height of the preview frame layout.
  private boolean mirror; // true if the camera is front-facing.
  private List<Area> focusArea; // focus area in driver format
  private List<Area> meteringArea; // metering area in driver format
  private String focusMode;
  private Parameters parameters;
  private Handler handler;
  private Listener listener;

  /** Listener used for the focus indicator to communicate back to the camera. */
  public interface Listener {
    void autoFocus();

    void cancelAutoFocus();

    boolean capture();

    void setFocusParameters();
  }

  private class MainHandler extends Handler {
    public MainHandler(Looper looper) {
      super(looper);
    }

    @Override
    public void handleMessage(Message msg) {
      switch (msg.what) {
        case RESET_TOUCH_FOCUS:
          {
            cancelAutoFocus();
            break;
          }
      }
    }
  }

  public FocusOverlayManager(Listener listener, Looper looper) {
    handler = new MainHandler(looper);
    matrix = new Matrix();
    this.listener = listener;
  }

  public void setFocusRenderer(PieRenderer renderer) {
    pieRenderer = renderer;
    initialized = (matrix != null);
  }

  public void setParameters(Parameters parameters) {
    // parameters can only be null when onConfigurationChanged is called
    // before camera is open. We will just return in this case, because
    // parameters will be set again later with the right parameters after
    // camera is open.
    if (parameters == null) {
      return;
    }
    this.parameters = parameters;
    focusAreaSupported = isFocusAreaSupported(parameters);
    meteringAreaSupported = isMeteringAreaSupported(parameters);
    lockAeAwbNeeded =
        (isAutoExposureLockSupported(this.parameters)
            || isAutoWhiteBalanceLockSupported(this.parameters));
  }

  public void setPreviewSize(int previewWidth, int previewHeight) {
    if (this.previewWidth != previewWidth || this.previewHeight != previewHeight) {
      this.previewWidth = previewWidth;
      this.previewHeight = previewHeight;
      setMatrix();
    }
  }

  public void setMirror(boolean mirror) {
    this.mirror = mirror;
    setMatrix();
  }

  private void setMatrix() {
    if (previewWidth != 0 && previewHeight != 0) {
      Matrix matrix = new Matrix();
      prepareMatrix(matrix, mirror, previewWidth, previewHeight);
      // In face detection, the matrix converts the driver coordinates to UI
      // coordinates. In tap focus, the inverted matrix converts the UI
      // coordinates to driver coordinates.
      matrix.invert(this.matrix);
      initialized = (pieRenderer != null);
    }
  }

  private void lockAeAwbIfNeeded() {
    if (lockAeAwbNeeded && !aeAwbLock) {
      aeAwbLock = true;
      listener.setFocusParameters();
    }
  }

  public void onAutoFocus(boolean focused, boolean shutterButtonPressed) {
    if (state == STATE_FOCUSING_SNAP_ON_FINISH) {
      // Take the picture no matter focus succeeds or fails. No need
      // to play the AF sound if we're about to play the shutter
      // sound.
      if (focused) {
        state = STATE_SUCCESS;
      } else {
        state = STATE_FAIL;
      }
      updateFocusUI();
      capture();
    } else if (state == STATE_FOCUSING) {
      // This happens when (1) user is half-pressing the focus key or
      // (2) touch focus is triggered. Play the focus tone. Do not
      // take the picture now.
      if (focused) {
        state = STATE_SUCCESS;
      } else {
        state = STATE_FAIL;
      }
      updateFocusUI();
      // If this is triggered by touch focus, cancel focus after a
      // while.
      if (focusArea != null) {
        handler.sendEmptyMessageDelayed(RESET_TOUCH_FOCUS, RESET_TOUCH_FOCUS_DELAY);
      }
      if (shutterButtonPressed) {
        // Lock AE & AWB so users can half-press shutter and recompose.
        lockAeAwbIfNeeded();
      }
    } else if (state == STATE_IDLE) {
      // User has released the focus key before focus completes.
      // Do nothing.
    }
  }

  public void onAutoFocusMoving(boolean moving) {
    if (!initialized) {
      return;
    }

    // Ignore if we have requested autofocus. This method only handles
    // continuous autofocus.
    if (state != STATE_IDLE) {
      return;
    }

    if (moving) {
      pieRenderer.showStart();
    } else {
      pieRenderer.showSuccess(true);
    }
  }

  private void initializeFocusAreas(
      int focusWidth, int focusHeight, int x, int y, int previewWidth, int previewHeight) {
    if (focusArea == null) {
      focusArea = new ArrayList<>();
      focusArea.add(new Area(new Rect(), 1));
    }

    // Convert the coordinates to driver format.
    calculateTapArea(
        focusWidth, focusHeight, 1f, x, y, previewWidth, previewHeight, focusArea.get(0).rect);
  }

  private void initializeMeteringAreas(
      int focusWidth, int focusHeight, int x, int y, int previewWidth, int previewHeight) {
    if (meteringArea == null) {
      meteringArea = new ArrayList<>();
      meteringArea.add(new Area(new Rect(), 1));
    }

    // Convert the coordinates to driver format.
    // AE area is bigger because exposure is sensitive and
    // easy to over- or underexposure if area is too small.
    calculateTapArea(
        focusWidth, focusHeight, 1.5f, x, y, previewWidth, previewHeight, meteringArea.get(0).rect);
  }

  public void onSingleTapUp(int x, int y) {
    if (!initialized || state == STATE_FOCUSING_SNAP_ON_FINISH) {
      return;
    }

    // Let users be able to cancel previous touch focus.
    if ((focusArea != null)
        && (state == STATE_FOCUSING || state == STATE_SUCCESS || state == STATE_FAIL)) {
      cancelAutoFocus();
    }
    // Initialize variables.
    int focusWidth = pieRenderer.getSize();
    int focusHeight = pieRenderer.getSize();
    if (focusWidth == 0 || pieRenderer.getWidth() == 0 || pieRenderer.getHeight() == 0) {
      return;
    }
    int previewWidth = this.previewWidth;
    int previewHeight = this.previewHeight;
    // Initialize mFocusArea.
    if (focusAreaSupported) {
      initializeFocusAreas(focusWidth, focusHeight, x, y, previewWidth, previewHeight);
    }
    // Initialize mMeteringArea.
    if (meteringAreaSupported) {
      initializeMeteringAreas(focusWidth, focusHeight, x, y, previewWidth, previewHeight);
    }

    // Use margin to set the focus indicator to the touched area.
    pieRenderer.setFocus(x, y);

    // Set the focus area and metering area.
    listener.setFocusParameters();
    if (focusAreaSupported) {
      autoFocus();
    } else { // Just show the indicator in all other cases.
      updateFocusUI();
      // Reset the metering area in 3 seconds.
      handler.removeMessages(RESET_TOUCH_FOCUS);
      handler.sendEmptyMessageDelayed(RESET_TOUCH_FOCUS, RESET_TOUCH_FOCUS_DELAY);
    }
  }

  public void onPreviewStarted() {
    state = STATE_IDLE;
  }

  public void onPreviewStopped() {
    // If auto focus was in progress, it would have been stopped.
    state = STATE_IDLE;
    resetTouchFocus();
    updateFocusUI();
  }

  public void onCameraReleased() {
    onPreviewStopped();
  }

  private void autoFocus() {
    LogUtil.v("FocusOverlayManager.autoFocus", "Start autofocus.");
    listener.autoFocus();
    state = STATE_FOCUSING;
    updateFocusUI();
    handler.removeMessages(RESET_TOUCH_FOCUS);
  }

  public void cancelAutoFocus() {
    LogUtil.v("FocusOverlayManager.cancelAutoFocus", "Cancel autofocus.");

    // Reset the tap area before calling mListener.cancelAutofocus.
    // Otherwise, focus mode stays at auto and the tap area passed to the
    // driver is not reset.
    resetTouchFocus();
    listener.cancelAutoFocus();
    state = STATE_IDLE;
    updateFocusUI();
    handler.removeMessages(RESET_TOUCH_FOCUS);
  }

  private void capture() {
    if (listener.capture()) {
      state = STATE_IDLE;
      handler.removeMessages(RESET_TOUCH_FOCUS);
    }
  }

  public String getFocusMode() {
    List<String> supportedFocusModes = parameters.getSupportedFocusModes();

    if (focusAreaSupported && focusArea != null) {
      // Always use autofocus in tap-to-focus.
      focusMode = Parameters.FOCUS_MODE_AUTO;
    } else {
      focusMode = Parameters.FOCUS_MODE_CONTINUOUS_PICTURE;
    }

    if (!isSupported(focusMode, supportedFocusModes)) {
      // For some reasons, the driver does not support the current
      // focus mode. Fall back to auto.
      if (isSupported(Parameters.FOCUS_MODE_AUTO, parameters.getSupportedFocusModes())) {
        focusMode = Parameters.FOCUS_MODE_AUTO;
      } else {
        focusMode = parameters.getFocusMode();
      }
    }
    return focusMode;
  }

  public List<Area> getFocusAreas() {
    return focusArea;
  }

  public List<Area> getMeteringAreas() {
    return meteringArea;
  }

  private void updateFocusUI() {
    if (!initialized) {
      return;
    }
    FocusIndicator focusIndicator = pieRenderer;

    if (state == STATE_IDLE) {
      if (focusArea == null) {
        focusIndicator.clear();
      } else {
        // Users touch on the preview and the indicator represents the
        // metering area. Either focus area is not supported or
        // autoFocus call is not required.
        focusIndicator.showStart();
      }
    } else if (state == STATE_FOCUSING || state == STATE_FOCUSING_SNAP_ON_FINISH) {
      focusIndicator.showStart();
    } else {
      if (Parameters.FOCUS_MODE_CONTINUOUS_PICTURE.equals(focusMode)) {
        // TODO(blemmon): check HAL behavior and decide if this can be removed.
        focusIndicator.showSuccess(false);
      } else if (state == STATE_SUCCESS) {
        focusIndicator.showSuccess(false);
      } else if (state == STATE_FAIL) {
        focusIndicator.showFail(false);
      }
    }
  }

  private void resetTouchFocus() {
    if (!initialized) {
      return;
    }

    // Put focus indicator to the center. clear reset position
    pieRenderer.clear();

    focusArea = null;
    meteringArea = null;
  }

  private void calculateTapArea(
      int focusWidth,
      int focusHeight,
      float areaMultiple,
      int x,
      int y,
      int previewWidth,
      int previewHeight,
      Rect rect) {
    int areaWidth = (int) (focusWidth * areaMultiple);
    int areaHeight = (int) (focusHeight * areaMultiple);
    final int maxW = previewWidth - areaWidth;
    int left = maxW > 0 ? clamp(x - areaWidth / 2, 0, maxW) : 0;
    final int maxH = previewHeight - areaHeight;
    int top = maxH > 0 ? clamp(y - areaHeight / 2, 0, maxH) : 0;

    RectF rectF = new RectF(left, top, left + areaWidth, top + areaHeight);
    matrix.mapRect(rectF);
    rectFToRect(rectF, rect);
  }

  private int clamp(int x, int min, int max) {
    Assert.checkArgument(max >= min);
    if (x > max) {
      return max;
    }
    if (x < min) {
      return min;
    }
    return x;
  }

  private boolean isAutoExposureLockSupported(Parameters params) {
    return TRUE.equals(params.get(AUTO_EXPOSURE_LOCK_SUPPORTED));
  }

  private boolean isAutoWhiteBalanceLockSupported(Parameters params) {
    return TRUE.equals(params.get(AUTO_WHITE_BALANCE_LOCK_SUPPORTED));
  }

  private boolean isSupported(String value, List<String> supported) {
    return supported != null && supported.indexOf(value) >= 0;
  }

  private boolean isMeteringAreaSupported(Parameters params) {
    return params.getMaxNumMeteringAreas() > 0;
  }

  private boolean isFocusAreaSupported(Parameters params) {
    return (params.getMaxNumFocusAreas() > 0
        && isSupported(Parameters.FOCUS_MODE_AUTO, params.getSupportedFocusModes()));
  }

  private void prepareMatrix(Matrix matrix, boolean mirror, int viewWidth, int viewHeight) {
    // Need mirror for front camera.
    matrix.setScale(mirror ? -1 : 1, 1);
    // Camera driver coordinates range from (-1000, -1000) to (1000, 1000).
    // UI coordinates range from (0, 0) to (width, height).
    matrix.postScale(viewWidth / 2000f, viewHeight / 2000f);
    matrix.postTranslate(viewWidth / 2f, viewHeight / 2f);
  }

  private void rectFToRect(RectF rectF, Rect rect) {
    rect.left = Math.round(rectF.left);
    rect.top = Math.round(rectF.top);
    rect.right = Math.round(rectF.right);
    rect.bottom = Math.round(rectF.bottom);
  }
}
