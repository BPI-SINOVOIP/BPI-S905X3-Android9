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
package com.android.loganalysis.item;

import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link IItem} used to store Bugreport info.
 */
public class BugreportItem extends GenericItem {

    /** Constant for JSON output */
    public static final String TIME = "TIME";
    /** Constant for JSON output */
    public static final String COMMAND_LINE = "COMMAND_LINE";
    /** Constant for JSON output */
    public static final String MEM_INFO = "MEM_INFO";
    /** Constant for JSON output */
    public static final String PROCRANK = "PROCRANK";
    /** Constant for JSON output */
    public static final String TOP = "TOP";
    /** Constant for JSON output */
    public static final String KERNEL_LOG = "KERNEL_LOG";
    /** Constant for JSON output */
    public static final String LAST_KMSG = "LAST_KMSG";
    /** Constant for JSON output */
    public static final String SYSTEM_LOG = "SYSTEM_LOG";
    /** Constant for JSON output */
    public static final String SYSTEM_PROPS = "SYSTEM_PROPS";
    /** Constant for JSON output */
    public static final String DUMPSYS = "DUMPSYS";
    /** Constant for JSON output */
    public static final String ACTIVITY_SERVICE = "ACTIVITY_SERVICE";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            TIME, COMMAND_LINE, MEM_INFO, PROCRANK, TOP, KERNEL_LOG, LAST_KMSG, SYSTEM_LOG,
            SYSTEM_PROPS, DUMPSYS, ACTIVITY_SERVICE));

    public static class CommandLineItem extends GenericMapItem<String> {
        private static final long serialVersionUID = 0L;
    }

    /**
     * The constructor for {@link BugreportItem}.
     */
    public BugreportItem() {
        super(ATTRIBUTES);
    }

    /**
     * Get the time of the bugreport.
     */
    public Date getTime() {
        return (Date) getAttribute(TIME);
    }

    /**
     * Set the time of the bugreport.
     */
    public void setTime(Date time) {
        setAttribute(TIME, time);
    }

    /**
     * Get the {@link CommandLineItem} of the bugreport.
     */
    public CommandLineItem getCommandLine() {
        return (CommandLineItem) getAttribute(COMMAND_LINE);
    }

    /**
     * Set the {@link CommandLineItem} of the bugreport.
     */
    public void setCommandLine(CommandLineItem commandLine) {
        setAttribute(COMMAND_LINE, commandLine);
    }

    /**
     * Get the {@link MemInfoItem} of the bugreport.
     */
    public MemInfoItem getMemInfo() {
        return (MemInfoItem) getAttribute(MEM_INFO);
    }

    /**
     * Set the {@link MemInfoItem} of the bugreport.
     */
    public void setMemInfo(MemInfoItem memInfo) {
        setAttribute(MEM_INFO, memInfo);
    }

    /**
     * Get the {@link ProcrankItem} of the bugreport.
     */
    public ProcrankItem getProcrank() {
        return (ProcrankItem) getAttribute(PROCRANK);
    }

    /**
     * Set the {@link ProcrankItem} of the bugreport.
     */
    public void setProcrank(ProcrankItem procrank) {
        setAttribute(PROCRANK, procrank);
    }

    /**
     * Get the {@link TopItem} of the bugreport.
     */
    public TopItem getTop() {
        return (TopItem) getAttribute(TOP);
    }

    /**
     * Set the {@link TopItem} of the bugreport.
     */
    public void setTop(TopItem top) {
        setAttribute(TOP, top);
    }

    /**
     * Get the kernel log {@link KernelLogItem} of the bugreport.
     */
    public KernelLogItem getKernelLog() {
        return (KernelLogItem) getAttribute(KERNEL_LOG);
    }

    /**
     * Set the kernel log {@link KernelLogItem} of the bugreport.
     */
    public void setKernelLog(KernelLogItem systemLog) {
        setAttribute(KERNEL_LOG, systemLog);
    }

    /**
     * Get the last kmsg {@link KernelLogItem} of the bugreport.
     */
    public KernelLogItem getLastKmsg() {
        return (KernelLogItem) getAttribute(LAST_KMSG);
    }

    /**
     * Set the last kmsg {@link KernelLogItem} of the bugreport.
     */
    public void setLastKmsg(KernelLogItem systemLog) {
        setAttribute(LAST_KMSG, systemLog);
    }

    /**
     * Get the {@link LogcatItem} of the bugreport.
     */
    public LogcatItem getSystemLog() {
        return (LogcatItem) getAttribute(SYSTEM_LOG);
    }

    /**
     * Set the {@link LogcatItem} of the bugreport.
     */
    public void setSystemLog(LogcatItem systemLog) {
        setAttribute(SYSTEM_LOG, systemLog);
    }

    /**
     * Get the {@link SystemPropsItem} of the bugreport.
     */
    public SystemPropsItem getSystemProps() {
        return (SystemPropsItem) getAttribute(SYSTEM_PROPS);
    }

    /**
     * Set the {@link SystemPropsItem} of the bugreport.
     */
    public void setSystemProps(SystemPropsItem systemProps) {
        setAttribute(SYSTEM_PROPS, systemProps);
    }

    /**
     * Get the {@link DumpsysItem} of the bugreport.
     */
    public DumpsysItem getDumpsys() {
        return (DumpsysItem) getAttribute(DUMPSYS);
    }

    /**
     * Set the {@link DumpsysItem} of the bugreport.
     */
    public void setDumpsys(DumpsysItem dumpsys) {
        setAttribute(DUMPSYS, dumpsys);
    }
    /**
     * Get the {@link ActivityServiceItem} of the bugreport.
     */
    public ActivityServiceItem getActivityService() {
        return (ActivityServiceItem) getAttribute(ACTIVITY_SERVICE);
    }

    /**
     * Set the {@link ActivityServiceItem} of the bugreport.
     */
    public void setActivityService(ActivityServiceItem activityService) {
        setAttribute(ACTIVITY_SERVICE, activityService);
    }
}
