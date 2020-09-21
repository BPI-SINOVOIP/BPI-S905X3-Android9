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

package libcore.java.lang;

public class NoClassDefFoundErrorTest extends junit.framework.TestCase {
    // The Android runtime requires a constructor accepting a cause.
    public void test_NoClassDefFoundError_constructor_with_cause()  throws Exception {
        Class<NoClassDefFoundError> klass = NoClassDefFoundError.class;
        // This will succeed if the constructor is declared in NoClassDefFoundError.
        klass.getDeclaredConstructor(String.class, Throwable.class);
    }
}
