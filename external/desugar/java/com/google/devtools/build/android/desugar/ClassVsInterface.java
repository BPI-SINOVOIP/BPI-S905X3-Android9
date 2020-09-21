// Copyright 2017 The Bazel Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package com.google.devtools.build.android.desugar;

import static com.google.common.base.Preconditions.checkNotNull;
import static com.google.common.base.Preconditions.checkState;

import java.util.HashMap;
import javax.annotation.Nullable;
import org.objectweb.asm.ClassReader;

/**
 * Simple memoizer for whether types are classes or interfaces.
 */
class ClassVsInterface {
  /** Map from internal names to whether they are an interface ({@code false} thus means class). */
  private final HashMap<String, Boolean> known = new HashMap<>();
  private final ClassReaderFactory classpath;

  public ClassVsInterface(ClassReaderFactory classpath) {
    this.classpath = classpath;
  }

  public ClassVsInterface addKnownClass(@Nullable String internalName) {
    if (internalName != null) {
      Boolean previous = known.put(internalName, false);
      checkState(previous == null || !previous, "Already recorded as interface: %s", internalName);
    }
    return this;
  }

  public ClassVsInterface addKnownInterfaces(String... internalNames) {
    for (String internalName : internalNames) {
      Boolean previous = known.put(internalName, true);
      checkState(previous == null || previous, "Already recorded as class: %s", internalName);
    }
    return this;
  }

  public boolean isOuterInterface(String outerName, String innerName) {
    Boolean result = known.get(outerName);
    if (result == null) {
      // We could just load the outer class here, but this tolerates incomplete classpaths better.
      // Note the outer class should be in the Jar we're desugaring, so it should always be there.
      ClassReader outerClass = checkNotNull(classpath.readIfKnown(outerName),
          "Couldn't find outer class %s of %s", outerName, innerName);
      result = BitFlags.isInterface(outerClass.getAccess());
      known.put(outerName, result);
    }
    return result;
  }
}
