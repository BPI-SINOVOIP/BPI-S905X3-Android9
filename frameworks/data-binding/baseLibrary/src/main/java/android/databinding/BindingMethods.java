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
package android.databinding;

import java.lang.annotation.ElementType;
import java.lang.annotation.Target;

/**
 * Used to enumerate attribute-to-setter renaming. By default, an attribute is associated with
 * setAttribute setter. If there is a simple rename, enumerate them in an array of
 * {@link BindingMethod} annotations in the value.
 */
@Target({ElementType.TYPE})
public @interface BindingMethods {
    BindingMethod[] value();
}
