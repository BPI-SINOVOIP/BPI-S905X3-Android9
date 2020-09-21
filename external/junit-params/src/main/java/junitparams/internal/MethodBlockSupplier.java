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
package junitparams.internal;

import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.Statement;

/**
 * Wraps the {@link Statement} provided by
 * {@link InvokableFrameworkMethod#getInvokeStatement(Object)} with additional {@link Statement}.
 *
 * <p>This is essentially a method reference to
 * {@link BlockJUnit4ClassRunner#methodBlock(FrameworkMethod)}
 */
public interface MethodBlockSupplier {

    Statement getMethodBlock(InvokableFrameworkMethod method);
}
