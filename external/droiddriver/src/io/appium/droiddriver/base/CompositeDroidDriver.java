/*
 * Copyright (C) 2016 DroidDriver committers
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

package io.appium.droiddriver.base;

import io.appium.droiddriver.UiDevice;
import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.actions.InputInjector;
import io.appium.droiddriver.finders.Finder;

/**
 * Helper class to ease creation of drivers that defer actions to other drivers.
 */
public abstract class CompositeDroidDriver extends AbstractDroidDriver {
  /**
   * Determines which DroidDriver should handle the current situation.
   *
   * @return The DroidDriver instance to use
   */
  protected abstract AbstractDroidDriver getApplicableDriver();

  @Override
  public InputInjector getInjector() {
    return getApplicableDriver().getInjector();
  }

  @Override
  public UiDevice getUiDevice() {
    return getApplicableDriver().getUiDevice();
  }

  @Override
  public UiElement find(Finder finder) {
    return getApplicableDriver().find(finder);
  }

  @Override
  public void refreshUiElementTree() {
    getApplicableDriver().refreshUiElementTree();
  }

  @Override
  public boolean dumpUiElementTree(String path) {
    return getApplicableDriver().dumpUiElementTree(path);
  }
}
