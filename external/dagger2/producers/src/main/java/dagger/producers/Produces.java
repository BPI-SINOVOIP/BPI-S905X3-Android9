/*
 * Copyright (C) 2014 Google Inc.
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
package dagger.producers;

import dagger.internal.Beta;
import com.google.common.util.concurrent.ListenableFuture;
import java.lang.annotation.Documented;
import java.lang.annotation.Target;

import static java.lang.annotation.ElementType.METHOD;

/**
 * Annotates methods of a producer module to create a production binding. If the method returns
 * a {@link ListenableFuture}, then the parameter type of the future is bound to the value that the
 * future provides; otherwise, the return type is bound to the returned value. The production
 * component will pass dependencies to the method as parameters.
 *
 * @author Jesse Beder
 */
@Documented
@Target(METHOD)
@Beta
public @interface Produces {
  /** The type of binding into which the return type of the annotated method contributes. */
  enum Type {
    /**
     * The method is the only one which can produce the value for the specified type. This is the
     * default behavior.
     */
    UNIQUE,

    /**
     * The method's resulting type forms the generic type argument of a {@code Set<T>}, and the
     * returned value or future is contributed to the set. The {@code Set<T>} produced from the
     * accumulation of values will be immutable.
     */
    SET,

    /**
     * Like {@link #SET}, except the method's return type is either {@code Set<T>} or
     * {@code Set<ListenableFuture<T>>}, where any values are contributed to the set. An example use
     * is to provide a default empty set binding, which is otherwise not possible using
     * {@link #SET}.
     */
    SET_VALUES,

    /**
     * The method's return type forms the type argument for the value of a
     * {@code Map<K, Producer<V>>}, and the combination of the annotated key and the returned value
     * is contributed to the map as a key/value pair. The {@code Map<K, Producer<V>>} produced from
     * the accumulation of values will be immutable.
     */
    MAP;
  }

  Type type() default Type.UNIQUE;
}
