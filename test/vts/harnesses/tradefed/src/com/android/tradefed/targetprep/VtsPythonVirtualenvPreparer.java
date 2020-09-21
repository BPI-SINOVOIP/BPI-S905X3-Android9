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

package com.android.tradefed.targetprep;

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.command.remote.DeviceDescriptor;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.VtsVendorConfigFileUtil;

import java.io.File;
import java.io.IOException;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.Arrays;
import java.util.Collection;
import java.util.NoSuchElementException;
import java.util.TreeSet;

/**
 * Sets up a Python virtualenv on the host and installs packages. To activate it, the working
 * directory is changed to the root of the virtualenv.
 *
 * This's a fork of PythonVirtualenvPreparer and is forked in order to simplify the change
 * deployment process and reduce the deployment time, which are critical for VTS services.
 * That means changes here will be upstreamed gradually.
 */
@OptionClass(alias = "python-venv")
public class VtsPythonVirtualenvPreparer implements IMultiTargetPreparer {
    private static final String PIP = "pip";
    private static final String PATH = "PATH";
    private static final String OS_NAME = "os.name";
    private static final String WINDOWS = "Windows";
    private static final String LOCAL_PYPI_PATH_ENV_VAR_NAME = "VTS_PYPI_PATH";
    private static final String LOCAL_PYPI_PATH_KEY = "pypi_packages_path";
    protected static final String PYTHONPATH = "PYTHONPATH";
    protected static final String VIRTUAL_ENV_PATH = "VIRTUALENVPATH";
    private static final int BASE_TIMEOUT = 1000 * 60;

    @Option(name = "venv-dir", description = "path of an existing virtualenv to use")
    private File mVenvDir = null;

    @Option(name = "requirements-file", description = "pip-formatted requirements file")
    private File mRequirementsFile = null;

    @Option(name = "script-file", description = "scripts which need to be executed in advance")
    private Collection<String> mScriptFiles = new TreeSet<>();

    @Option(name = "dep-module", description = "modules which need to be installed by pip")
    protected Collection<String> mDepModules = new TreeSet<>();

    @Option(name = "no-dep-module", description = "modules which should not be installed by pip")
    private Collection<String> mNoDepModules = new TreeSet<>(Arrays.asList());

    @Option(name = "python-version", description = "The version of a Python interpreter to use.")
    private String mPythonVersion = "";

    private IBuildInfo mBuildInfo = null;
    private DeviceDescriptor mDescriptor = null;
    private IRunUtil mRunUtil = new RunUtil();

    String mPip = PIP;
    String mLocalPypiPath = null;

    // Since we allow virtual env path to be reused during a test plan/module, only the preparer
    // which created the directory should be the one to delete it.
    private boolean mIsDirCreator = false;

