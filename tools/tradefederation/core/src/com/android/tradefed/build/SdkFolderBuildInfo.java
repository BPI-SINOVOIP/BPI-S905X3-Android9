/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.tradefed.build;

import com.android.tradefed.log.LogUtil.CLog;

import java.io.File;

/**
 *  A {@link IBuildInfo} that represents an extracted Android SDK and tests and additional build
 *  artifacts that are contained in a local file system directory.
 */
public class SdkFolderBuildInfo extends BuildInfo implements ISdkBuildInfo, IFolderBuildInfo {

    private static final long serialVersionUID = BuildSerializedVersion.VERSION;
    private ISdkBuildInfo mSdkBuild;
    private IFolderBuildInfo mFolderBuild;

    public SdkFolderBuildInfo(String buildId, String buildName) {
        super(buildId, buildName);
    }

    public SdkFolderBuildInfo() {
    }

    @Override
    public void cleanUp() {
        if (mSdkBuild != null) {
            mSdkBuild.cleanUp();
        }
        if (mFolderBuild != null) {
            mFolderBuild.cleanUp();
        }
    }

    @Override
    public File getRootDir() {
        if (mFolderBuild != null) {
            return mFolderBuild.getRootDir();
        }
        return null;
    }

    @Override
    public void setRootDir(File rootDir) {
        if (mFolderBuild != null) {
            mFolderBuild.setRootDir(rootDir);
        }
    }

    @Override
    public File getSdkDir() {
        if (mSdkBuild != null) {
            return mSdkBuild.getSdkDir();
        }
        return null;
    }

    @Override
    public File getTestsDir() {
        if (mSdkBuild != null) {
            return mSdkBuild.getTestsDir();
        }
        return null;
    }

    @Override
    public void setTestsDir(File testsDir) {
        if (mSdkBuild != null) {
            mSdkBuild.setTestsDir(testsDir);
        }
    }

    @Override
    public void setSdkDir(File sdkDir) {
        if (mSdkBuild != null) {
            mSdkBuild.setSdkDir(sdkDir);
        }
    }

    @Override
    public void setSdkDir(File sdkDir, boolean deleteParent) {
        if (mSdkBuild != null) {
            mSdkBuild.setSdkDir(sdkDir, deleteParent);
        }
    }

    @Override
    public String getAndroidToolPath() {
        if (mSdkBuild != null) {
            return mSdkBuild.getAndroidToolPath();
        }
        return null;
    }

    @Override
    public String getEmulatorToolPath() {
        if (mSdkBuild != null) {
            return mSdkBuild.getEmulatorToolPath();
        }
        return null;
    }

    @Override
    public String[] getSdkTargets() {
        if (mSdkBuild != null) {
            return mSdkBuild.getSdkTargets();
        }
        return null;
    }

    @Override
    public void makeToolsExecutable() {
        if (mSdkBuild != null) {
            mSdkBuild.makeToolsExecutable();
        }
    }

    /**
    * @param folderBuild
    */
   public void setFolderBuild(IFolderBuildInfo folderBuild) {
       mFolderBuild = folderBuild;
   }

   /**
    * @param sdkBuild
    */
   public void setSdkBuild(ISdkBuildInfo sdkBuild) {
       mSdkBuild = sdkBuild;
   }

    @Override
    public IBuildInfo clone() {
        if (mSdkBuild == null || mFolderBuild == null) {
            CLog.w("Invalid SdkFolderBuildInfo to clone.");
            return null;
        }
        SdkFolderBuildInfo copy = new SdkFolderBuildInfo(getBuildId(), getBuildTargetName());
        copy.addAllBuildAttributes(this);
        ISdkBuildInfo sdkBuildClone = (ISdkBuildInfo)mSdkBuild.clone();
        sdkBuildClone.makeToolsExecutable();
        copy.setSdkBuild(sdkBuildClone);
        IFolderBuildInfo folderBuildClone = (IFolderBuildInfo)mFolderBuild.clone();
        copy.setFolderBuild(folderBuildClone);
        return copy;
    }

}
