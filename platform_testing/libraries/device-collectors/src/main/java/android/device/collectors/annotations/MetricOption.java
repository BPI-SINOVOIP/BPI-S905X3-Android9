/*
 * Copyright (C) 2017 The Android Open Source Project
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
package android.device.collectors.annotations;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Annotation for test methods annotated with {@code @Test} that allows to specify some extra
 * parameters useful for: Tuning the behavior of the collectors, filtering some methods.
 */
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.METHOD})
public @interface MetricOption {

    /**
     * Defines a group name, all method annotated with the same name will be considered part of
     * the same group. By default or when the annotation is not specified method will be considered
     * part of a default group. Several groups can be specified using comma separated values.
     * For example: @MetricOption(group = "testGroup,testGroup2")
     */
    String group() default "";
}
