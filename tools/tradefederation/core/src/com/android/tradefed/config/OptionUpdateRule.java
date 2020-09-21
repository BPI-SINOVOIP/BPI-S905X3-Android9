/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.tradefed.config;

import java.lang.reflect.Field;
import java.util.Collection;  // imported for javadoc
import java.util.Map;  // imported for javadoc

/**
 * Controls the behavior when an option is specified multiple times.  Note that this enum assumes
 * that the values to be set are not {@link Collection}s or {@link Map}s.
 */
public enum OptionUpdateRule {
    /** once an option is set, subsequent attempts to update it should be ignored. */
    FIRST {
        @Override
        boolean shouldUpdate(String optionName, Object current, Object update)
                throws ConfigurationException {
            return current == null;
        }
    },

    /** if an option is set multiple times, ignore all but the last value. */
    LAST {
        @Override
        boolean shouldUpdate(String optionName, Object current, Object update)
                throws ConfigurationException {
            return true;
        }
    },

    /** for {@link Comparable} options, keep the one that compares as the greatest. */
    GREATEST {
        @Override
        boolean shouldUpdate(String optionName, Object current, Object update)
                throws ConfigurationException {
            return current == null || compare(optionName, current, update) < 0;
        }
    },

    /** for {@link Comparable} options, keep the one that compares as the least. */
    LEAST {
        @Override
        boolean shouldUpdate(String optionName, Object current, Object update)
                throws ConfigurationException {
            return current == null || compare(optionName, current, update) > 0;
        }
    },

    /** throw a {@link ConfigurationException} if this option is set more than once. */
    IMMUTABLE {
        @Override
        boolean shouldUpdate(String optionName, Object current, Object update)
                throws ConfigurationException {

            if (current != null) {
                throw new ConfigurationException(String.format(
                        "Attempted to update immutable value (%s) for option \"%s\"", optionName,
                        optionName));
            }
            return true;
        }
    };

    abstract boolean shouldUpdate(String optionName, Object current, Object update)
            throws ConfigurationException;

    /**
      * Takes the current value and the update value, and returns whether the value should be
      * updated.  Assumes that <code>update</code> is never null.
      */
    public boolean shouldUpdate(String optionName, Object optionSource, Field field, Object update)
            throws ConfigurationException {
        Object current;
        try {
            current = field.get(optionSource);
        } catch (IllegalAccessException e) {
            throw new ConfigurationException(String.format(
                    "internal error when setting option '%s'", optionName), e);
        }
        return shouldUpdate(optionName, current, update);
    }

    /**
     * Check if the objects are {@link Comparable}, and if so, compare them using
     * {@link Comparable#compareTo(Object)}
     */
    @SuppressWarnings({"unchecked", "rawtypes"})
    private static int compare(String optionName, Object current, Object update)
            throws ConfigurationException {
        Comparable compCurrent;
        if (current instanceof Comparable) {
            compCurrent = (Comparable) current;
        } else {
            throw new ConfigurationException(String.format(
                    "internal error: Class %s for option %s was used with GREATEST or LEAST " +
                    "updateRule, but does not implement Comparable.",
                    current.getClass().getSimpleName(), optionName));
        }

        try {
            return compCurrent.compareTo(update);
        } catch (ClassCastException e) {
            throw new ConfigurationException(String.format(
                    "internal error: Failed to compare %s (%s) and %s (%s)",
                    current.getClass().getName(), current, update.getClass().getName(), update), e);
        }
    }
}

