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
package com.android.tradefed.targetprep;

import com.android.tradefed.config.Option;

/**
 * Base implementation class for {@link ITargetPreparer} that allows to control whether the object
 * is disabled or not.
 */
public abstract class BaseTargetPreparer implements ITargetPreparer {

    @Option(name = "disable", description = "disables the target preparer")
    private boolean mDisable = false;

    @Option(
        name = "disable-tear-down",
        description = "disables the clean up step of a target cleaner"
    )
    private boolean mDisableTearDown = false;

    /** {@inheritDoc} */
    @Override
    public final boolean isDisabled() {
        return mDisable;
    }

    /** {@inheritDoc} */
    @Override
    public final boolean isTearDownDisabled() {
        return mDisableTearDown;
    }

    /** {@inheritDoc} */
    @Override
    public final void setDisable(boolean isDisabled) {
        mDisable = isDisabled;
    }

    /** {@inheritDoc} */
    @Override
    public final void setDisableTearDown(boolean isDisabled) {
        mDisableTearDown = isDisabled;
    }
}
