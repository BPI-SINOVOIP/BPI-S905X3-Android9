/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.dialer.compat;

import android.content.Context;
import android.os.Build;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.os.LocaleList;
import java.util.Locale;

public final class CompatUtils {

  /** PrioritizedMimeType is added in API level 23. */
  public static boolean hasPrioritizedMimeType() {
    return SdkVersionOverride.getSdkVersion(Build.VERSION_CODES.M) >= Build.VERSION_CODES.M;
  }

  /**
   * Determines if this version is compatible with multi-SIM and the phone account APIs. Can also
   * force the version to be lower through SdkVersionOverride.
   *
   * @return {@code true} if multi-SIM capability is available, {@code false} otherwise.
   */
  public static boolean isMSIMCompatible() {
    return SdkVersionOverride.getSdkVersion(Build.VERSION_CODES.LOLLIPOP)
        >= Build.VERSION_CODES.LOLLIPOP_MR1;
  }

  /**
   * Determines if this version is compatible with video calling. Can also force the version to be
   * lower through SdkVersionOverride.
   *
   * @return {@code true} if video calling is allowed, {@code false} otherwise.
   */
  public static boolean isVideoCompatible() {
    return SdkVersionOverride.getSdkVersion(Build.VERSION_CODES.LOLLIPOP) >= Build.VERSION_CODES.M;
  }

  /**
   * Determines if this version is capable of using presence checking for video calling. Support for
   * video call presence indication is added in SDK 24.
   *
   * @return {@code true} if video presence checking is allowed, {@code false} otherwise.
   */
  public static boolean isVideoPresenceCompatible() {
    return SdkVersionOverride.getSdkVersion(Build.VERSION_CODES.M) > Build.VERSION_CODES.M;
  }

  /**
   * Determines if this version is compatible with call subject. Can also force the version to be
   * lower through SdkVersionOverride.
   *
   * @return {@code true} if call subject is a feature on this device, {@code false} otherwise.
   */
  public static boolean isCallSubjectCompatible() {
    return SdkVersionOverride.getSdkVersion(Build.VERSION_CODES.LOLLIPOP) >= Build.VERSION_CODES.M;
  }

  /** Returns locale of the device. */
  public static Locale getLocale(Context context) {
    if (VERSION.SDK_INT >= VERSION_CODES.N) {
      LocaleList localList = context.getResources().getConfiguration().getLocales();
      if (!localList.isEmpty()) {
        return localList.get(0);
      }
      return Locale.getDefault();
    } else {
      return context.getResources().getConfiguration().locale;
    }
  }
}
