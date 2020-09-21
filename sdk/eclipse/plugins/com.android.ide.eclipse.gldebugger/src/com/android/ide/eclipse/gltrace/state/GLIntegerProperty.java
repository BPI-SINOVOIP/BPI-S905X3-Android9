/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.ide.eclipse.gltrace.state;

/** Properties that hold an integer value. */
public class GLIntegerProperty extends GLAbstractAtomicProperty {
    private final Integer mDefaultValue;
    private Integer mCurrentValue;
    private final DisplayRadix mRadix;

    public GLIntegerProperty(GLStateType name, Integer defaultValue, DisplayRadix radix) {
        super(name);

        mDefaultValue = mCurrentValue = defaultValue;
        mRadix = radix;
    }

    public GLIntegerProperty(GLStateType name, Integer defaultValue) {
        this(name, defaultValue, DisplayRadix.DECIMAL);
    }

    @Override
    public boolean isDefault() {
        return mDefaultValue != null & mDefaultValue.equals(mCurrentValue);
    }

    public void setValue(Integer newValue) {
        mCurrentValue = newValue;
    }

    @Override
    public String getStringValue() {
        if (mRadix == DisplayRadix.HEX) {
            return String.format("0x%08x", Integer.valueOf(mCurrentValue));
        }

        return mCurrentValue.toString();
    }

    @Override
    public String toString() {
        return getType() + "=" + getStringValue(); //$NON-NLS-1$
    }

    @Override
    public void setValue(Object value) {
        if (value instanceof Integer) {
            mCurrentValue = (Integer) value;
        } else {
            throw new IllegalArgumentException("Attempt to set non-integer value for " //$NON-NLS-1$
                                    + getType());
        }
    }

    @Override
    public Object getValue() {
        return mCurrentValue;
    }
}
