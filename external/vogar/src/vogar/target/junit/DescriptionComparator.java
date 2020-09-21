/*
 * Copyright (C) 2016 The Android Open Source Project
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

package vogar.target.junit;

import java.util.Comparator;
import org.junit.runner.Description;

/**
 * Order by class name first, then display name.
 */
public class DescriptionComparator implements Comparator<Description> {

    private static final DescriptionComparator DESCRIPTION_COMPARATOR =
            new DescriptionComparator();

    public static DescriptionComparator getInstance() {
        return DESCRIPTION_COMPARATOR;
    }

    private DescriptionComparator() {
    }

    @Override
    public int compare(Description d1, Description d2) {
        String c1 = d1.getClassName();
        String c2 = d2.getClassName();
        int result = c1.compareTo(c2);
        if (result != 0) {
            return result;
        }

        return d1.getDisplayName().compareTo(d2.getDisplayName());
    }
}
