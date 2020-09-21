/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.dialer.assisteddialing;

import android.annotation.TargetApi;
import android.os.Build.VERSION_CODES;
import android.support.annotation.NonNull;
import java.util.Optional;

/** The core interface for the AssistedDialingMediator. */
public interface AssistedDialingMediator {

  /** Returns {@code true} if the current client platform supports Assisted Dialing. */
  public boolean isPlatformEligible();

  /** Returns the country code in which the library thinks the user typically resides. */
  @SuppressWarnings("AndroidApiChecker") // Use of optional
  @TargetApi(VERSION_CODES.N)
  public Optional<String> userHomeCountryCode();

  @SuppressWarnings("AndroidApiChecker") // Use of optional
  @TargetApi(VERSION_CODES.N)
  public Optional<TransformationInfo> attemptAssistedDial(@NonNull String numberToTransform);
}
