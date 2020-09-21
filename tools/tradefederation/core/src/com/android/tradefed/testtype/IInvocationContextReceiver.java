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
package com.android.tradefed.testtype;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.targetprep.ITargetPreparer;

/**
 * A test that needs reference to the context of the invocation.
 * <p/>
 * Should be used when test needs to know the details of the invocation without needing a reference
 * to the {@link IBuildInfo}.
 * Most tests should not have a dependency on the build-under-test, and
 * should rely on {@link ITargetPreparer}s to prepare the test environment.
 */
public interface IInvocationContextReceiver {

    public void setInvocationContext(IInvocationContext invocationContext);
}