    // If the same object is used in multiple threads (in sharding mode), the class
    // needs to know when it is safe to call the teardown method.
    private int mNumOfInstances = 0;

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void setUp(IInvocationContext context)
            throws TargetSetupError, BuildError, DeviceNotAvailableException {
        ++mNumOfInstances;
        mBuildInfo = context.getBuildInfos().get(0);
        if (mNumOfInstances == 1) {
            ITestDevice device = context.getDevices().get(0);
            mDescriptor = device.getDeviceDescriptor();
            createVirtualenv(mBuildInfo);
            setLocalPypiPath();
            installDeps();
        }
        addPathToBuild(mBuildInfo);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void tearDown(IInvocationContext context, Throwable e)
            throws DeviceNotAvailableException {
        --mNumOfInstances;
        if (mNumOfInstances > 0) {
            // Since this is a host side preparer, no need to repeat
            return;
        }
        if (mVenvDir != null && mIsDirCreator) {
            try {
                recursiveDelete(mVenvDir.toPath());
                CLog.i("Deleted the virtual env's temp working dir, %s.", mVenvDir);
            } catch (IOException exception) {
                CLog.e("Failed to delete %s: %s", mVenvDir, exception);
            }
            mVenvDir = null;
        }
    }

    /**
     * This method sets mLocalPypiPath, the local PyPI package directory to
     * install python packages from in the installDeps method.
     */
    protected void setLocalPypiPath() {
        VtsVendorConfigFileUtil configReader = new VtsVendorConfigFileUtil();
        if (configReader.LoadVendorConfig(mBuildInfo)) {
            // First try to load local PyPI directory path from vendor config file
            try {
                String pypiPath = configReader.GetVendorConfigVariable(LOCAL_PYPI_PATH_KEY);
                if (pypiPath.length() > 0 && dirExistsAndHaveReadAccess(pypiPath)) {
                    mLocalPypiPath = pypiPath;
                    CLog.i(String.format("Loaded %s: %s", LOCAL_PYPI_PATH_KEY, mLocalPypiPath));
                }
            } catch (NoSuchElementException e) {
                /* continue */
            }
        }

        // If loading path from vendor config file is unsuccessful,
        // check local pypi path defined by LOCAL_PYPI_PATH_ENV_VAR_NAME
        if (mLocalPypiPath == null) {
            CLog.i("Checking whether local pypi packages directory exists");
            String pypiPath = System.getenv(LOCAL_PYPI_PATH_ENV_VAR_NAME);
            if (pypiPath == null) {
                CLog.i("Local pypi packages directory not specified by env var %s",
                        LOCAL_PYPI_PATH_ENV_VAR_NAME);
            } else if (dirExistsAndHaveReadAccess(pypiPath)) {
                mLocalPypiPath = pypiPath;
                CLog.i("Set local pypi packages directory to %s", pypiPath);
            }
        }

        if (mLocalPypiPath == null) {
            CLog.i("Failed to set local pypi packages path. Therefore internet connection to "
                    + "https://pypi.python.org/simple/ must be available to run VTS tests.");
        }
    }

    /**
     * This method returns whether the given path is a dir that exists and the user has read access.
     */
    private boolean dirExistsAndHaveReadAccess(String path) {
        File pathDir = new File(path);
        if (!pathDir.exists() || !pathDir.isDirectory()) {
            CLog.i("Directory %s does not exist.", pathDir);
            return false;
        }

        if (!isOnWindows()) {
            CommandResult c = getRunUtil().runTimedCmd(BASE_TIMEOUT * 5, "ls", path);
            if (c.getStatus() != CommandStatus.SUCCESS) {
                CLog.i(String.format("Failed to read dir: %s. Result %s. stdout: %s, stderr: %s",
                        path, c.getStatus(), c.getStdout(), c.getStderr()));
                return false;
            }
            return true;
        } else {
            try {
                String[] pathDirList = pathDir.list();
                if (pathDirList == null) {
                    CLog.i("Failed to read dir: %s. Please check access permission.", pathDir);
                    return false;
                }
            } catch (SecurityException e) {
                CLog.i(String.format(
                        "Failed to read dir %s with SecurityException %s", pathDir, e));
                return false;
            }
            return true;
        }
    }

    protected void installDeps() throws TargetSetupError {
        boolean hasDependencies = false;
        if (!mScriptFiles.isEmpty()) {
            for (String scriptFile : mScriptFiles) {
                CLog.i("Attempting to execute a script, %s", scriptFile);
                CommandResult c = getRunUtil().runTimedCmd(BASE_TIMEOUT * 5, scriptFile);
                if (c.getStatus() != CommandStatus.SUCCESS) {
                    CLog.e("Executing script %s failed", scriptFile);
                    throw new TargetSetupError("Failed to source a script", mDescriptor);
                }
            }
        }
        if (mRequirementsFile != null) {
            CommandResult c = getRunUtil().runTimedCmd(
                    BASE_TIMEOUT * 5, mPip, "install", "-r", mRequirementsFile.getAbsolutePath());
            if (!CommandStatus.SUCCESS.equals(c.getStatus())) {
                CLog.e("Installing dependencies from %s failed with error: %s",
                        mRequirementsFile.getAbsolutePath(), c.getStderr());
                throw new TargetSetupError("Failed to install dependencies with pip", mDescriptor);
            }
            hasDependencies = true;
        }
        if (!mDepModules.isEmpty()) {
            for (String dep : mDepModules) {
                if (mNoDepModules.contains(dep)) {
                    continue;
                }
                CommandResult result = null;
                if (mLocalPypiPath != null) {
                    CLog.i("Attempting installation of %s from local directory", dep);
                    result = getRunUtil().runTimedCmd(BASE_TIMEOUT * 5, mPip, "install", dep,
                            "--no-index", "--find-links=" + mLocalPypiPath);
                    CLog.i(String.format("Result %s. stdout: %s, stderr: %s", result.getStatus(),
                            result.getStdout(), result.getStderr()));
                    if (result.getStatus() != CommandStatus.SUCCESS) {
                        CLog.e(String.format("Installing %s from %s failed", dep, mLocalPypiPath));
                    }
                }
                if (mLocalPypiPath == null || result.getStatus() != CommandStatus.SUCCESS) {
                    CLog.i("Attempting installation of %s from PyPI", dep);
                    result = getRunUtil().runTimedCmd(BASE_TIMEOUT * 5, mPip, "install", dep);
                    CLog.i("Result %s. stdout: %s, stderr: %s", result.getStatus(),
                            result.getStdout(), result.getStderr());
                    if (result.getStatus() != CommandStatus.SUCCESS) {
                        CLog.e("Installing %s from PyPI failed.", dep);
                        CLog.i("Attempting to upgrade %s", dep);
                        result = getRunUtil().runTimedCmd(
                                BASE_TIMEOUT * 5, mPip, "install", "--upgrade", dep);
                        if (result.getStatus() != CommandStatus.SUCCESS) {
                            throw new TargetSetupError(
                                    String.format("Failed to install dependencies with pip. "
                                                    + "Result %s. stdout: %s, stderr: %s",
                                            result.getStatus(), result.getStdout(),
                                            result.getStderr()),
                                    mDescriptor);
                        } else {
                            CLog.i(String.format("Result %s. stdout: %s, stderr: %s",
                                    result.getStatus(), result.getStdout(), result.getStderr()));
                        }
                    }
                }
                hasDependencies = true;
            }
        }
        if (!hasDependencies) {
            CLog.i("No dependencies to install");
        }
    }

    /**
     * Add PYTHONPATH and VIRTUAL_ENV_PATH to BuildInfo.
     * @param buildInfo
     */
    protected void addPathToBuild(IBuildInfo buildInfo) {
        if (buildInfo.getFile(PYTHONPATH) == null) {
            // make the install directory of new packages available to other classes that
            // receive the build
            buildInfo.setFile(PYTHONPATH, new File(mVenvDir, "local/lib/python2.7/site-packages"),
                    buildInfo.getBuildId());
        }

        if (buildInfo.getFile(VIRTUAL_ENV_PATH) == null) {
            buildInfo.setFile(
                    VIRTUAL_ENV_PATH, new File(mVenvDir.getAbsolutePath()), buildInfo.getBuildId());
        }
    }

    /**
     * Create virtualenv directory by executing virtualenv command.
     * @param buildInfo
     * @throws TargetSetupError
     */
    protected void createVirtualenv(IBuildInfo buildInfo) throws TargetSetupError {
        if (mVenvDir == null) {
            mVenvDir = buildInfo.getFile(VIRTUAL_ENV_PATH);
        }

        if (mVenvDir == null) {
            CLog.i("Creating virtualenv");
            try {
                mVenvDir = FileUtil.createTempDir(getMD5(buildInfo.getTestTag()) + "-virtualenv");
                mIsDirCreator = true;
                String virtualEnvPath = mVenvDir.getAbsolutePath();
                CommandResult c;
                if (mPythonVersion.length() == 0) {
                    c = getRunUtil().runTimedCmd(BASE_TIMEOUT, "virtualenv", virtualEnvPath);
                } else {
                    String[] cmd = new String[] {
                            "virtualenv", "-p", "python" + mPythonVersion, virtualEnvPath};
                    c = getRunUtil().runTimedCmd(BASE_TIMEOUT, cmd);
                }
                if (c.getStatus() != CommandStatus.SUCCESS) {
                    CLog.e(String.format("Failed to create virtualenv with : %s.", virtualEnvPath));
                    throw new TargetSetupError("Failed to create virtualenv", mDescriptor);
                }
            } catch (IOException | RuntimeException e) {
                CLog.e("Failed to create temp directory for virtualenv");
                throw new TargetSetupError("Error creating virtualenv", e, mDescriptor);
            }
        }

        CLog.i("Python virtualenv path is: " + mVenvDir);
        activate();
    }

    /**
     * This method returns a MD5 hash string for the given string.
     */
    private String getMD5(String str) throws RuntimeException {
        try {
            java.security.MessageDigest md = java.security.MessageDigest.getInstance("MD5");
            byte[] array = md.digest(str.getBytes());
            StringBuffer sb = new StringBuffer();
            for (int i = 0; i < array.length; ++i) {
                sb.append(Integer.toHexString((array[i] & 0xFF) | 0x100).substring(1, 3));
            }
            return sb.toString();
        } catch (java.security.NoSuchAlgorithmException e) {
            throw new RuntimeException("Error generating MD5 hash.", e);
        }
    }

    protected void addDepModule(String module) {
        mDepModules.add(module);
    }

    protected void setRequirementsFile(File f) {
        mRequirementsFile = f;
    }

    /**
     * Get an instance of {@link IRunUtil}.
     */
    @VisibleForTesting
    IRunUtil getRunUtil() {
        if (mRunUtil == null) {
            mRunUtil = new RunUtil();
        }
        return mRunUtil;
    }

    /**
     * This method recursively deletes a file tree without following symbolic links.
     *
     * @param rootPath the path to delete.
     * @throws IOException if fails to traverse or delete the files.
     */
    private static void recursiveDelete(Path rootPath) throws IOException {
        Files.walkFileTree(rootPath, new SimpleFileVisitor<Path>() {
            @Override
            public FileVisitResult visitFile(Path file, BasicFileAttributes attrs)
                    throws IOException {
                Files.delete(file);
                return FileVisitResult.CONTINUE;
            }
            @Override
            public FileVisitResult postVisitDirectory(Path dir, IOException e) throws IOException {
                if (e != null) {
                    throw e;
                }
                Files.delete(dir);
                return FileVisitResult.CONTINUE;
            }
        });
    }

    /**
     * This method returns whether the OS is Windows.
     */
    private static boolean isOnWindows() {
        return System.getProperty(OS_NAME).contains(WINDOWS);
    }

    private void activate() {
        File binDir = new File(mVenvDir, isOnWindows() ? "Scripts" : "bin");
        getRunUtil().setWorkingDir(binDir);
        String path = System.getenv(PATH);
        getRunUtil().setEnvVariable(PATH, binDir + File.pathSeparator + path);
        File pipFile = new File(binDir, PIP);
        pipFile.setExecutable(true);
        mPip = pipFile.getAbsolutePath();
    }
}
