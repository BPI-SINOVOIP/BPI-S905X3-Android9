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

package io.appium.droiddriver.duo;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.Instrumentation;

import io.appium.droiddriver.base.AbstractDroidDriver;
import io.appium.droiddriver.base.CompositeDroidDriver;
import io.appium.droiddriver.instrumentation.InstrumentationDriver;
import io.appium.droiddriver.uiautomation.UiAutomationDriver;
import io.appium.droiddriver.util.ActivityUtils;
import io.appium.droiddriver.util.InstrumentationUtils;

/**
 * Implementation of DroidDriver that attempts to use the best driver for the current activity.
 * If the activity is part of the application under instrumentation, the InstrumentationDriver is
 * used. Otherwise, the UiAutomationDriver is used.
 */
@TargetApi(18)
public class DuoDriver extends CompositeDroidDriver {
  private final String targetApkPackage;
  private final UiAutomationDriver uiAutomationDriver;
  private final InstrumentationDriver instrumentationDriver;

  public DuoDriver() {
    Instrumentation instrumentation = InstrumentationUtils.getInstrumentation();
    targetApkPackage = InstrumentationUtils.getTargetContext().getPackageName();
    uiAutomationDriver = new UiAutomationDriver(instrumentation);
    instrumentationDriver = new InstrumentationDriver(instrumentation);
  }

  @Override
  protected AbstractDroidDriver getApplicableDriver() {
    Activity activity = ActivityUtils.getRunningActivity();
    if (activity != null && targetApkPackage.equals(
            activity.getApplicationContext().getPackageName())) {
      return instrumentationDriver;
    }
    return uiAutomationDriver;
  }
}
