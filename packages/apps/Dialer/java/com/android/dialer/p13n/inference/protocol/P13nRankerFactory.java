/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.dialer.p13n.inference.protocol;

import android.support.annotation.Nullable;

/**
 * This interface should be implemented by the Application subclass. It allows this module to get
 * references to the {@link P13nRanker}.
 */
public interface P13nRankerFactory {
  @Nullable
  P13nRanker newP13nRanker();
}
