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

package io.appium.droiddriver.base;

import android.util.Log;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.finders.ByXPath;
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.util.Logs;

/**
 * Enhances AbstractDroidDriver to include basic element handling and matching operations.
 */
public abstract class BaseDroidDriver<R, E extends BaseUiElement<R, E>> extends AbstractDroidDriver {

  private E rootElement;

  @Override
  public UiElement find(Finder finder) {
    Logs.call(Log.VERBOSE, this, "find", finder);
    return finder.find(getRootElement());
  }

  protected abstract E newRootElement();

  /**
   * Returns a new UiElement of type {@code E}.
   */
  protected abstract E newUiElement(R rawElement, E parent);

  public E getRootElement() {
    if (rootElement == null) {
      refreshUiElementTree();
    }
    return rootElement;
  }

  @Override
  public void refreshUiElementTree() {
    rootElement = newRootElement();
  }

  @Override
  public boolean dumpUiElementTree(String path) {
    Logs.call(this, "dumpUiElementTree", path);
    return ByXPath.dumpDom(path, getRootElement());
  }
}
