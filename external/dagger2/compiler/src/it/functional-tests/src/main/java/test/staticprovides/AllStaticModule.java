/*
 * Copyright (C) 2015 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package test.staticprovides;

import static dagger.Provides.Type.SET;
import static dagger.Provides.Type.SET_VALUES;
import static java.util.Collections.emptySet;

import dagger.Module;
import dagger.Provides;
import java.util.Set;

@Module
final class AllStaticModule {
  @Provides(type = SET) static String contributeString() {
    return AllStaticModule.class + ".contributeString";
  }

  @Provides(type = SET_VALUES) static Set<Integer> contibuteEmptyIntegerSet() {
    return emptySet();
  }
}
