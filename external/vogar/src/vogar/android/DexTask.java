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

package vogar.android;

import java.io.File;
import vogar.Action;
import vogar.Classpath;
import vogar.Dexer;
import vogar.Result;
import vogar.Run;
import vogar.tasks.Task;
import vogar.Toolchain;

public final class DexTask extends Task {
    private final Dexer dexer;
    private final AndroidSdk androidSdk;
    private final Classpath classpath;
    private final boolean benchmark;
    private final File jar;
    private final Action action;
    private final File localDex;
    private final File localTempDir;
    private final boolean multidex;

    public DexTask(Dexer dexer, AndroidSdk androidSdk, Classpath classpath, boolean benchmark, String name,
            File jar, Action action, File localDex, File localTempDir, boolean multidex) {
        super("dex " + name);
        this.dexer = dexer;
        this.androidSdk = androidSdk;
        this.classpath = classpath;
        this.benchmark = benchmark;
        this.jar = jar;
        this.action = action;
        this.localDex = localDex;
        this.localTempDir = localTempDir;
        this.multidex = multidex;
    }

    @Override protected Result execute() throws Exception {
        // make the local dex (inside a jar)
        Classpath cp = Classpath.of(jar);
        Classpath dependentCp = classpath;
        if (benchmark && action != null) {
            cp.addAll(classpath);
            // Everything is already in 'cp' so the dependent classpath becomes empty.
            dependentCp = new Classpath();
        }
        androidSdk.dex(multidex, localDex, localTempDir, cp, dependentCp, dexer);
        return Result.SUCCESS;
    }
}
