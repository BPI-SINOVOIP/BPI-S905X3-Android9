/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @author Vitaly A. Provodin
 */

package org.apache.harmony.jpda.tests.framework.jdwp;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.harmony.jpda.tests.framework.Breakpoint;
import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.framework.TestOptions;
import org.apache.harmony.jpda.tests.framework.jdwp.Capabilities;
import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Event;
import org.apache.harmony.jpda.tests.framework.jdwp.EventMod;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPConstants;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.TransportWrapper;
import org.apache.harmony.jpda.tests.framework.jdwp.TypesLengths;
import org.apache.harmony.jpda.tests.framework.jdwp.Frame.Variable;
import org.apache.harmony.jpda.tests.framework.jdwp.exceptions.ReplyErrorCodeException;
import org.apache.harmony.jpda.tests.framework.jdwp.exceptions.TimeoutException;

/**
 * This class provides convenient way for communicating with debuggee VM using
 * JDWP packets.
 * <p>
 * Most methods can throw ReplyErrorCodeException if error occurred in execution
 * of corresponding JDWP command or TestErrorException if any other error
 * occurred.
 */
public class VmMirror {

    /** Target VM Capabilities. */
    private Capabilities targetVMCapabilities;

    /** Transport used to sent and receive packets. */
    private TransportWrapper connection;

    /** PacketDispatcher thread used for asynchronous reading packets. */
    private PacketDispatcher packetDispatcher;

    /** Test run options. */
    protected TestOptions config;

    /** Log to write messages. */
    protected LogWriter logWriter;

    /**
     * Creates new VmMirror instance for given test run options.
     * 
     * @param config
     *            test run options
     * @param logWriter
     *            log writer
     */
    public VmMirror(TestOptions config, LogWriter logWriter) {
        connection = null;
        this.config = config;
        this.logWriter = logWriter;
    }

    /**
     * Checks error code of given reply packet and throws
     * ReplyErrorCodeException if any error detected.
     * 
     * @param reply
     *            reply packet to check
     * @return ReplyPacket unchanged reply packet
     */
    public ReplyPacket checkReply(ReplyPacket reply) {
        if (reply.getErrorCode() != JDWPConstants.Error.NONE)
            throw new ReplyErrorCodeException(reply.getErrorCode());
        return reply;
    }

    /**
     * Sets breakpoint to given location with suspend policy ALL.
     *
     * @param location
     *            Location of breakpoint
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setBreakpoint(Location location) {
        return setBreakpoint(location, JDWPConstants.SuspendPolicy.ALL);
    }

    /**
     * Sets breakpoint to given location
     * 
     * @param location
     *            Location of breakpoint
     * @param suspendPolicy
     *            Suspend policy for a breakpoint being created
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setBreakpoint(Location location, byte suspendPolicy) {
        Event event = Event.builder(JDWPConstants.EventKind.BREAKPOINT, suspendPolicy)
                .setLocationOnly(location)
                .build();
        return setEvent(event);
    }

    /**
     * Sets breakpoint that triggers only on a certain occurrence to a given
     * location
     * 
     * @param typeTag
     * @param breakpoint
     * @param suspendPolicy
     *            Suspend policy for a breakpoint being created
     * @param count
     *            Limit the requested event to be reported at most once after a
     *            given number of occurrences
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setCountableBreakpoint(byte typeTag,
            Breakpoint breakpoint, byte suspendPolicy, int count) {
        long typeID = getTypeID(breakpoint.className, typeTag);
        long methodID = getMethodID(typeID, breakpoint.methodName);

        Event event = Event.builder(JDWPConstants.EventKind.BREAKPOINT, suspendPolicy)
                .setLocationOnly(new Location(typeTag, typeID, methodID, breakpoint.index))
                .setCount(count)
                .build();
        return setEvent(event);
    }

    /**
     * Sets breakpoint at the beginning of method with name <i>methodName</i> with suspend policy
     * ALL.
     *
     * @param classID
     *            id of class with required method
     * @param methodName
     *            name of required method
     * @return requestID id of request
     */
    public int setBreakpointAtMethodBegin(long classID, String methodName) {
        return setBreakpointAtMethodBegin(classID, methodName, JDWPConstants.SuspendPolicy.ALL);
    }

    /**
     * Sets breakpoint at the beginning of method with name <i>methodName</i>.
     *
     * @param classID
     *            id of class with required method
     * @param methodName
     *            name of required method
     * @return requestID id of request
     */
    public int setBreakpointAtMethodBegin(long classID, String methodName, byte suspendPolicy) {
        long methodID = getMethodID(classID, methodName);

        ReplyPacket lineTableReply = getLineTable(classID, methodID);
        if (lineTableReply.getErrorCode() != JDWPConstants.Error.NONE) {
            throw new TestErrorException(
                    "Command getLineTable returned error code: "
                            + lineTableReply.getErrorCode()
                            + " - "
                            + JDWPConstants.Error.getName(lineTableReply
                                    .getErrorCode()));
        }

        lineTableReply.getNextValueAsLong();
        // Lowest valid code index for the method

        lineTableReply.getNextValueAsLong();
        // Highest valid code index for the method

        // int numberOfLines =
        lineTableReply.getNextValueAsInt();

        long lineCodeIndex = lineTableReply.getNextValueAsLong();

        // set breakpoint inside checked method
        Location breakpointLocation = new Location(JDWPConstants.TypeTag.CLASS,
                classID, methodID, lineCodeIndex);

        ReplyPacket reply = setBreakpoint(breakpointLocation, suspendPolicy);
        checkReply(reply);

        return reply.getNextValueAsInt();
    }

    /**
     * Waits for stop on breakpoint and gets id of thread where it stopped.
     * 
     * @param requestID
     *            id of request for breakpoint
     * @return threadID id of thread, where we stop on breakpoint
     */
    public long waitForBreakpoint(int requestID) {
        // receive event
        CommandPacket event = null;
        event = receiveEvent();

        event.getNextValueAsByte();
        // suspendPolicy - is not used here

        // int numberOfEvents =
        event.getNextValueAsInt();

        long breakpointThreadID = 0;
        ParsedEvent[] eventParsed = ParsedEvent.parseEventPacket(event);

        if (eventParsed.length != 1) {
            throw new TestErrorException("Received " + eventParsed.length
                    + " events instead of 1 BREAKPOINT_EVENT");
        }

        // check if received event is for breakpoint
        if (eventParsed[0].getEventKind() == JDWPConstants.EventKind.BREAKPOINT) {
            breakpointThreadID = ((ParsedEvent.Event_BREAKPOINT) eventParsed[0])
                    .getThreadID();

        } else {
            throw new TestErrorException(
                    "Kind of received event is not BREAKPOINT_EVENT: "
                            + eventParsed[0].getEventKind());

        }

        if (eventParsed[0].getRequestID() != requestID) {
            throw new TestErrorException(
                    "Received BREAKPOINT_EVENT with another requestID: "
                            + eventParsed[0].getRequestID());
        }

        return breakpointThreadID;
    }

    /**
     * Removes breakpoint according to specified requestID.
     * 
     * @param requestID
     *            for given breakpoint
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket clearBreakpoint(int requestID) {

        // Create new command packet
        CommandPacket commandPacket = new CommandPacket();

        // Set command. "2" - is ID of Clear command in EventRequest Command Set
        commandPacket
                .setCommand(JDWPCommands.EventRequestCommandSet.ClearCommand);

        // Set command set. "15" - is ID of EventRequest Command Set
        commandPacket
                .setCommandSet(JDWPCommands.EventRequestCommandSet.CommandSetID);

        // Set outgoing data
        // Set eventKind
        commandPacket.setNextValueAsByte(JDWPConstants.EventKind.BREAKPOINT);

        // Set suspendPolicy
        commandPacket.setNextValueAsInt(requestID);

        // Send packet
        return checkReply(performCommand(commandPacket));
    }

    /**
     * Removes all breakpoints.
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket ClearAllBreakpoints() {

        // Create new command packet
        CommandPacket commandPacket = new CommandPacket();

        // Set command. "3" - is ID of ClearAllBreakpoints command in
        // EventRequest Command Set
        commandPacket
                .setCommand(JDWPCommands.EventRequestCommandSet.ClearAllBreakpointsCommand);

        // Set command set. "15" - is ID of EventRequest Command Set
        commandPacket
                .setCommandSet(JDWPCommands.EventRequestCommandSet.CommandSetID);

        // Send packet
        return checkReply(performCommand(commandPacket));
    }

    /**
     * Requests debuggee VM capabilities. Function parses reply packet of
     * VirtualMachine::CapabilitiesNew command, creates and fills class
     * Capabilities with returned info.
     * 
     * @return ReplyPacket useless, already parsed reply packet.
     */
    public ReplyPacket capabilities() {

        // Create new command packet
        CommandPacket commandPacket = new CommandPacket();

        // Set command. "17" - is ID of CapabilitiesNew command in
        // VirtualMachine Command Set
        commandPacket
                .setCommand(JDWPCommands.VirtualMachineCommandSet.CapabilitiesNewCommand);

        // Set command set. "1" - is ID of VirtualMachine Command Set
        commandPacket
                .setCommandSet(JDWPCommands.VirtualMachineCommandSet.CommandSetID);

        // Send packet
        ReplyPacket replyPacket = checkReply(performCommand(commandPacket));

        targetVMCapabilities = new Capabilities();

        // Set capabilities
        targetVMCapabilities.canWatchFieldModification = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canWatchFieldAccess = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canGetBytecodes = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canGetSyntheticAttribute = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canGetOwnedMonitorInfo = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canGetCurrentContendedMonitor = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canGetMonitorInfo = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canRedefineClasses = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canAddMethod = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.canUnrestrictedlyRedefineClasses = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canPopFrames = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.canUseInstanceFilters = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canGetSourceDebugExtension = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canRequestVMDeathEvent = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canSetDefaultStratum = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canGetInstanceInfo = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.reserved17 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.canGetMonitorFrameInfo = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canUseSourceNameFilters = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.canGetConstantPool = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.canForceEarlyReturn = replyPacket
                .getNextValueAsBoolean();
        targetVMCapabilities.reserved22 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved23 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved24 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved25 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved26 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved27 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved28 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved29 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved30 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved31 = replyPacket.getNextValueAsBoolean();
        targetVMCapabilities.reserved32 = replyPacket.getNextValueAsBoolean();

        return replyPacket;
    }

