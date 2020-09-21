/*
 * Copyright (C) 2014 Google, Inc.
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
package dagger.internal.codegen;

import com.google.common.base.Ascii;
import com.google.common.base.Function;
import dagger.Lazy;
import javax.inject.Provider;

/**
 * Picks a reasonable name for what we think is being provided from the variable name associated
 * with the {@link DependencyRequest}.  I.e. strips out words like "lazy" and "provider" if we
 * believe that those refer to {@link Lazy} and {@link Provider} rather than the type being
 * provided.
 *
 * @author Gregory Kick
 * @since 2.0
 */
//TODO(gak): develop the heuristics to get better names
final class DependencyVariableNamer implements Function<DependencyRequest, String> {
  @Override
  public String apply(DependencyRequest dependency) {
    if (dependency.overriddenVariableName().isPresent()) {
      return dependency.overriddenVariableName().get();
    }
    String variableName = dependency.requestElement().getSimpleName().toString();
    switch (dependency.kind()) {
      case INSTANCE:
        return variableName;
      case LAZY:
        return variableName.startsWith("lazy") && !variableName.equals("lazy")
            ? Ascii.toLowerCase(variableName.charAt(4)) + variableName.substring(5)
            : variableName;
      case PROVIDER:
        return variableName.endsWith("Provider") && !variableName.equals("Provider")
            ? variableName.substring(0, variableName.length() - 8)
            : variableName;
      case MEMBERS_INJECTOR:
        return variableName.endsWith("MembersInjector") && !variableName.equals("MembersInjector")
            ? variableName.substring(0, variableName.length() - 15)
            : variableName;
      case PRODUCED:
        return variableName.startsWith("produced") && !variableName.equals("produced")
            ? Ascii.toLowerCase(variableName.charAt(8)) + variableName.substring(9)
            : variableName;
      case PRODUCER:
        return variableName.endsWith("Producer") && !variableName.equals("Producer")
            ? variableName.substring(0, variableName.length() - 8)
            : variableName;
      default:
        throw new AssertionError();
    }
  }
}
