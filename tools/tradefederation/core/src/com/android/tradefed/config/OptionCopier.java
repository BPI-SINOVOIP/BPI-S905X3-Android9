/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.tradefed.log.LogUtil.CLog;

import java.lang.reflect.Field;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * A helper class that can copy {@link Option} field values with same names from one object to
 * another.
 */
public class OptionCopier {

    /**
     * Copy the values from {@link Option} fields in <var>origObject</var> to <var>destObject</var>
     *
     * @param origObject the {@link Object} to copy from
     * @param destObject the {@link Object} tp copy to
     * @throws ConfigurationException if options failed to copy
     */
    public static void copyOptions(Object origObject, Object destObject)
            throws ConfigurationException {
        Collection<Field> origFields = OptionSetter.getOptionFieldsForClass(origObject.getClass());
        Map<String, Field> destFieldMap = getFieldOptionMap(destObject);
        for (Field origField : origFields) {
            final Option option = origField.getAnnotation(Option.class);
            Field destField = destFieldMap.remove(option.name());
            if (destField != null) {
                Object origValue = OptionSetter.getFieldValue(origField,
                        origObject);
                OptionSetter.setFieldValue(option.name(), destObject, destField, origValue);
            }
        }
    }

    /**
     * Identical to {@link #copyOptions(Object, Object)} but will log instead of throw if exception
     * occurs.
     */
    public static void copyOptionsNoThrow(Object source, Object dest) {
        try {
            copyOptions(source, dest);
        } catch (ConfigurationException e) {
            CLog.e(e);
        }
    }

    /**
     * Build a map of {@link Option#name()} to {@link Field} for given {@link Object}.
     *
     * @param destObject
     * @return a {@link Map}
     */
    private static Map<String, Field> getFieldOptionMap(Object destObject) {
        Collection<Field> destFields = OptionSetter.getOptionFieldsForClass(destObject.getClass());
        Map<String, Field> fieldMap = new HashMap<String, Field>(destFields.size());
        for (Field field : destFields) {
            Option o = field.getAnnotation(Option.class);
            fieldMap.put(o.name(), field);
        }
        return fieldMap;
    }
}