    /**
     * Indicates whether the capability <i>canRedefineClasses</i> is supported.
     *
     * @return true if supported, false otherwise.
     */
    public boolean canRedefineClasses() {
        capabilities();
        return targetVMCapabilities.canRedefineClasses;
    }

    /**
     * Indicates whether the capability <i>canPopFrames</i> is supported.
     *
     * @return true if supported, false otherwise.
     */
    public boolean canPopFrames() {
        capabilities();
        return targetVMCapabilities.canPopFrames;
    }

    /**
     * Indicates whether the capability <i>canGetSourceDebugExtension</i> is supported.
     *
     * @return true if supported, false otherwise.
     */
    public boolean canGetSourceDebugExtension() {
        capabilities();
        return targetVMCapabilities.canGetSourceDebugExtension;
    }

    /**
     * Indicates whether the capability <i>canRequestVMDeathEvent</i> is supported.
     *
     * @return true if supported, false otherwise.
     */
    public boolean canRequestVMDeathEvent() {
        capabilities();
        return targetVMCapabilities.canRequestVMDeathEvent;
    }

    /**
     * Indicates whether the capability <i>canSetDefaultStratum</i> is supported.
     *
     * @return true if supported, false otherwise.
     */
    public boolean canSetDefaultStratum() {
        capabilities();
        return targetVMCapabilities.canSetDefaultStratum;
    }

    /**
     * Indicates whether the capability <i>canUseSourceNameFilters</i> is supported.
     *
     * @return true if supported, false otherwise.
     */
    public boolean canUseSourceNameFilters() {
        capabilities();
        return targetVMCapabilities.canUseSourceNameFilters;
    }

    /**
     * Indicates whether the capability <i>canGetConstantPool</i> is supported.
     *
     * @return true if supported, false otherwise.
     */
    public boolean canGetConstantPool() {
        capabilities();
        return targetVMCapabilities.canGetConstantPool;
    }

    /**
     * Indicates whether the capability <i>canForceEarlyReturn</i> is supported.
     *
     * @return true if supported, false otherwise.
     */
    public boolean canForceEarlyReturn() {
        capabilities();
        return targetVMCapabilities.canForceEarlyReturn;
    }

    /**
     * Indicates whether the non-standard capability <i>canRedefineDexClasses</i> is supported.
     *
     * @return true if supported, false otherwise.
     */
    public boolean canRedefineDexClasses() {
        capabilities();
        // This unused capability is used by android libjdwp agents to say if they can redefine
        // classes using (single class) DEX files.
        return targetVMCapabilities.reserved32;
    }

    /**
     * Resumes debuggee VM.
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket resume() {
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.VirtualMachineCommandSet.CommandSetID,
                JDWPCommands.VirtualMachineCommandSet.ResumeCommand);

        return checkReply(performCommand(commandPacket));
    }

    /**
     * Resumes specified thread on target Virtual Machine
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket resumeThread(long threadID) {
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.ResumeCommand);

        commandPacket.setNextValueAsThreadID(threadID);
        return checkReply(performCommand(commandPacket));
    }

    /**
     * Suspends debuggee VM.
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket suspend() {
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.VirtualMachineCommandSet.CommandSetID,
                JDWPCommands.VirtualMachineCommandSet.SuspendCommand);

        return checkReply(performCommand(commandPacket));
    }

    /**
     * Suspends specified thread in debuggee VM.
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket suspendThread(long threadID) {
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.SuspendCommand);

        commandPacket.setNextValueAsThreadID(threadID);
        return checkReply(performCommand(commandPacket));
    }

    /**
     * Disposes connection to debuggee VM.
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket dispose() {
        // Create new command packet
        CommandPacket commandPacket = new CommandPacket();
        commandPacket
                .setCommand(JDWPCommands.VirtualMachineCommandSet.DisposeCommand);
        commandPacket
                .setCommandSet(JDWPCommands.VirtualMachineCommandSet.CommandSetID);

        // Send packet
        return checkReply(performCommand(commandPacket));
    }

    /**
     * Exits debuggee VM process.
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket exit(int exitCode) {
        // Create new command packet
        CommandPacket commandPacket = new CommandPacket();
        commandPacket
                .setCommand(JDWPCommands.VirtualMachineCommandSet.ExitCommand);
        commandPacket
                .setCommandSet(JDWPCommands.VirtualMachineCommandSet.CommandSetID);
        commandPacket.setNextValueAsInt(exitCode);

        // Send packet
        return checkReply(performCommand(commandPacket));
    }

    /**
     * Adjusts lengths for all VM-specific types.
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket adjustTypeLength() {
        // Create new command packet
        CommandPacket commandPacket = new CommandPacket();
        commandPacket
                .setCommand(JDWPCommands.VirtualMachineCommandSet.IDSizesCommand);
        commandPacket
                .setCommandSet(JDWPCommands.VirtualMachineCommandSet.CommandSetID);

        // Send packet
        ReplyPacket replyPacket = checkReply(performCommand(commandPacket));

        // Get FieldIDSize from ReplyPacket
        TypesLengths.setTypeLength(TypesLengths.FIELD_ID, replyPacket
                .getNextValueAsInt());

        // Get MethodIDSize from ReplyPacket
        TypesLengths.setTypeLength(TypesLengths.METHOD_ID, replyPacket
                .getNextValueAsInt());

        // Get ObjectIDSize from ReplyPacket
        TypesLengths.setTypeLength(TypesLengths.OBJECT_ID, replyPacket
                .getNextValueAsInt());

        // Get ReferenceTypeIDSize from ReplyPacket
        TypesLengths.setTypeLength(TypesLengths.REFERENCE_TYPE_ID, replyPacket
                .getNextValueAsInt());

        // Get FrameIDSize from ReplyPacket
        TypesLengths.setTypeLength(TypesLengths.FRAME_ID, replyPacket
                .getNextValueAsInt());

        // Adjust all other types lengths
        TypesLengths.setTypeLength(TypesLengths.ARRAY_ID, TypesLengths
                .getTypeLength(TypesLengths.OBJECT_ID));
        TypesLengths.setTypeLength(TypesLengths.STRING_ID, TypesLengths
                .getTypeLength(TypesLengths.OBJECT_ID));
        TypesLengths.setTypeLength(TypesLengths.THREAD_ID, TypesLengths
                .getTypeLength(TypesLengths.OBJECT_ID));
        TypesLengths.setTypeLength(TypesLengths.THREADGROUP_ID, TypesLengths
                .getTypeLength(TypesLengths.OBJECT_ID));
        TypesLengths.setTypeLength(TypesLengths.LOCATION_ID, TypesLengths
                .getTypeLength(TypesLengths.OBJECT_ID));
        TypesLengths.setTypeLength(TypesLengths.CLASS_ID, TypesLengths
                .getTypeLength(TypesLengths.OBJECT_ID));
        TypesLengths.setTypeLength(TypesLengths.CLASSLOADER_ID, TypesLengths
                .getTypeLength(TypesLengths.OBJECT_ID));
        TypesLengths.setTypeLength(TypesLengths.CLASSOBJECT_ID, TypesLengths
                .getTypeLength(TypesLengths.OBJECT_ID));
        return replyPacket;
    }

    /**
     * Gets TypeID for specified type signature and type tag.
     * 
     * @param typeSignature
     *            type signature
     * @param classTypeTag
     *            type tag
     * @return received TypeID
     */
    public long getTypeID(String typeSignature, byte classTypeTag) {
        int classes = 0;
        byte refTypeTag = 0;
        long typeID = -1;

        // Request referenceTypeID for exception
        ReplyPacket classReference = getClassBySignature(typeSignature);

        // Get referenceTypeID from received packet
        classes = classReference.getNextValueAsInt();
        for (int i = 0; i < classes; i++) {
            refTypeTag = classReference.getNextValueAsByte();
            if (refTypeTag == classTypeTag) {
                typeID = classReference.getNextValueAsReferenceTypeID();
                classReference.getNextValueAsInt();
                break;
            } else {
                classReference.getNextValueAsReferenceTypeID();
                classReference.getNextValueAsInt();
                refTypeTag = 0;
            }
        }
        return typeID;
    }

    /**
     * Gets ClassID for specified class signature.
     * 
     * @param classSignature
     *            class signature
     * @return received ClassID
     */
    public long getClassID(String classSignature) {
        return getTypeID(classSignature, JDWPConstants.TypeTag.CLASS);
    }

    /**
     * Gets ThreadID for specified thread name.
     * 
     * @param threadName
     *            thread name
     * @return received ThreadID
     */
    public long getThreadID(String threadName) {
        ReplyPacket request = null;
        long threadID = -1;
        long thread = -1;
        String name = null;
        int threads = -1;

        // Get All Threads IDs
        request = getAllThreadID();

        // Get thread ID for threadName
        threads = request.getNextValueAsInt();
        for (int i = 0; i < threads; i++) {
            thread = request.getNextValueAsThreadID();
            name = getThreadName(thread);
            if (threadName.equals(name)) {
                threadID = thread;
                break;
            }
        }

        return threadID;
    }

