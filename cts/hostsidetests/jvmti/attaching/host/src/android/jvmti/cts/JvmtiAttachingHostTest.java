/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package android.jvmti.cts;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.ddmlib.NullOutputReceiver;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.util.AbiUtils;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.ZipUtil;

import java.io.File;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipFile;

/**
 * Specialization of JvmtiHostTest to test attaching on startup.
 */
public class JvmtiAttachingHostTest extends DeviceTestCase implements IBuildReceiver, IAbiReceiver {
    // inject these options from HostTest directly using --set-option <option name>:<option value>
    @Option(name = "package-name",
            description = "The package name of the device test",
            mandatory = true)
    private String mTestPackageName = null;

    @Option(name = "test-file-name",
            description = "the name of a test zip file to install on device.",
            mandatory = true)
    private String mTestApk = null;

    private CompatibilityBuildHelper mBuildHelper;
    private IAbi mAbi;

    @Override
    public void setBuild(IBuildInfo arg0) {
        mBuildHelper = new CompatibilityBuildHelper(arg0);
    }

    @Override
    public void setAbi(IAbi arg0) {
        mAbi = arg0;
    }

    private static interface TestRun {
        public void run(ITestDevice device, String pkg, String apk, String abiName);
    }

    private final static String AGENT = "libctsjvmtiattachagent.so";

    public void testJvmtiAttachDuringBind() throws Exception {
        runJvmtiAgentLoadTest((ITestDevice device, String pkg, String apk, String abiName) -> {
            try {
                runAttachTestCmd(device, pkg, "--attach-agent-bind " + AGENT);
            } catch (Exception e) {
                throw new RuntimeException("Failed bind-time attaching", e);
            }
        });
    }

    public void testJvmtiAttachEarly() throws Exception {
        runJvmtiAgentLoadTest((ITestDevice device, String pkg, String apk, String abiName) -> {
            try {
                String pwd = device.executeShellCommand("run-as " + pkg + " pwd");
                if (pwd == null) {
                    throw new RuntimeException("pwd failed");
                }
                pwd = pwd.trim();
                if (pwd.isEmpty()) {
                    throw new RuntimeException("pwd failed");
                }

                // Give it a different name, so we do not have "contamination" from
                // the test APK.
                String libInDataData = AGENT.substring(0, AGENT.length() - ".so".length())
                        + "2.so";
                String agentInDataData =
                        installLibToDataData(device, pkg, abiName, apk, pwd, AGENT,
                                libInDataData);
                runAttachTestCmd(device, pkg, "--attach-agent " + agentInDataData);
            } catch (Exception e) {
                throw new RuntimeException("Failed pre-bind attaching", e);
            }
        });
    }

    public void testJvmtiAgentAppInternal() throws Exception {
        runJvmtiAgentLoadTest((ITestDevice device, String pkg, String apk, String abiName) -> {
            try {
                String setAgentAppCmd = "cmd activity set-agent-app " + pkg + " " + AGENT;
                device.executeShellCommand(setAgentAppCmd);
            } catch (Exception e) {
                throw new RuntimeException("Failed running set-agent-app", e);
            }

            try {
                runAttachTestCmd(device, pkg, "");

                // And again.
                runAttachTestCmd(device, pkg, "");
            } catch (Exception e) {
                throw new RuntimeException("Failed agent-app attaching", e);
            }
        });
    }

