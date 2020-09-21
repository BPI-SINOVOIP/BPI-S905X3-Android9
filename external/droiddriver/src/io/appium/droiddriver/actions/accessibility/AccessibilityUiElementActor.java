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

package io.appium.droiddriver.actions.accessibility;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.actions.UiElementActor;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;

/**
 * A {@link UiElementActor} that performs actions via the Accessibility API.
 */
public class AccessibilityUiElementActor implements UiElementActor {
  public static final AccessibilityUiElementActor INSTANCE = new AccessibilityUiElementActor();

  @Override
  public void click(UiElement uiElement) {
    uiElement.perform(AccessibilityClickAction.SINGLE);
  }

  @Override
  public void longClick(UiElement uiElement) {
    uiElement.perform(AccessibilityClickAction.LONG);
  }

  @Override
  public void doubleClick(UiElement uiElement) {
    uiElement.perform(AccessibilityClickAction.DOUBLE);
  }

  @Override
  public void scroll(UiElement uiElement, PhysicalDirection direction) {
    uiElement.perform(new AccessibilityScrollAction(direction));
  }
}
