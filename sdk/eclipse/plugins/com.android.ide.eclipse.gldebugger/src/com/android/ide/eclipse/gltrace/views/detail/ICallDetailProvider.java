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

package com.android.ide.eclipse.gltrace.views.detail;

import com.android.ide.eclipse.gltrace.model.GLCall;
import com.android.ide.eclipse.gltrace.model.GLTrace;

public interface ICallDetailProvider extends IDetailProvider {
    /** Is this provider applicable for given {@link GLCall}? */
    boolean isApplicable(GLCall call);

    /**
     * Update the detail view for given {@link GLCall} that is part of the given
     * {@link GLTrace}.
     */
    void updateControl(GLTrace trace, GLCall call);
}
