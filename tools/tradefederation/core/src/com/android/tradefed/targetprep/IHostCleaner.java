package com.android.tradefed.targetprep;

import com.android.tradefed.build.IBuildInfo;

/**
 * Cleans up the host after the test run has finished.
 */
public interface IHostCleaner {

    /**
     * Clean the host (remove the tmp files, etc).
     * @param buildInfo {@link IBuildInfo}.
     * @param e the {@link Throwable} which ended the test.
     */
    public void cleanUp(IBuildInfo buildInfo, Throwable e);
}