    /**
     * Returns all running thread IDs.
     * 
     * @return received reply packet
     */
    public ReplyPacket getAllThreadID() {
        // Create new command packet
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.VirtualMachineCommandSet.CommandSetID,
                JDWPCommands.VirtualMachineCommandSet.AllThreadsCommand);

        return checkReply(performCommand(commandPacket));
    }

    /**
     * Gets class signature for specified class ID.
     * 
     * @param classID
     *            class ID
     * @return received class signature
     */
    public String getClassSignature(long classID) {
        // Create new command packet
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.SignatureCommand);
        commandPacket.setNextValueAsReferenceTypeID(classID);
        ReplyPacket replyPacket = checkReply(performCommand(commandPacket));
        return replyPacket.getNextValueAsString();
    }

    /**
     * Returns thread name for specified <code>threadID</code>.
     * 
     * @param threadID
     *            thread ID
     * @return thread name
     */
    public String getThreadName(long threadID) {
        // Create new command packet
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.NameCommand);
        commandPacket.setNextValueAsThreadID(threadID);
        ReplyPacket replyPacket = checkReply(performCommand(commandPacket));
        return replyPacket.getNextValueAsString();
    }

    /**
     * Returns thread status for specified <code>threadID</code>.
     * 
     * @param threadID
     *            thread ID
     * @return thread status
     */
    public int getThreadStatus(long threadID) {
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.StatusCommand);
        commandPacket.setNextValueAsThreadID(threadID);
        ReplyPacket replyPacket = checkReply(performCommand(commandPacket));
        return replyPacket.getNextValueAsInt();
    }

    /**
     * Returns suspend count for specified <code>threadID</code>.
     *
     * @param threadID
     *            thread ID
     * @return thread's suspend count
     */
    public int getThreadSuspendCount(long threadID) {
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.SuspendCountCommand);
        commandPacket.setNextValueAsThreadID(threadID);
        ReplyPacket replyPacket = checkReply(performCommand(commandPacket));
        return replyPacket.getNextValueAsInt();
    }

    /**
     * Returns name of thread group for specified <code>groupID</code>
     * 
     * @param groupID
     *            thread group ID
     * 
     * @return name of thread group
     */
    public String getThreadGroupName(long groupID) {
        // Create new command packet
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.ThreadGroupReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadGroupReferenceCommandSet.NameCommand);
        commandPacket.setNextValueAsReferenceTypeID(groupID);
        ReplyPacket replyPacket = checkReply(performCommand(commandPacket));
        return replyPacket.getNextValueAsString();
    }

    /**
     * Gets InterfaceID for specified interface signature.
     * 
     * @param interfaceSignature
     *            interface signature
     * @return received ClassID
     */
    public long getInterfaceID(String interfaceSignature) {
        return getTypeID(interfaceSignature, JDWPConstants.TypeTag.INTERFACE);
    }

    /**
     * Gets ArrayID for specified array signature.
     * 
     * @param arraySignature
     *            array signature
     * @return received ArrayID
     */
    public long getArrayID(String arraySignature) {
        return getTypeID(arraySignature, JDWPConstants.TypeTag.INTERFACE);
    }

    /**
     * Gets RequestID from specified ReplyPacket.
     * 
     * @param request
     *            ReplyPacket with RequestID
     * @return received RequestID
     */
    public int getRequestID(ReplyPacket request) {
        return request.getNextValueAsInt();
    }

    /**
     * Returns FieldID for specified class and field name.
     * 
     * @param classID
     *            ClassID to find field
     * @param fieldName
     *            field name
     * @return received FieldID
     */
    public long getFieldID(long classID, String fieldName) {
        Field[] fields = getFieldsInfo(classID);
        for (Field field : fields) {
            if (field.getName().equals(fieldName)) {
                return field.getFieldID();
            }
        }
        return -1;
    }

    /**
     * Gets Method ID for specified class and method name.
     * 
     * @param classID
     *            class to find method
     * @param methodName
     *            method name
     * @return received MethodID
     */
    public long getMethodID(long classID, String methodName) {
        // Only take method name into account.
        return getMethodID(classID, methodName, null);
    }

    /**
     * Gets Method ID for specified class, method name and signature.
     *
     * @param classID
     *            class to find method
     * @param methodName
     *            method name
     * @param methodSignature
     *            method signature
     * @return received MethodID
     */
    public long getMethodID(long classID, String methodName, String methodSignature) {
        Method[] methods = getMethods(classID);
        for (Method method : methods) {
            if (method.getName().equals(methodName)) {
                if (methodSignature == null || method.getSignature().equals(methodSignature)) {
                    return method.getMethodID();
                }
            }
        }
        return -1;
    }

    /**
     * Returns method name for specified pair of classID and methodID.
     * 
     * @param classID
     * @param methodID
     * @return method name
     */
    public String getMethodName(long classID, long methodID) {
        Method[] methods = getMethods(classID);
        for (Method method : methods) {
            if (methodID == method.getMethodID()) {
                return method.getName();
            }
        }
        return "unknown";
    }

    /**
     * Sets ClassPrepare event request for given class name pattern with suspend policy ALL.
     *
     * @param classRegexp
     *            Required class pattern. Matches are limited to exact matches
     *            of the given class pattern and matches of patterns that begin
     *            or end with '*'; for example, "*.Foo" or "java.*".
     * @return ReplyPacket for setting request.
     */
    public ReplyPacket setClassPrepared(String classRegexp) {
        return setClassMatchEvent(JDWPConstants.EventKind.CLASS_PREPARE,
                JDWPConstants.SuspendPolicy.ALL, classRegexp);
    }

    /**
     * Set ClassPrepare event request for given class ID with suspend policy ALL.
     *
     * @param referenceTypeID
     *            class referenceTypeID
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setClassPrepared(long referenceTypeID) {
        return setClassOnlyEvent(JDWPConstants.EventKind.CLASS_PREPARE,
                JDWPConstants.SuspendPolicy.ALL, referenceTypeID);
    }
    
    /**
     * Sets ClassPrepare event request for given source name pattern with suspend policy ALL.
     *
     * @param sourceNamePattern
     *            Required source name pattern. Matches are limited to exact matches
     *            of the given source name pattern and matches of patterns that begin
     *            or end with '*'; for example, "*.Foo" or "java.*".
     * @return ReplyPacket for setting request.
     */
    public ReplyPacket setClassPreparedForSourceNameMatch(String sourceNamePattern) {
        byte eventKind = JDWPConstants.EventKind.CLASS_PREPARE;
        byte suspendPolicy = JDWPConstants.SuspendPolicy.ALL;
        Event event = Event.builder(eventKind, suspendPolicy)
                .setSourceNameMatch(sourceNamePattern)
                .build();
        return setEvent(event);
    }

    /**
     * Sets ClassUnload event request for given class name pattern with suspend policy ALL.
     *
     * @param classRegexp
     *            class name pattern
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setClassUnload(String classRegexp) {
        return setClassMatchEvent(JDWPConstants.EventKind.CLASS_UNLOAD,
                JDWPConstants.SuspendPolicy.ALL, classRegexp);
    }

    /**
     * Set ClassUnload event request for given class ID with suspend policy ALL.
     *
     * @param referenceTypeID
     *            class referenceTypeID
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setClassUnload(long referenceTypeID) {
        return setClassOnlyEvent(JDWPConstants.EventKind.CLASS_UNLOAD,
                JDWPConstants.SuspendPolicy.ALL, referenceTypeID);
    }

    /**
     * Sets ClassLoad event request for given class signature with suspend policy ALL.
     *
     * @param classSignature
     *            class signature
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setClassLoad(String classSignature) {
        long typeID;

        // Request referenceTypeID for class
        typeID = getClassID(classSignature);

        // Set corresponding event
        return setClassLoad(typeID);
    }

    /**
     * Set ClassLoad event request for given class ID with suspend policy ALL.
     *
     * @param referenceTypeID
     *            class referenceTypeID
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setClassLoad(long referenceTypeID) {
        return setClassOnlyEvent(JDWPConstants.EventKind.CLASS_LOAD,
                JDWPConstants.SuspendPolicy.ALL, referenceTypeID);
    }
    
    /**
     * Set MonitorContendedEnter event request for given class's reference type
     * 
     * @param referenceTypeID
     *            class referenceTypeID
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setMonitorContendedEnterForClassOnly(long referenceTypeID) {
        return setClassOnlyEvent(JDWPConstants.EventKind.MONITOR_CONTENDED_ENTER,
                JDWPConstants.SuspendPolicy.ALL, referenceTypeID);
    }

    /**
     * Set MonitorContendedEnter event request for given class's name pattern
     *
     * @param classRegexp
     *            class name pattern
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setMonitorContendedEnterForClassMatch(String classRegexp) {
        return setClassMatchEvent(JDWPConstants.EventKind.MONITOR_CONTENDED_ENTER,
                JDWPConstants.SuspendPolicy.ALL, classRegexp);
    }

    /**
     * Set MonitorContendedEntered event request for given class's reference type
     * 
     * @param referenceTypeID
     *            class referenceTypeID
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setMonitorContendedEnteredForClassOnly(long referenceTypeID) {
        return setClassOnlyEvent(JDWPConstants.EventKind.MONITOR_CONTENDED_ENTERED,
                JDWPConstants.SuspendPolicy.ALL, referenceTypeID);
    }

    /**
     * Set MonitorContendedEntered event request for given class's name pattern
     *
     * @param classRegexp
     *            class name pattern
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setMonitorContendedEnteredForClassMatch(String classRegexp) {
        return setClassMatchEvent(JDWPConstants.EventKind.MONITOR_CONTENDED_ENTERED,
                JDWPConstants.SuspendPolicy.ALL, classRegexp);
    }
    
    /**
     * Set MonitorWait event request for given class's reference type
     * 
     * @param referenceTypeID
     *            class referenceTypeID
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setMonitorWaitForClassOnly(long referenceTypeID) {
        return setClassOnlyEvent(JDWPConstants.EventKind.MONITOR_WAIT,
                JDWPConstants.SuspendPolicy.ALL, referenceTypeID);
    }

    /**
     * Set MonitorWait event request for given given class name pattern.
     * 
     * @param classRegexp
     *            Required class pattern. Matches are limited to exact matches
     *            of the given class pattern and matches of patterns that begin
     *            or end with '*'; for example, "*.Foo" or "java.*".
     * @return ReplyPacket for setting request.
     */
    public ReplyPacket setMonitorWaitForClassMatch(String classRegexp) {
        return setClassMatchEvent(JDWPConstants.EventKind.MONITOR_WAIT,
                JDWPConstants.SuspendPolicy.ALL, classRegexp);
    }
    
    /**
     * Set MonitorWait event request for classes 
     * whose name does not match the given restricted regular expression.
     * 
     * @param classRegexp
     *            Exclude class pattern. Matches are limited to exact matches
     *            of the given class pattern and matches of patterns that begin
     *            or end with '*'; for example, "*.Foo" or "java.*".
     * @return ReplyPacket for setting request.
     */
    public ReplyPacket setMonitorWaitForClassExclude (String classRegexp) {
        return setClassExcludeEvent(JDWPConstants.EventKind.MONITOR_WAIT,
                JDWPConstants.SuspendPolicy.ALL, classRegexp);
    }
    
    /**
     * Set MonitorWaited event request for given class's reference type
     * 
     * @param referenceTypeID
     *            class referenceTypeID
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setMonitorWaitedForClassOnly(long referenceTypeID) {
        return setClassOnlyEvent(JDWPConstants.EventKind.MONITOR_WAITED,
                JDWPConstants.SuspendPolicy.ALL, referenceTypeID);
    }
    
    /**
     * Set MonitorWaited event request for given given source name pattern.
     * 
     * @param classRegexp
     *            Required class pattern. Matches are limited to exact matches
     *            of the given class pattern and matches of patterns that begin
     *            or end with '*'; for example, "*.Foo" or "java.*".
     * @return ReplyPacket for setting request.
     */
    public ReplyPacket setMonitorWaitedForClassMatch(String classRegexp) {
        return setClassMatchEvent(JDWPConstants.EventKind.MONITOR_WAITED,
                JDWPConstants.SuspendPolicy.ALL, classRegexp);
    }
    
    /**
     * Set MonitorWaited event request for classes 
     * whose name does not match the given restricted regular expression.
     * 
     * @param classRegexp
     *            Required class pattern. Matches are limited to exact matches
     *            of the given class pattern and matches of patterns that begin
     *            or end with '*'; for example, "*.Foo" or "java.*".
     * @return ReplyPacket for setting request.
     */
    public ReplyPacket setMonitorWaitedForClassExclude (String classRegexp) {
        return setClassExcludeEvent(JDWPConstants.EventKind.MONITOR_WAITED,
                JDWPConstants.SuspendPolicy.ALL, classRegexp);
    }

    /**
     * Set event request for given event.
     * 
     * @param event
     *            event to set request for
     * @return ReplyPacket for setting request
     */
    public ReplyPacket setEvent(Event event) {
        // Create new command packet
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.EventRequestCommandSet.CommandSetID,
                JDWPCommands.EventRequestCommandSet.SetCommand);

        // Set eventKind
        commandPacket.setNextValueAsByte(event.eventKind);
        // Set suspendPolicy
        commandPacket.setNextValueAsByte(event.suspendPolicy);

        // Set modifiers
        commandPacket.setNextValueAsInt(event.mods.size());

        for (EventMod eventModifier : event.mods) {

            commandPacket.setNextValueAsByte(eventModifier.modKind);

            switch (eventModifier.modKind) {
                case EventMod.ModKind.Count: {
                    // Case Count
                    commandPacket.setNextValueAsInt(eventModifier.count);
                    break;
                }
                case EventMod.ModKind.Conditional: {
                    // Case Conditional
                    commandPacket.setNextValueAsInt(eventModifier.exprID);
                    break;
                }
                case EventMod.ModKind.ThreadOnly: {
                    // Case ThreadOnly
                    commandPacket.setNextValueAsThreadID(eventModifier.thread);
                    break;
                }
                case EventMod.ModKind.ClassOnly: {
                    // Case ClassOnly
                    commandPacket
                    .setNextValueAsReferenceTypeID(eventModifier.clazz);
                    break;
                }
                case EventMod.ModKind.ClassMatch: {
                    // Case ClassMatch
                    commandPacket.setNextValueAsString(eventModifier.classPattern);
                    break;
                }
                case EventMod.ModKind.ClassExclude: {
                    // Case ClassExclude
                    commandPacket.setNextValueAsString(eventModifier.classPattern);
                    break;
                }
                case EventMod.ModKind.LocationOnly: {
                    // Case LocationOnly
                    commandPacket.setNextValueAsLocation(eventModifier.loc);
                    break;
                }
                case EventMod.ModKind.ExceptionOnly:
                    // Case ExceptionOnly
                    commandPacket
                    .setNextValueAsReferenceTypeID(eventModifier.exceptionOrNull);
                    commandPacket.setNextValueAsBoolean(eventModifier.caught);
                    commandPacket.setNextValueAsBoolean(eventModifier.uncaught);
                    break;
                case EventMod.ModKind.FieldOnly: {
                    // Case FieldOnly
                    commandPacket
                    .setNextValueAsReferenceTypeID(eventModifier.declaring);
                    commandPacket.setNextValueAsFieldID(eventModifier.fieldID);
                    break;
                }
                case EventMod.ModKind.Step: {
                    // Case Step
                    commandPacket.setNextValueAsThreadID(eventModifier.thread);
                    commandPacket.setNextValueAsInt(eventModifier.size);
                    commandPacket.setNextValueAsInt(eventModifier.depth);
                    break;
                }
                case EventMod.ModKind.InstanceOnly: {
                    // Case InstanceOnly
                    commandPacket.setNextValueAsObjectID(eventModifier.instance);
                    break;
                }
                case EventMod.ModKind.SourceNameMatch: {
                    // Case SourceNameMatch
                    commandPacket.setNextValueAsString(eventModifier.sourceNamePattern);
                }
            }
        }

        // Send packet
        return checkReply(performCommand(commandPacket));
    }

    public Method[] getMethods(long classID) {
        boolean withGeneric = true;
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.MethodsWithGenericCommand);
        commandPacket.setNextValueAsReferenceTypeID(classID);
        ReplyPacket reply = performCommand(commandPacket);
        if (reply.getErrorCode() == JDWPConstants.Error.NOT_IMPLEMENTED) {
            withGeneric = false;
            commandPacket.setCommand(JDWPCommands.ReferenceTypeCommandSet.MethodsCommand);
            reply = performCommand(commandPacket);
        }
        checkReply(reply);

        int declared = reply.getNextValueAsInt();
        Method[] methods = new Method[declared];
        for (int i = 0; i < declared; i++) {
            long methodID = reply.getNextValueAsMethodID();
            String methodName = reply.getNextValueAsString();
            String methodSignature = reply.getNextValueAsString();
            String methodGenericSignature = "";
            if (withGeneric) {
                methodGenericSignature = reply.getNextValueAsString();
            }
            int methodModifiers = reply.getNextValueAsInt();
            methods[i] = new Method(
                methodID,
                methodName,
                methodSignature,
                methodGenericSignature,
                methodModifiers
            );
        }
        return methods;
    }

    /**
     * Gets class reference by signature.
     * 
     * @param classSignature
     *            class signature.
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket getClassBySignature(String classSignature) {
        CommandPacket commandPacket = new CommandPacket(
                JDWPCommands.VirtualMachineCommandSet.CommandSetID,
                JDWPCommands.VirtualMachineCommandSet.ClassesBySignatureCommand);
        commandPacket.setNextValueAsString(classSignature);
        return checkReply(performCommand(commandPacket));
    }

    /**
     * Returns for specified class array with information about fields of this class.
     * <BR>Each element of array contains:
     * <BR>Field ID, Field name, Field signature, Field modifier bit flags;
     * @param refType - ReferenceTypeID, defining class.
     * @return array with information about fields.
     */
    public Field[] getFieldsInfo(long refType) {
        boolean withGeneric = true;
        CommandPacket packet = new CommandPacket(JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.FieldsWithGenericCommand);
        packet.setNextValueAsReferenceTypeID(refType);
        ReplyPacket reply = performCommand(packet);
        if (reply.getErrorCode() == JDWPConstants.Error.NOT_IMPLEMENTED) {
            withGeneric = false;
            packet.setCommand(JDWPCommands.ReferenceTypeCommandSet.FieldsCommand);
            reply = performCommand(packet);
        }
        checkReply(reply);

        int declared = reply.getNextValueAsInt();
        Field[] fields = new Field[declared];
        for (int i = 0; i < declared; i++) {
            long fieldID = reply.getNextValueAsFieldID();
            String fieldName = reply.getNextValueAsString();
            String fieldSignature = reply.getNextValueAsString();
            String fieldGenericSignature = "";
            if (withGeneric) {
                fieldGenericSignature = reply.getNextValueAsString();
            }
            int fieldModifiers = reply.getNextValueAsInt();
            fields[i] = new Field(fieldID, refType, fieldName, fieldSignature,
                    fieldGenericSignature, fieldModifiers);
        }
        return fields;
    }

    /**
     * Sets exception event request for given exception class signature.
     * 
     * @param exceptionSignature
     *            exception signature.
     * @param caught
     *            is exception caught
     * @param uncaught
     *            is exception uncaught
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setException(String exceptionSignature, boolean caught,
            boolean uncaught) {
        // Request referenceTypeID for exception
        long typeID = getClassID(exceptionSignature);
        return setException(typeID, caught, uncaught);
    }

    /**
     * Sets exception event request for given exception class ID.
     * 
     * @param exceptionID
     *            exception referenceTypeID.
     * @param caught
     *            is exception caught
     * @param uncaught
     *            is exception uncaught
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setException(long exceptionID, boolean caught,
            boolean uncaught) {
        byte eventKind = JDWPConstants.EventKind.EXCEPTION;
        byte suspendPolicy = JDWPConstants.SuspendPolicy.ALL;
        Event event = Event.builder(eventKind, suspendPolicy)
                .setExceptionOnly(exceptionID, caught, uncaught)
                .build();
        return setEvent(event);
    }

    /**
     * Sets METHOD_ENTRY event request for specified class name pattern.
     * 
     * @param classRegexp
     *            class name pattern or null for no pattern
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setMethodEntry(String classRegexp) {
        byte eventKind = JDWPConstants.EventKind.METHOD_ENTRY;
        byte suspendPolicy = JDWPConstants.SuspendPolicy.ALL;
        EventBuilder builder = Event.builder(eventKind, suspendPolicy);
        if (classRegexp != null) {
            builder = builder.setClassMatch(classRegexp);
        }
        Event event = builder.build();
        return setEvent(event);
    }

    /**
     * Sets METHOD_EXIT event request for specified class name pattern.
     * 
     * @param classRegexp
     *            class name pattern or null for no pattern
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setMethodExit(String classRegexp) {
        byte eventKind = JDWPConstants.EventKind.METHOD_EXIT;
        byte suspendPolicy = JDWPConstants.SuspendPolicy.ALL;
        EventBuilder builder = Event.builder(eventKind, suspendPolicy);
        if (classRegexp != null) {
            builder = builder.setClassMatch(classRegexp);
        }
        Event event = builder.build();
        return setEvent(event);
    }
    
    /**
     * Sets METHOD_EXIT_WITH_RETURN_VALUE event request for specified class name pattern.
     * 
     * @param classRegexp
     *            class name pattern or null for no pattern
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setMethodExitWithReturnValue(String classRegexp) {
        byte eventKind = JDWPConstants.EventKind.METHOD_EXIT_WITH_RETURN_VALUE;
        byte suspendPolicy = JDWPConstants.SuspendPolicy.ALL;
        EventBuilder builder = Event.builder(eventKind, suspendPolicy);
        if (classRegexp != null) {
            builder = builder.setClassMatch(classRegexp);
        }
        Event event = builder.build();
        return setEvent(event);
    }

    /**
     * Sets field access event request for specified class signature and field
     * name.
     * 
     * @param classTypeTag
     *            class Type Tag (class/interface/array)
     * @param classSignature
     *            class signature
     * @param fieldName
     *            field name
     * @return ReplyPacket if breakpoint is set
     */
    public ReplyPacket setFieldAccess(String classSignature, byte classTypeTag,
            String fieldName) {
        // Request referenceTypeID for class
        long typeID = getClassID(classSignature);

        // Get fieldID from received packet
        long fieldID = getFieldID(typeID, fieldName);

        byte eventKind = JDWPConstants.EventKind.FIELD_ACCESS;
        byte suspendPolicy = JDWPConstants.SuspendPolicy.ALL;
        Event event = Event.builder(eventKind, suspendPolicy)
                .setFieldOnly(typeID, fieldID)
                .build();
        return setEvent(event);
    }

    /**
     * Sets field modification event request for specified class signature and
     * field name.
     * 
     * @param classTypeTag
     *            class Type Tag (class/interface/array)
     * @param classSignature
     *            class signature
     * @param fieldName
     *            field name
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setFieldModification(String classSignature,
            byte classTypeTag, String fieldName) {
        // Request referenceTypeID for class
        long typeID = getClassID(classSignature);

        // Get fieldID from received packet
        long fieldID = getFieldID(typeID, fieldName);

        byte eventKind = JDWPConstants.EventKind.FIELD_MODIFICATION;
        byte suspendPolicy = JDWPConstants.SuspendPolicy.ALL;
        Event event = Event.builder(eventKind, suspendPolicy)
                .setFieldOnly(typeID, fieldID)
                .build();
        return setEvent(event);
    }

    /**
     * Sets step event request for given thread ID.
     *
     * @param threadID
     *          the ID of the thread
     * @param stepSize
     *          the step size
     * @param stepDepth
     *          the step depth
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setStep(long threadID, int stepSize, int stepDepth) {
        byte eventKind = JDWPConstants.EventKind.SINGLE_STEP;
        byte suspendPolicy = JDWPConstants.SuspendPolicy.ALL;
        Event event = Event.builder(eventKind, suspendPolicy)
                .setStep(threadID, stepSize, stepDepth)
                .build();
        return setEvent(event);
    }

    /**
     * Sets SINGLE_STEP event request for classes whose name does not match the
     * given restricted regular expression
     * 
     * @param classRegexp
     *            Disallowed class patterns. Matches are limited to exact
     *            matches of the given class pattern and matches of patterns
     *            that begin or end with '*'; for example, "*.Foo" or "java.*".
     * @param stepSize
     * @param stepDepth
     * @return ReplyPacket for setting request.
     */
    public ReplyPacket setStep(String[] classRegexp, long threadID,
            int stepSize, int stepDepth) {
        byte eventKind = JDWPConstants.EventKind.SINGLE_STEP;
        byte suspendPolicy = JDWPConstants.SuspendPolicy.ALL;
        EventBuilder builder = Event.builder(eventKind, suspendPolicy);
        for (String pattern : classRegexp) {
            builder.setClassExclude(pattern);
        }
        Event event = builder.setStep(threadID, stepSize, stepDepth).build();
        return setEvent(event);
    }

    /**
     * Sets THREAD_START event request.
     * 
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setThreadStart(byte suspendPolicy) {
        byte eventKind = JDWPConstants.EventKind.THREAD_START;
        Event event = Event.builder(eventKind, suspendPolicy).build();
        return setEvent(event);
    }

    /**
     * Sets THREAD_END event request.
     *
     * @param suspendPolicy the suspend policy
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket setThreadEnd(byte suspendPolicy) {
        byte eventKind = JDWPConstants.EventKind.THREAD_END;
        Event event = Event.builder(eventKind, suspendPolicy).build();
        return setEvent(event);
    }

    private ReplyPacket setClassOnlyEvent(byte eventKind, byte suspendPolicy, long classId) {
        Event event = Event.builder(eventKind, suspendPolicy)
                .setClassOnly(classId)
                .build();
        return setEvent(event);
    }

    private ReplyPacket setClassMatchEvent(byte eventKind, byte suspendPolicy, String pattern) {
        Event event = Event.builder(eventKind, suspendPolicy)
                .setClassMatch(pattern)
                .build();
        return setEvent(event);
    }

    private ReplyPacket setClassExcludeEvent(byte eventKind, byte suspendPolicy, String pattern) {
        Event event = Event.builder(eventKind, suspendPolicy)
                .setClassExclude(pattern)
                .build();
        return setEvent(event);
    }

    /**
     * Clear an event request for specified request ID.
     * 
     * @param eventKind
     *            event type to clear
     * @param requestID
     *            request ID to clear
     * @return ReplyPacket for corresponding command
     */
    public ReplyPacket clearEvent(byte eventKind, int requestID) {
        // Create new command packet
        CommandPacket commandPacket = new CommandPacket();

        // Set command. "2" - is ID of Clear command in EventRequest Command Set
        commandPacket
                .setCommand(JDWPCommands.EventRequestCommandSet.ClearCommand);

        // Set command set. "15" - is ID of EventRequest Command Set
        commandPacket
                .setCommandSet(JDWPCommands.EventRequestCommandSet.CommandSetID);

        // Set outgoing data
        // Set event type to clear
        commandPacket.setNextValueAsByte(eventKind);

        // Set ID of request to clear
        commandPacket.setNextValueAsInt(requestID);

        // Send packet
        return checkReply(performCommand(commandPacket));
    }

    /**
     * Sends CommandPacket to debuggee VM and waits for ReplyPacket using
     * default timeout. All thrown exceptions are wrapped into
     * TestErrorException. Consider using checkReply() for checking error code
     * in reply packet.
     * 
     * @param command
     *            Command packet to be sent
     * @return received ReplyPacket
     */
    public ReplyPacket performCommand(CommandPacket command)
            throws TestErrorException {
        ReplyPacket replyPacket = null;
        try {
            replyPacket = packetDispatcher.performCommand(command);
        } catch (IOException e) {
            throw new TestErrorException(e);
        } catch (InterruptedException e) {
            throw new TestErrorException(e);
        }

        return replyPacket;
    }

    /**
     * Sends CommandPacket to debuggee VM and waits for ReplyPacket using
     * specified timeout.
     * 
     * @param command
     *            Command packet to be sent
     * @param timeout
     *            Timeout in milliseconds for waiting reply packet
     * @return received ReplyPacket
     * @throws InterruptedException
     * @throws IOException
     * @throws TimeoutException
     */
    public ReplyPacket performCommand(CommandPacket command, long timeout)
            throws IOException, InterruptedException, TimeoutException {

        return packetDispatcher.performCommand(command, timeout);
    }

    /**
     * Sends CommandPacket to debuggee VM without waiting for the reply. This
     * method is intended for special cases when there is need to divide
     * command's performing into two actions: command's sending and receiving
     * reply (e.g. for asynchronous JDWP commands' testing). After this method
     * the 'receiveReply()' method must be used latter for receiving reply for
     * sent command. It is NOT recommended to use this method for usual cases -
     * 'performCommand()' method must be used.
     * 
     * @param command
     *            Command packet to be sent
     * @return command ID of sent command
     * @throws IOException
     *             if any connection error occurred
     */
    public int sendCommand(CommandPacket command) throws IOException {
        return packetDispatcher.sendCommand(command);
    }

    /**
     * Waits for reply for command which was sent before by 'sendCommand()'
     * method. Default timeout is used as time limit for waiting. This method
     * (jointly with 'sendCommand()') is intended for special cases when there
     * is need to divide command's performing into two actions: command's
     * sending and receiving reply (e.g. for asynchronous JDWP commands'
     * testing). It is NOT recommended to use 'sendCommand()- receiveReply()'
     * pair for usual cases - 'performCommand()' method must be used.
     * 
     * @param commandId
     *            Command ID of sent before command, reply from which is
     *            expected to be received
     * @return received ReplyPacket
     * @throws IOException
     *             if any connection error occurred
     * @throws InterruptedException
     *             if reply packet's waiting was interrupted
     * @throws TimeoutException
     *             if timeout exceeded
     */
    public ReplyPacket receiveReply(int commandId) throws InterruptedException,
            IOException, TimeoutException {
        return packetDispatcher.receiveReply(commandId, config.getTimeout());
    }

    /**
     * Waits for reply for command which was sent before by 'sendCommand()'
     * method. Specified timeout is used as time limit for waiting. This method
     * (jointly with 'sendCommand()') is intended for special cases when there
     * is need to divide command's performing into two actions: command's
     * sending and receiving reply (e.g. for asynchronous JDWP commands'
     * testing). It is NOT recommended to use 'sendCommand()- receiveReply()'
     * pair for usual cases - 'performCommand()' method must be used.
     * 
     * @param commandId
     *            Command ID of sent before command, reply from which is
     *            expected to be received
     * @param timeout
     *            Specified timeout in milliseconds to wait for reply
     * @return received ReplyPacket
     * @throws IOException
     *             if any connection error occurred
     * @throws InterruptedException
     *             if reply packet's waiting was interrupted
     * @throws TimeoutException
     *             if timeout exceeded
     */
    public ReplyPacket receiveReply(int commandId, long timeout)
            throws InterruptedException, IOException, TimeoutException {
        return packetDispatcher.receiveReply(commandId, timeout);
    }

    /**
     * Waits for EventPacket using default timeout. All thrown exceptions are
     * wrapped into TestErrorException.
     * 
     * @return received EventPacket
     */
    public EventPacket receiveEvent() throws TestErrorException {
        try {
            return receiveEvent(config.getTimeout());
        } catch (IOException e) {
            throw new TestErrorException(e);
        } catch (InterruptedException e) {
            throw new TestErrorException(e);
        }
    }

    /**
     * Waits for EventPacket using specified timeout.
     * 
     * @param timeout
     *            Timeout in milliseconds to wait for event
     * @return received EventPacket
     * @throws IOException
     * @throws InterruptedException
     * @throws TimeoutException
     */
    public EventPacket receiveEvent(long timeout) throws IOException,
            InterruptedException, TimeoutException {

        return packetDispatcher.receiveEvent(timeout);
    }

    /**
     * Waits for expected event kind using default timeout. Throws
     * TestErrorException if received event is not of expected kind or not a
     * single event in the received event set.
     * 
     * @param eventKind
     *            Type of expected event -
     * @see JDWPConstants.EventKind
     * @return received EventPacket
     */
    public EventPacket receiveCertainEvent(byte eventKind)
            throws TestErrorException {

        EventPacket eventPacket = receiveEvent();
        ParsedEvent[] parsedEvents = ParsedEvent.parseEventPacket(eventPacket);

        if (parsedEvents.length == 1
                && parsedEvents[0].getEventKind() == eventKind)
            return eventPacket;

        switch (parsedEvents.length) {
        case (0):
            throw new TestErrorException(
                    "Unexpected event received: zero length");
        case (1):
            throw new TestErrorException("Unexpected event received: "
                    + "expected " + JDWPConstants.EventKind.getName(eventKind)
                    + " (" + eventKind + ") but received "
                    + JDWPConstants.EventKind.getName(parsedEvents[0].getEventKind())
                    + " (" + parsedEvents[0].getEventKind() + ")");
        default:
            throw new TestErrorException(
                    "Unexpected event received: Event was grouped in a composite event");
        }
    }

    /**
     * Returns JDWP connection channel used by this VmMirror.
     * 
     * @return connection channel
     */
    public TransportWrapper getConnection() {
        return connection;
    }

    /**
     * Sets established connection channel to be used with this VmMirror and
     * starts reading packets.
     * 
     * @param connection
     *            connection channel to be set
     */
    public void setConnection(TransportWrapper connection) {
        this.connection = connection;
        packetDispatcher = new PacketDispatcher(connection, config, logWriter);
    }

    /**
     * Closes connection channel used with this VmMirror and stops reading
     * packets.
     * 
     */
    public void closeConnection() throws IOException {
        if (connection != null && connection.isOpen())
            connection.close();

        // wait for packetDispatcher is closed
        if (packetDispatcher != null) {
            try {
                packetDispatcher.join();
            } catch (InterruptedException e) {
                // do nothing but print a stack trace
                e.printStackTrace();
            }
        }
    }

    /**
     * Returns the count of frames on this thread's stack
     * 
     * @param threadID
     *            The thread object ID.
     * @return The count of frames on this thread's stack
     */
    public final int getFrameCount(long threadID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.FrameCountCommand);
        command.setNextValueAsThreadID(threadID);
        ReplyPacket reply = checkReply(performCommand(command));
        return reply.getNextValueAsInt();
    }

    /**
     * Returns a list containing all frames of a certain thread
     * 
     * @param threadID
     *            ID of the thread
     * @return A list of frames
     */
    public final List<Frame> getAllThreadFrames(long threadID) {
        if (!isThreadSuspended(threadID)) {
            return new ArrayList<Frame>(0);
        }

        ReplyPacket reply = getThreadFrames(threadID, 0, -1);
        int framesCount = reply.getNextValueAsInt();
        if (framesCount == 0) {
            return new ArrayList<Frame>(0);
        }

        ArrayList<Frame> frames = new ArrayList<Frame>(framesCount);
        for (int i = 0; i < framesCount; i++) {
            Frame frame = new Frame();
            frame.setThreadID(threadID);
            frame.setID(reply.getNextValueAsFrameID());
            frame.setLocation(reply.getNextValueAsLocation());
            frames.add(frame);
        }

        return frames;
    }

    /**
     * Returns a set of frames of a certain suspended thread
     * 
     * @param threadID
     *            ID of the thread whose frames to obtain
     * @param startIndex
     *            The index of the first frame to retrieve.
     * @param length
     *            The count of frames to retrieve (-1 means all remaining).
     * @return ReplyPacket for corresponding command
     */
    public final ReplyPacket getThreadFrames(long threadID, int startIndex,
            int length) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.FramesCommand);
        command.setNextValueAsThreadID(threadID);
        command.setNextValueAsInt(startIndex); // start frame's index
        command.setNextValueAsInt(length); // get all remaining frames;
        return checkReply(performCommand(command));
    }

    /**
     * Returns variable information for the method
     * 
     * @param classID
     *            The class ID
     * @param methodID
     *            The method ID
     * @return A list containing all variables (arguments and locals) declared
     *         within the method.
     */
    public final List<Variable> getVariableTable(long classID, long methodID) {
        boolean withGeneric = true;
        CommandPacket command = new CommandPacket(
                JDWPCommands.MethodCommandSet.CommandSetID,
                JDWPCommands.MethodCommandSet.VariableTableWithGenericCommand);
        command.setNextValueAsReferenceTypeID(classID);
        command.setNextValueAsMethodID(methodID);
        ReplyPacket reply = performCommand(command);
        if (reply.getErrorCode() == JDWPConstants.Error.NOT_IMPLEMENTED) {
            withGeneric = false;
            command.setCommand(JDWPCommands.MethodCommandSet.VariableTableCommand);
            reply = performCommand(command);
        }
        if (reply.getErrorCode() == JDWPConstants.Error.ABSENT_INFORMATION
                || reply.getErrorCode() == JDWPConstants.Error.NATIVE_METHOD) {
            return null;
        }

        checkReply(reply);

        reply.getNextValueAsInt(); // argCnt, is not used
        int slots = reply.getNextValueAsInt();
        ArrayList<Variable> vars = new ArrayList<Variable>(slots);
        for (int i = 0; i < slots; i++) {
            Variable var = new Variable();
            var.setCodeIndex(reply.getNextValueAsLong());
            var.setName(reply.getNextValueAsString());
            var.setSignature(reply.getNextValueAsString());
            if (withGeneric) {
                var.setGenericSignature(reply.getNextValueAsString());
            }
            var.setLength(reply.getNextValueAsInt());
            var.setSlot(reply.getNextValueAsInt());
            vars.add(var);
        }

        return vars;
    }

    /**
     * Returns values of local variables in a given frame
     * 
     * @param frame
     *            Frame whose variables to get
     * @return An array of Value objects
     */
    public final Value[] getFrameValues(Frame frame) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.GetValuesCommand);
        command.setNextValueAsThreadID(frame.getThreadID());
        command.setNextValueAsFrameID(frame.getID());
        int slots = frame.getVars().size();
        command.setNextValueAsInt(slots);
        Iterator<?> it = frame.getVars().iterator();
        while (it.hasNext()) {
            Frame.Variable var = (Frame.Variable) it.next();
            command.setNextValueAsInt(var.getSlot());
            command.setNextValueAsByte(var.getTag());
        }

        ReplyPacket reply = checkReply(performCommand(command));
        reply.getNextValueAsInt(); // number of values , is not used
        Value[] values = new Value[slots];
        for (int i = 0; i < slots; i++) {
            values[i] = reply.getNextValueAsValue();
        }

        return values;
    }

    /**
     * Returns the immediate superclass of a class
     * 
     * @param classID
     *            The class ID whose superclass ID is to get
     * @return The superclass ID (null if the class ID for java.lang.Object is
     *         specified).
     */
    public final long getSuperclassId(long classID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ClassTypeCommandSet.CommandSetID,
                JDWPCommands.ClassTypeCommandSet.SuperclassCommand);
        command.setNextValueAsClassID(classID);
        ReplyPacket reply = checkReply(performCommand(command));
        return reply.getNextValueAsClassID();
    }

    /**
     * Returns the runtime type of the object
     * 
     * @param objectID
     *            The object ID
     * @return The runtime reference type.
     */
    public final long getReferenceType(long objectID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ObjectReferenceCommandSet.CommandSetID,
                JDWPCommands.ObjectReferenceCommandSet.ReferenceTypeCommand);
        command.setNextValueAsObjectID(objectID);
        ReplyPacket reply = checkReply(performCommand(command));
        reply.getNextValueAsByte();
        return reply.getNextValueAsLong();
    }

    /**
     * Returns the class object corresponding to this type
     * 
     * @param refType
     *            The reference type ID.
     * @return The class object.
     */
    public final long getClassObjectId(long refType) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.ClassObjectCommand);
        command.setNextValueAsReferenceTypeID(refType);
        ReplyPacket reply = checkReply(performCommand(command));
        return reply.getNextValueAsObjectID();
    }

    /**
     * Returns line number information for the method, if present.
     * 
     * @param refType
     *            The class ID
     * @param methodID
     *            The method ID
     * @return ReplyPacket for corresponding command.
     */
    public final ReplyPacket getLineTable(long refType, long methodID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.MethodCommandSet.CommandSetID,
                JDWPCommands.MethodCommandSet.LineTableCommand);
        command.setNextValueAsReferenceTypeID(refType);
        command.setNextValueAsMethodID(methodID);
        // ReplyPacket reply =
        // debuggeeWrapper.vmMirror.checkReply(debuggeeWrapper.vmMirror.performCommand(command));
        // it is impossible to obtain line table information from native
        // methods, so reply checking is not performed
        ReplyPacket reply = performCommand(command);
        if (reply.getErrorCode() != JDWPConstants.Error.NONE) {
            if (reply.getErrorCode() == JDWPConstants.Error.NATIVE_METHOD) {
                return reply;
            }
        }

        return checkReply(reply);
    }

    /**
     * Returns the value of one or more instance fields.
     * 
     * @param objectID
     *            The object ID
     * @param fieldIDs
     *            IDs of fields to get
     * @return An array of Value objects representing each field's value
     */
    public final Value[] getObjectReferenceValues(long objectID, long[] fieldIDs) {
        int fieldsCount = fieldIDs.length;
        if (fieldsCount == 0) {
            return null;
        }

        CommandPacket command = new CommandPacket(
                JDWPCommands.ObjectReferenceCommandSet.CommandSetID,
                JDWPCommands.ObjectReferenceCommandSet.GetValuesCommand);
        command.setNextValueAsReferenceTypeID(objectID);
        command.setNextValueAsInt(fieldsCount);
        for (int i = 0; i < fieldsCount; i++) {
            command.setNextValueAsFieldID(fieldIDs[i]);
        }

        ReplyPacket reply = checkReply(performCommand(command));
        reply.getNextValueAsInt(); // fields returned, is not used
        Value[] values = new Value[fieldsCount];
        for (int i = 0; i < fieldsCount; i++) {
            values[i] = reply.getNextValueAsValue();
        }

        return values;
    }

    /**
     * Returns the value of one or more static fields of the reference type
     * 
     * @param refTypeID
     *            The reference type ID.
     * @param fieldIDs
     *            IDs of fields to get
     * @return An array of Value objects representing each field's value
     */
    public final Value[] getReferenceTypeValues(long refTypeID, long[] fieldIDs) {
        int fieldsCount = fieldIDs.length;
        if (fieldsCount == 0) {
            return null;
        }

        CommandPacket command = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.GetValuesCommand);
        command.setNextValueAsReferenceTypeID(refTypeID);
        command.setNextValueAsInt(fieldsCount);
        for (int i = 0; i < fieldsCount; i++) {
            command.setNextValueAsFieldID(fieldIDs[i]);
        }

        ReplyPacket reply = checkReply(performCommand(command));
        reply.getNextValueAsInt(); // fields returned, is not used
        Value[] values = new Value[fieldsCount];
        for (int i = 0; i < fieldsCount; i++) {
            values[i] = reply.getNextValueAsValue();
        }

        return values;
    }

    /**
     * Returns the value of one static field of the reference type
     *
     * @param refTypeID
     *            The reference type ID.
     * @param fieldID
     *            ID of field to get
     * @return A Value object representing the field's value
     */
    public final Value getReferenceTypeValue(long refTypeID, long fieldID) {
        Value[] values = getReferenceTypeValues(refTypeID, new long[]{fieldID});
        return values[0];
    }

    /**
     * Returns the value of the 'this' reference for this frame
     * 
     * @param threadID
     *            The frame's thread ID
     * @param frameID
     *            The frame ID.
     * @return The 'this' object ID for this frame.
     */
    public final long getThisObject(long threadID, long frameID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.ThisObjectCommand);
        command.setNextValueAsThreadID(threadID);
        command.setNextValueAsFrameID(frameID);
        ReplyPacket reply = checkReply(performCommand(command));
        TaggedObject taggedObject = reply.getNextValueAsTaggedObject();
        return taggedObject.objectID;
    }

    /**
     * Returns the reference type reflected by this class object
     * 
     * @param classObjectID
     *            The class object ID.
     * @return ReplyPacket for corresponding command
     */
    public final ReplyPacket getReflectedType(long classObjectID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ClassObjectReferenceCommandSet.CommandSetID,
                JDWPCommands.ClassObjectReferenceCommandSet.ReflectedTypeCommand);
        command.setNextValueAsClassObjectID(classObjectID);
        return checkReply(performCommand(command));
    }

    /**
     * Returns the JNI signature of a reference type. JNI signature formats are
     * described in the Java Native Interface Specification
     * 
     * @param refTypeID
     *            The reference type ID.
     * @return The JNI signature for the reference type.
     */
    public final String getReferenceTypeSignature(long refTypeID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ReferenceTypeCommandSet.CommandSetID,
                JDWPCommands.ReferenceTypeCommandSet.SignatureCommand);
        command.setNextValueAsReferenceTypeID(refTypeID);
        ReplyPacket reply = checkReply(performCommand(command));
        return reply.getNextValueAsString();
    }

    /**
     * Returns the thread group that contains a given thread
     * 
     * @param threadID
     *            The thread object ID.
     * @return The thread group ID of this thread.
     */
    public final long getThreadGroupID(long threadID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.ThreadGroupCommand);
        command.setNextValueAsThreadID(threadID);
        ReplyPacket reply = checkReply(performCommand(command));
        return reply.getNextValueAsThreadGroupID();
    }

    /**
     * Checks whether a given thread is suspended or not
     * 
     * @param threadID
     *            The thread object ID.
     * @return True if a given thread is suspended, false otherwise.
     */
    public final boolean isThreadSuspended(long threadID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.StatusCommand);
        command.setNextValueAsThreadID(threadID);
        ReplyPacket reply = checkReply(performCommand(command));
        reply.getNextValueAsInt(); // the thread's status; is not used
        return reply.getNextValueAsInt() == JDWPConstants.SuspendStatus.SUSPEND_STATUS_SUSPENDED;
    }

    /**
     * Returns JNI signature of method.
     * 
     * @param classID
     *            The reference type ID.
     * @param methodID
     *            The method ID.
     * @return JNI signature of method.
     */
    public final String getMethodSignature(long classID, long methodID) {
        Method[] methods = getMethods(classID);
        for (Method method : methods) {
            if (methodID == method.getMethodID()) {
                String value = method.getSignature();
                value = value.replaceAll("/", ".");
                int lastRoundBracketIndex = value.lastIndexOf(")");
                value = value.substring(0, lastRoundBracketIndex + 1);
                return value;
            }
        }
        return null;
    }

    /**
     * Returns the characters contained in the string
     * 
     * @param objectID
     *            The String object ID.
     * @return A string value.
     */
    public final String getStringValue(long objectID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.StringReferenceCommandSet.CommandSetID,
                JDWPCommands.StringReferenceCommandSet.ValueCommand);
        command.setNextValueAsObjectID(objectID);
        ReplyPacket reply = checkReply(performCommand(command));
        return reply.getNextValueAsString();
    }

    /**
     * Returns a range of array components
     * 
     * @param objectID
     *            The array object ID.
     * @return The retrieved values.
     */
    public Value[] getArrayValues(long objectID) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ArrayReferenceCommandSet.CommandSetID,
                JDWPCommands.ArrayReferenceCommandSet.LengthCommand);
        command.setNextValueAsArrayID(objectID);
        ReplyPacket reply = checkReply(performCommand(command));
        int length = reply.getNextValueAsInt();

        if (length == 0) {
            return null;
        }

        command = new CommandPacket(
                JDWPCommands.ArrayReferenceCommandSet.CommandSetID,
                JDWPCommands.ArrayReferenceCommandSet.GetValuesCommand);
        command.setNextValueAsArrayID(objectID);
        command.setNextValueAsInt(0);
        command.setNextValueAsInt(length);
        reply = checkReply(performCommand(command));
        ArrayRegion arrayRegion = reply.getNextValueAsArrayRegion();

        Value[] values = new Value[length];
        for (int i = 0; i < length; i++) {
            values[i] = arrayRegion.getValue(i);
        }

        return values;
    }

    /**
     * Returns a source line number according to a corresponding line code index
     * in a method's line table.
     * 
     * @param classID
     *            The class object ID.
     * @param methodID
     *            The method ID.
     * @param codeIndex
     *            The line code index.
     * @return An integer line number.
     */
    public final int getLineNumber(long classID, long methodID, long codeIndex) {
        int lineNumber = -1;
        ReplyPacket reply = getLineTable(classID, methodID);
        if (reply.getErrorCode() != JDWPConstants.Error.NONE) {
            return lineNumber;
        }

        reply.getNextValueAsLong(); // start line index, is not used
        reply.getNextValueAsLong(); // end line index, is not used
        int lines = reply.getNextValueAsInt();
        for (int i = 0; i < lines; i++) {
            long lineCodeIndex = reply.getNextValueAsLong();
            lineNumber = reply.getNextValueAsInt();
            if (lineCodeIndex == codeIndex) {
                break;
            }

            if (lineCodeIndex > codeIndex) {
                --lineNumber;
                break;
            }
        }

        return lineNumber;
    }

    /**
     * Returns a line code index according to a corresponding line number in a
     * method's line table.
     * 
     * @param classID
     *            The class object ID.
     * @param methodID
     *            The method ID.
     * @param lineNumber
     *            A source line number.
     * @return An integer representing the line code index.
     */
    public final long getLineCodeIndex(long classID, long methodID,
            int lineNumber) {
        ReplyPacket reply = getLineTable(classID, methodID);
        if (reply.getErrorCode() != JDWPConstants.Error.NONE) {
            return -1L;
        }

        reply.getNextValueAsLong(); // start line index, is not used
        reply.getNextValueAsLong(); // end line index, is not used
        int lines = reply.getNextValueAsInt();
        for (int i = 0; i < lines; i++) {
            long lineCodeIndex = reply.getNextValueAsLong();
            if (lineNumber == reply.getNextValueAsInt()) {
                return lineCodeIndex;
            }
        }

        return -1L;
    }

    /**
     * Returns all variables which are visible within the given frame.
     * 
     * @param frame
     *            The frame whose visible local variables to retrieve.
     * @return A list of Variable objects representing each visible local
     *         variable within the given frame.
     */
    public final List<Variable> getLocalVars(Frame frame) {
        List<Variable> vars = getVariableTable(frame.getLocation().classID, frame
                .getLocation().methodID);
        if (vars == null) {
            return null;
        }

        // All variables that are not visible from within current frame must be
        // removed from the list
        long frameCodeIndex = frame.getLocation().index;
        for (int i = 0; i < vars.size(); i++) {
            Variable var = (Variable) vars.toArray()[i];
            long varCodeIndex = var.getCodeIndex();
            if (varCodeIndex > frameCodeIndex
                    || (frameCodeIndex >= varCodeIndex + var.getLength())) {
                vars.remove(i);
                --i;
                continue;
            }
        }

        return vars;
    }

    /**
     * Sets the value of one or more local variables
     * 
     * @param frame
     *            The frame ID.
     * @param vars
     *            An array of Variable objects whose values to set
     * @param values
     *            An array of Value objects to set
     */
    public final void setLocalVars(Frame frame, Variable[] vars, Value[] values) {
        if (vars.length != values.length) {
            throw new TestErrorException(
                    "Number of variables doesn't correspond to number of their values");
        }

        CommandPacket command = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.SetValuesCommand);
        command.setNextValueAsThreadID(frame.getThreadID());
        command.setNextValueAsFrameID(frame.getID());
        command.setNextValueAsInt(vars.length);
        for (int i = 0; i < vars.length; i++) {
            command.setNextValueAsInt(vars[i].getSlot());
            command.setNextValueAsValue(values[i]);
        }

        checkReply(performCommand(command));
    }

    /**
     * Sets the value of one or more instance fields
     * 
     * @param objectID
     *            The object ID.
     * @param fieldIDs
     *            An array of fields IDs
     * @param values
     *            An array of Value objects representing each value to set
     */
    public final void setInstanceFieldsValues(long objectID, long[] fieldIDs,
            Value[] values) {
        if (fieldIDs.length != values.length) {
            throw new TestErrorException(
                    "Number of fields doesn't correspond to number of their values");
        }

        CommandPacket command = new CommandPacket(
                JDWPCommands.ObjectReferenceCommandSet.CommandSetID,
                JDWPCommands.ObjectReferenceCommandSet.SetValuesCommand);
        command.setNextValueAsObjectID(objectID);
        command.setNextValueAsInt(fieldIDs.length);
        for (int i = 0; i < fieldIDs.length; i++) {
            command.setNextValueAsFieldID(fieldIDs[i]);
            command.setNextValueAsUntaggedValue(values[i]);
        }

        checkReply(performCommand(command));
    }

    /**
     * Sets a range of array components. The specified range must be within the
     * bounds of the array.
     * 
     * @param arrayID
     *            The array object ID.
     * @param firstIndex
     *            The first index to set.
     * @param values
     *            An array of Value objects representing each value to set.
     */
    public final void setArrayValues(long arrayID, int firstIndex,
            Value[] values) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.ArrayReferenceCommandSet.CommandSetID,
                JDWPCommands.ArrayReferenceCommandSet.SetValuesCommand);
        command.setNextValueAsArrayID(arrayID);
        command.setNextValueAsInt(firstIndex);
        command.setNextValueAsInt(values.length);
        for (int i = 0; i < values.length; i++) {
            command.setNextValueAsUntaggedValue(values[i]);
        }

        checkReply(performCommand(command));
    }

    /**
     * Sets the value of one or more static fields
     * 
     * @param classID
     *            The class type ID.
     * @param fieldIDs
     *            An array of fields IDs
     * @param values
     *            An array of Value objects representing each value to set
     */
    public final void setStaticFieldsValues(long classID, long[] fieldIDs,
            Value[] values) {
        if (fieldIDs.length != values.length) {
            throw new TestErrorException(
                    "Number of fields doesn't correspond to number of their values");
        }

        CommandPacket command = new CommandPacket(
                JDWPCommands.ClassTypeCommandSet.CommandSetID,
                JDWPCommands.ClassTypeCommandSet.SetValuesCommand);
        command.setNextValueAsClassID(classID);
        command.setNextValueAsInt(fieldIDs.length);
        for (int i = 0; i < fieldIDs.length; i++) {
            command.setNextValueAsFieldID(fieldIDs[i]);
            command.setNextValueAsUntaggedValue(values[i]);
        }

        checkReply(performCommand(command));
    }

    /**
     * Creates java String in target VM with the given value.
     * 
     * @param value
     *            The value of the string.
     * @return The string id.
     */
    public final long createString(String value) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.VirtualMachineCommandSet.CommandSetID,
                JDWPCommands.VirtualMachineCommandSet.CreateStringCommand);
        command.setNextValueAsString(value);
        ReplyPacket reply = checkReply(performCommand(command));
        return reply.getNextValueAsStringID();
    }

    /**
     * Processes JDWP PopFrames command from StackFrame command set.
     * 
     * @param frame
     *            The instance of Frame.
     */
    public final void popFrame(Frame frame) {
        CommandPacket command = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.PopFramesCommand);
        command.setNextValueAsThreadID(frame.getThreadID());
        command.setNextValueAsFrameID(frame.getID());
        checkReply(performCommand(command));
    }

    /**
     * Invokes a member method of the given object.
     * 
     * @param objectID
     *            The object ID.
     * @param threadID
     *            The thread ID.
     * @param methodName
     *            The name of method for the invocation.
     * @param args
     *            The arguments for the invocation.
     * @param options
     *            The invocation options.
     * @return ReplyPacket for corresponding command
     */
    public final ReplyPacket invokeInstanceMethod(long objectID, long threadID,
            String methodName, Value[] args, int options) {
        long classID = getReferenceType(objectID);
        long methodID = getMethodID(classID, methodName);
        CommandPacket command = new CommandPacket(
                JDWPCommands.ObjectReferenceCommandSet.CommandSetID,
                JDWPCommands.ObjectReferenceCommandSet.InvokeMethodCommand);
        command.setNextValueAsObjectID(objectID);
        command.setNextValueAsThreadID(threadID);
        command.setNextValueAsClassID(classID);
        command.setNextValueAsMethodID(methodID);
        command.setNextValueAsInt(args.length);
        for (int i = 0; i < args.length; i++) {
            command.setNextValueAsValue(args[i]);
        }
        command.setNextValueAsInt(options);

        return checkReply(performCommand(command));
    }

    /**
     * Invokes a static method of the given class.
     * 
     * @param classID
     *            The class type ID.
     * @param threadID
     *            The thread ID.
     * @param methodName
     *            The name of method for the invocation.
     * @param args
     *            The arguments for the invocation.
     * @param options
     *            The invocation options.
     * @return ReplyPacket for corresponding command
     */
    public final ReplyPacket invokeStaticMethod(long classID, long threadID,
            String methodName, Value[] args, int options) {
        long methodID = getMethodID(classID, methodName);
        CommandPacket command = new CommandPacket(
                JDWPCommands.ClassTypeCommandSet.CommandSetID,
                JDWPCommands.ClassTypeCommandSet.InvokeMethodCommand);
        command.setNextValueAsClassID(classID);
        command.setNextValueAsThreadID(threadID);
        command.setNextValueAsMethodID(methodID);
        command.setNextValueAsInt(args.length);
        for (int i = 0; i < args.length; i++) {
            command.setNextValueAsValue(args[i]);
        }
        command.setNextValueAsInt(options);

        return checkReply(performCommand(command));
    }
}
