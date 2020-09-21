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
import org.junit.runner.Describable;
import org.junit.runner.Description;

/**
 * A comparator of {@link Describable} instances.
 *
 * <p>Compares two {@link Describable} objects by comparing their
 * {@link Describable#getDescription() Description} using the
 * {@link DescriptionComparator#getInstance()}.
 */
public class DescribableComparator implements Comparator<Describable> {

    private static final DescribableComparator DESCRIBABLE_COMPARATOR =
            new DescribableComparator();
    private DescriptionComparator descriptionComparator = DescriptionComparator.getInstance();

    public static DescribableComparator getInstance() {
        return DESCRIBABLE_COMPARATOR;
    }

    private DescribableComparator() {
    }

    @Override
    public int compare(Describable d1, Describable d2) {
        return descriptionComparator.compare(d1.getDescription(), d2.getDescription());
    }
}
