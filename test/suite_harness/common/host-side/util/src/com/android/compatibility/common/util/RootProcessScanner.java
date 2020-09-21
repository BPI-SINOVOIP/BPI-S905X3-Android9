/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.compatibility.common.util;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.HashSet;
import java.util.InputMismatchException;
import java.util.Scanner;
import java.util.Set;
import java.util.regex.Pattern;

/** Crawls /proc to find processes that are running as root. */
public class RootProcessScanner {

    private Set<String> mPidDirs;
    private ITestDevice mDevice;

    public RootProcessScanner(ITestDevice device) throws DeviceNotAvailableException {
        mDevice = device;
        mPidDirs = new HashSet<>();
        String lsOutput = device.executeShellCommand("ls -F /proc | grep /$"); // directories only
        String[] lines = lsOutput.split("/?\r?\n"); // split by line, shave "/" suffix if present
        for (String line : lines) {
            if (Pattern.matches("\\d+", line)) {
                mPidDirs.add(String.format("/proc/%s", line));
            }
        }
    }

    /** Processes that are allowed to run as root. */
    private static final Pattern ROOT_PROCESS_WHITELIST_PATTERN = getRootProcessWhitelistPattern(
            "debuggerd",
            "debuggerd64",
            "healthd",
            "init",
            "installd",
            "lmkd",
            "netd",
            "servicemanager",
            "ueventd",
            "vold",
            "watchdogd",
            "zygote"
    );

    /** Combine the individual patterns into one super pattern. */
    private static Pattern getRootProcessWhitelistPattern(String... patterns) {
        StringBuilder rootProcessPattern = new StringBuilder();
        for (int i = 0; i < patterns.length; i++) {
            rootProcessPattern.append(patterns[i]);
            if (i + 1 < patterns.length) {
                rootProcessPattern.append('|');
            }
        }
        return Pattern.compile(rootProcessPattern.toString());
    }

    /**
     * Get the names of approved or unapproved root processes running on the system.
     * @param approved whether to retrieve approved (true) or unapproved (false) processes
     * @return names of approved or unapproved root processes running on the system
     */
    public Set<String> getRootProcesses(boolean approved)
            throws DeviceNotAvailableException, MalformedStatMException {
        Set<String> rootProcessDirs = getRootProcessDirs(approved);
        Set<String> rootProcessNames = new HashSet<>();
        for (String dir : rootProcessDirs) {
            rootProcessNames.add(getProcessName(dir));
        }
        return rootProcessNames;
    }

    private Set<String> getRootProcessDirs(boolean approved)
            throws DeviceNotAvailableException, MalformedStatMException {
        Set<String> rootProcesses = new HashSet<>();
        if (mPidDirs != null && mPidDirs.size() > 0) {
            for (String processDir : mPidDirs) {
                if (isRootProcessDir(processDir, approved)) {
                    rootProcesses.add(processDir);
                }
            }
        } else {
            CLog.e("RootProcessScanner Failed to collect PID directories.");
        }
        return rootProcesses;
    }

    /**
     * Returns processes in /proc that are running as root with a certain approval status.
     * @throws FileNotFoundException
     * @throws MalformedStatMException
     */
    private boolean isRootProcessDir(String pathname, boolean approved)
            throws DeviceNotAvailableException, MalformedStatMException {
        try {
            return !isKernelProcess(pathname)
                    && isRootProcess(pathname)
                    && (isApproved(pathname) == approved);
        } catch (InputMismatchException e) {
            CLog.d("Path %s determined not to be a root process directory", pathname);
            return false;
        }
    }

    private boolean isKernelProcess(String processDir)
            throws DeviceNotAvailableException, MalformedStatMException {
        String statm = getProcessStatM(processDir);
        try (Scanner scanner = new Scanner(statm)) {
            boolean allZero = true;
            for (int i = 0; i < 7; i++) {
                if (scanner.nextInt() != 0) {
                    allZero = false;
                }
            }

            if (scanner.hasNext()) {
                throw new MalformedStatMException(processDir
                        + " statm expected to have 7 integers (man 5 proc)");
            }

            return allZero;
        }
    }

    private String getProcessStatM(String processDir) throws DeviceNotAvailableException {
        return mDevice.executeShellCommand(String.format("cat %s/statm", processDir));
    }

    public static class MalformedStatMException extends Exception {
        MalformedStatMException(String detailMessage) {
            super(detailMessage);
        }
    }

    /**
     * Return whether or not this process is running as root.
     *
     * @param processDir with the status file
     * @return whether or not it is a root process
     */
    private boolean isRootProcess(String processDir) throws DeviceNotAvailableException {
        String status = getProcessStatus(processDir);
        try (Scanner scanner = new Scanner(status)) {
            findToken(scanner, "Uid:");
            boolean rootUid = hasRootId(scanner);
            findToken(scanner, "Gid:");
            boolean rootGid = hasRootId(scanner);
            return rootUid || rootGid;
        }
    }

    /**
     * Return whether or not this process is approved to run as root.
     *
     * @param processDir with the status file
     * @return whether or not it is a root-whitelisted process
     * @throws FileNotFoundException
     */
    private boolean isApproved(String processDir) throws DeviceNotAvailableException {
        String status = getProcessStatus(processDir);
        try (Scanner scanner = new Scanner(status)) {
            findToken(scanner, "Name:");
            String name = scanner.next();
            return ROOT_PROCESS_WHITELIST_PATTERN.matcher(name).matches();
        }
    }

    /**
     * Get the status File path that has name:value pairs.
     * <pre>
     * Name:   init
     * ...
     * Uid:    0       0       0       0
     * Gid:    0       0       0       0
     * </pre>
     */
    private String getProcessStatus(String processDir) throws DeviceNotAvailableException {
        return mDevice.executeShellCommand(String.format("cat %s/status", processDir));
    }

    /**
     * Convenience method to move the scanner's position to the point after the given token.
     *
     * @param scanner to call next() until the token is found
     * @param token to find like "Name:"
     */
    private static void findToken(Scanner scanner, String token) {
        while (true) {
            String next = scanner.next();
            if (next.equals(token)) {
                return;
            }
        }

        // Scanner will exhaust input and throw an exception before getting here.
    }

    /**
     * Uid and Gid lines have four values: "Uid:    0       0       0       0"
     *
     * @param scanner that has just processed the "Uid:" or "Gid:" token
     * @return whether or not any of the ids are root
     */
    private static boolean hasRootId(Scanner scanner) {
        int realUid = scanner.nextInt();
        int effectiveUid = scanner.nextInt();
        int savedSetUid = scanner.nextInt();
        int fileSystemUid = scanner.nextInt();
        return realUid == 0 || effectiveUid == 0 || savedSetUid == 0 || fileSystemUid == 0;
    }

    /** Returns the name of the process corresponding to its process directory in /proc. */
    private String getProcessName(String processDir) throws DeviceNotAvailableException {
        String status = getProcessStatus(processDir);
        try (Scanner scanner = new Scanner(status)) {
            findToken(scanner, "Name:");
            return scanner.next();
        }
    }
}
