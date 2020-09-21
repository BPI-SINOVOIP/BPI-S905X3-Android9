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

package com.android.gallery3d.ingest.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.GridView;

/**
 * Extends GridView with the ability to listen for calls to clearChoices()
 */
public class IngestGridView extends GridView {

  /**
   * Listener for all checked choices being cleared.
   */
  public interface OnClearChoicesListener {
    public void onClearChoices();
  }

  private OnClearChoicesListener mOnClearChoicesListener = null;

  public IngestGridView(Context context) {
    super(context);
  }

  public IngestGridView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  public IngestGridView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
  }

  public void setOnClearChoicesListener(OnClearChoicesListener l) {
    mOnClearChoicesListener = l;
  }

  @Override
  public void clearChoices() {
    super.clearChoices();
    if (mOnClearChoicesListener != null) {
      mOnClearChoicesListener.onClearChoices();
    }
  }
}
