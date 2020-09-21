/*
 * Copyright (C) 2013 DroidDriver committers
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

package io.appium.droiddriver;

import android.graphics.Bitmap.CompressFormat;

import io.appium.droiddriver.actions.Action;

/**
 * Interface for device-wide interaction.
 */
public interface UiDevice {
  /**
   * Returns whether the screen is on.
   */
  boolean isScreenOn();

  /** Wakes up device if the screen is off */
  void wakeUp();

  /** Puts device to sleep if the screen is on */
  void sleep();

  /** Simulates pressing "back" button */
  void pressBack();

  /**
   * Executes a global action without the context of a certain UiElement.
   *
   * @param action The action to execute
   * @return true if the action is successful
   */
  boolean perform(Action action);

  /**
   * Takes a screenshot of current window and stores it in {@code path} as PNG.
   * <p>
   * If this is used in a test which extends
   * {@link android.test.ActivityInstrumentationTestCase2}, call this before
   * {@code tearDown()} because {@code tearDown()} finishes activities created
   * by {@link android.test.ActivityInstrumentationTestCase2#getActivity()}.
   *
   * @param path the path of file to save screenshot
   * @return true if screen shot is created successfully
   */
  boolean takeScreenshot(String path);

  /**
   * Takes a screenshot of current window and stores it in {@code path}. Note
   * some implementations may not capture everything on the screen, for example
   * InstrumentationDriver may not see the IME soft keyboard or system content.
   *
   * @param path the path of file to save screenshot
   * @param format The format of the compressed image
   * @param quality Hint to the compressor, 0-100. 0 meaning compress for small
   *        size, 100 meaning compress for max quality. Some formats, like PNG
   *        which is lossless, will ignore the quality setting
   * @return true if screen shot is created successfully
   */
  boolean takeScreenshot(String path, CompressFormat format, int quality);
}