    public void testJvmtiAgentAppExternal() throws Exception {
        runJvmtiAgentLoadTest((ITestDevice device, String pkg, String apk, String abiName) -> {
            try {
                String pwd = device.executeShellCommand("run-as " + pkg + " pwd");
                if (pwd == null) {
                    throw new RuntimeException("pwd failed");
                }
                pwd = pwd.trim();
                if (pwd.isEmpty()) {
                    throw new RuntimeException("pwd failed");
                }

                // Give it a different name, so we do not have "contamination" from
                // the test APK.
                String libInDataData = AGENT.substring(0, AGENT.length() - ".so".length())
                        + "2.so";
                String agentInDataData =
                        installLibToDataData(device, pkg, abiName, apk, pwd, AGENT,
                                libInDataData);

                String setAgentAppCmd = "cmd activity set-agent-app " + pkg + " " + agentInDataData;
                device.executeShellCommand(setAgentAppCmd);
            } catch (Exception e) {
                throw new RuntimeException("Failed running set-agent-app", e);
            }

            try {
                runAttachTestCmd(device, pkg, "");

                // And again.
                runAttachTestCmd(device, pkg, "");
            } catch (Exception e) {
                throw new RuntimeException("Failed agent-app attaching", e);
            }
        });
    }

    private void runJvmtiAgentLoadTest(TestRun runner) throws Exception {
        final ITestDevice device = getDevice();

        String testingArch = AbiUtils.getBaseArchForAbi(mAbi.getName());
        String deviceArch = getDeviceBaseArch(device);

        //Only bypass if Base Archs are different
        if (!testingArch.equals(deviceArch)) {
            CLog.d(
                    "Bypass as testing Base Arch:"
                            + testingArch
                            + " is different from DUT Base Arch:"
                            + deviceArch);
            return;
        }

        if (mTestApk == null || mTestPackageName == null) {
            throw new IllegalStateException("Incorrect configuration");
        }

        runner.run(device, mTestPackageName, mTestApk, mAbi.getName());
    }

    private String getDeviceBaseArch(ITestDevice device) throws Exception {
        String abi = device.executeShellCommand("getprop ro.product.cpu.abi").replace("\n", "");
        CLog.d("DUT abi:" + abi);
        return AbiUtils.getBaseArchForAbi(abi);
    }

    private static void runAttachTestCmd(ITestDevice device, String pkg, String agentParams)
            throws Exception {
        String attachCmd = "cmd activity start -S -W " + agentParams + " -n " + pkg
                + "/android.jvmti.JvmtiActivity";

        // Don't try to parse the output. The test will time out anyways if this didn't
        // work.
        device.executeShellCommand(attachCmd, NullOutputReceiver.getReceiver(), 10,
                TimeUnit.SECONDS, 1);
    }

    private String installLibToDataData(ITestDevice device, String pkg, String abiName,
            String apk, String dataData, String library, String newLibName) throws Exception {
        ZipFile zf = null;
        File tmpFile = null;
        String libInTmp = null;
        try {
            String libInDataData = dataData + "/" + newLibName;

            File apkFile = mBuildHelper.getTestFile(apk);
            zf = new ZipFile(apkFile);

            String libPathInApk = "lib/" + abiName + "/" + library;
            tmpFile = ZipUtil.extractFileFromZip(zf, libPathInApk);

            libInTmp = "/data/local/tmp/" + tmpFile.getName();
            if (!device.pushFile(tmpFile, libInTmp)) {
                throw new RuntimeException("Could not push library " + library + " to device");
            }

            String runAsCp = device.executeShellCommand(
                    "run-as " + pkg + " cp " + libInTmp + " " + libInDataData);
            if (runAsCp != null && !runAsCp.trim().isEmpty()) {
                throw new RuntimeException(runAsCp.trim());
            }

            String runAsChmod = device
                    .executeShellCommand("run-as " + pkg + " chmod a+x " + libInDataData);
            if (runAsChmod != null && !runAsChmod.trim().isEmpty()) {
                throw new RuntimeException(runAsChmod.trim());
            }

            return libInDataData;
        } finally {
            FileUtil.deleteFile(tmpFile);
            ZipUtil.closeZip(zf);
            if (libInTmp != null) {
                try {
                    device.executeShellCommand("rm " + libInTmp);
                } catch (Exception e) {
                    CLog.e("Failed cleaning up library on device");
                }
            }
        }
    }
}
