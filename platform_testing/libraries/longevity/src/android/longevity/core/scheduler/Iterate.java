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
package android.longevity.core.scheduler;

import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.junit.runner.Runner;

/**
 * A {@link Scheduler} for repeating tests a configurable number of times.
 */
public class Iterate implements Scheduler {
    public static final String OPTION_NAME = "iterations";
    private static final int DEFAULT_VALUE = 1;

    @Override
    public List<Runner> apply(Map<String, String> args, List<Runner> input) {
        int iterations = args.containsKey(OPTION_NAME) ?
            Integer.parseInt(args.get(OPTION_NAME)) : DEFAULT_VALUE;
        // TODO: Log the options selected.
        return Collections.nCopies(iterations, input)
            .stream()
            .flatMap(Collection::stream)
            .collect(Collectors.toList());
    }
}
