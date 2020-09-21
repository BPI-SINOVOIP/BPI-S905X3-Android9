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

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.Poller;
import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.actions.InputInjector;
import io.appium.droiddriver.exceptions.ElementNotFoundException;
import io.appium.droiddriver.exceptions.TimeoutException;
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.util.Logs;

/**
 * Base DroidDriver that implements the common operations.
 */
public abstract class AbstractDroidDriver implements DroidDriver {

  private Poller poller = new DefaultPoller();

  @Override
  public boolean has(Finder finder) {
    try {
      refreshUiElementTree();
      find(finder);
      return true;
    } catch (ElementNotFoundException enfe) {
      return false;
    }
  }

  @Override
  public boolean has(Finder finder, long timeoutMillis) {
    try {
      getPoller().pollFor(this, finder, Poller.EXISTS, timeoutMillis);
      return true;
    } catch (TimeoutException e) {
      return false;
    }
  }

  @Override
  public UiElement on(Finder finder) {
    Logs.call(this, "on", finder);
    return getPoller().pollFor(this, finder, Poller.EXISTS);
  }

  @Override
  public void checkExists(Finder finder) {
    Logs.call(this, "checkExists", finder);
    getPoller().pollFor(this, finder, Poller.EXISTS);
  }

  @Override
  public void checkGone(Finder finder) {
    Logs.call(this, "checkGone", finder);
    getPoller().pollFor(this, finder, Poller.GONE);
  }

  @Override
  public Poller getPoller() {
    return poller;
  }

  @Override
  public void setPoller(Poller poller) {
    this.poller = poller;
  }

  public abstract InputInjector getInjector();

}