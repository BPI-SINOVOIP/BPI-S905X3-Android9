/*
 * Copyright (C) 2018 Google Inc.
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
package com.android.car.settingslib.robolectric;

import org.junit.Rule;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

/**
 * Base test for CarSettingsLib Robolectric tests that sets the manifest and sdk config parameters
 */
@Config(packageName = "com.android.car.settingslib")
public abstract class BaseRobolectricTest {
    //This rule automatically initializes any mocks created using the @Mock annotation
    @Rule
    public MockitoRule mMockitoRule = MockitoJUnit.rule();
}
