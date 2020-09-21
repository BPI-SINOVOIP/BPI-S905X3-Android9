/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.signature.cts.tests.data;

/**
 * A public API class, with some members are marked as system API
 */
public class PublicApiClass extends PublicApiClassParent {
    public PublicApiClass(boolean foo) {
    }

    public void publicApiMethod() {
    }

    public boolean publicApiField;

    /** @hide */
    @ApiAnnotation
    public PublicApiClass() {
    }

    /** @hide */
    @ApiAnnotation
    public void apiMethod() {
    }

    /** @hide */
    @ApiAnnotation
    public boolean apiField;

    /** @hide */
    public PublicApiClass(int foo) {
    }

    /** @hide */
    public void privateMethod() {
    }

    /** @hide */
    public boolean privateField;

    /** @hide */
    @Override
    public void anOverriddenMethod() {
        // This is @hide but should be recognized as an API because the exact same method is defined
        // as API in one of the ancestor classes.
    }
}
