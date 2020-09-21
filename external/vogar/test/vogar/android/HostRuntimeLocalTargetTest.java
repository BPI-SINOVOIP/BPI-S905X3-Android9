/*
 * Copyright (C) 2015 The Android Open Source Project
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

import com.google.common.base.Function;
import com.google.common.collect.Lists;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;
import vogar.LocalTarget;
import vogar.Mode;
import vogar.ModeId;
import vogar.Target;
import vogar.Variant;
import vogar.commands.Command;
import vogar.commands.VmCommandBuilder;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

/**
 * Test the behaviour of the {@link HostRuntime} class when run with {@link LocalTarget}.
 */
@RunWith(MockitoJUnitRunner.class)
public class HostRuntimeLocalTargetTest extends AbstractModeTest {

    public static final String ANDROID_BUILD_TOP_REFERENCE =
            Matcher.quoteReplacement("${ANDROID_BUILD_TOP}/");

    @Override
    protected Target createTarget() {
        return new LocalTarget(console, mkdir, rm);
    }

    @Test
    @VogarArgs({"action"})
    public void testLocalTarget()
            throws IOException {

        Mode hostRuntime = new HostRuntime(run, ModeId.HOST, Variant.X32);

        VmCommandBuilder builder = newVmCommandBuilder(hostRuntime)
                .classpath(classpath)
                .mainClass("mainclass")
                .args("-x", "a b");
        Command command = builder.build(run.target);
        List<String> args = command.getArgs();
        args = replaceEnvironmentVariables(args);

        assertEquals(Arrays.asList(
                "sh", "-c", ""
                        + "ANDROID_PRINTF_LOG=tag"
                        + " ANDROID_LOG_TAGS=*:i"
                        + " ANDROID_DATA=" + run.localFile("android-data")
                        + " ANDROID_ROOT=${ANDROID_BUILD_TOP}/out/host/linux-x86"
                        + " LD_LIBRARY_PATH=${ANDROID_BUILD_TOP}/out/host/linux-x86/lib"
                        + " DYLD_LIBRARY_PATH=${ANDROID_BUILD_TOP}/out/host/linux-x86/lib"
                        + " LD_USE_LOAD_BIAS=1"
                        + " ${ANDROID_BUILD_TOP}/out/host/linux-x86/bin/dalvikvm32"
                        + " -classpath classes"
                        + " -Xbootclasspath"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/core-oj-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/core-libart-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/conscrypt-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/okhttp-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/bouncycastle-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/apache-xml-hostdex.jar"
                        + " -Duser.language=en"
                        + " -Duser.region=US"
                        + " -Xcheck:jni"
                        + " -Xjnigreflimit:2000"
                        + " mainclass"
                        + " -x a\\ b"), args);
    }

    public List<String> replaceEnvironmentVariables(List<String> args) {
        String androidBuildTop = System.getenv("ANDROID_BUILD_TOP");
        assertNotNull("ANDROID_BUILD_TOP not set", androidBuildTop);
        Path path = Paths.get(androidBuildTop).normalize();
        final String prefix = Pattern.quote(path.toString() + "/");
        args = Lists.transform(args, new Function<String, String>() {
            @Override
            public String apply(String input) {
                return input.replaceAll(prefix, ANDROID_BUILD_TOP_REFERENCE);
            }
        });
        return args;
    }

    @Test
    @VogarArgs({"--benchmark", "action"})
    public void testLocalTarget_Benchmark()
            throws IOException {

        Mode hostRuntime = new HostRuntime(run, ModeId.HOST, Variant.X32);

        VmCommandBuilder builder = newVmCommandBuilder(hostRuntime)
                .classpath(classpath)
                .mainClass("mainclass")
                .args("-x", "a b");
        Command command = builder.build(run.target);
        List<String> args = command.getArgs();
        args = replaceEnvironmentVariables(args);

        assertEquals(Arrays.asList(
                "sh", "-c", ""
                        + "ANDROID_PRINTF_LOG=tag"
                        + " ANDROID_LOG_TAGS=*:i"
                        + " ANDROID_DATA=" + run.localFile("android-data")
                        + " ANDROID_ROOT=${ANDROID_BUILD_TOP}/out/host/linux-x86"
                        + " LD_LIBRARY_PATH=${ANDROID_BUILD_TOP}/out/host/linux-x86/lib"
                        + " DYLD_LIBRARY_PATH=${ANDROID_BUILD_TOP}/out/host/linux-x86/lib"
                        + " LD_USE_LOAD_BIAS=1"
                        + " ${ANDROID_BUILD_TOP}/out/host/linux-x86/bin/dalvikvm32"
                        + " -classpath classes"
                        + " -Xbootclasspath"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/core-oj-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/core-libart-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/conscrypt-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/okhttp-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/bouncycastle-hostdex.jar"
                        + ":${ANDROID_BUILD_TOP}/out/host/linux-x86/framework/apache-xml-hostdex.jar"
                        + " -Duser.language=en"
                        + " -Duser.region=US"
                        + " -Xjnigreflimit:2000"
                        + " mainclass"
                        + " -x a\\ b"), args);
    }
}
