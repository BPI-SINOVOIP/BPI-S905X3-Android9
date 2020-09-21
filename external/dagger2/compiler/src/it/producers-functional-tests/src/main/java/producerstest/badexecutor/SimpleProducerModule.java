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
package producerstest.badexecutor;

import dagger.producers.ProducerModule;
import dagger.producers.Produces;

@ProducerModule
final class SimpleProducerModule {
  @Produces
  static String noArgStr() {
    return "no arg string";
  }

  @Produces
  static int singleArgInt(String arg) {
    return arg.length();
  }

  @Produces
  static boolean singleArgBool(double arg) {
    return arg > 0.0;
  }
}
