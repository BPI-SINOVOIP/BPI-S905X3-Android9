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
 * @author Anton V. Karnachuk
 */

/**
 * Created on 16.02.2005
 */
package org.apache.harmony.jpda.tests.jdwp.StackFrame;

import java.util.List;
import org.apache.harmony.jpda.tests.framework.jdwp.CommandPacket;
import org.apache.harmony.jpda.tests.framework.jdwp.Frame;
import org.apache.harmony.jpda.tests.framework.jdwp.Frame.Variable;
import org.apache.harmony.jpda.tests.framework.jdwp.JDWPCommands;
import org.apache.harmony.jpda.tests.framework.jdwp.Location;
import org.apache.harmony.jpda.tests.framework.jdwp.ReplyPacket;
import org.apache.harmony.jpda.tests.jdwp.share.JDWPSyncTestCase;


public class JDWPStackFrameTestCase extends JDWPSyncTestCase {
    
    public class FrameInfo {
        long frameID;
        Location location;
        
        public FrameInfo(long frameID, Location location) {
            super();
            this.frameID = frameID;
            this.location = location;
        }
        
        /**
         * @return Returns the frameID.
         */
        public long getFrameID() {
            return frameID;
        }
        /**
         * @return Returns the location.
         */
        public Location getLocation() {
            return location;
        }
    }
    
    @Override
    protected String getDebuggeeClassName() {
        return StackTraceDebuggee.class.getName();
    }
    
    protected int jdwpGetFrameCount(long threadID) {
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.FrameCountCommand);
        packet.setNextValueAsThreadID(threadID);
        
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ThreadReference::FrameCount command");
        
        int frameCount = reply.getNextValueAsInt();
        return frameCount;
    }

    protected FrameInfo[] jdwpGetFrames(long threadID, int startFrame, int length) {
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.FramesCommand);
        packet.setNextValueAsThreadID(threadID);
        packet.setNextValueAsInt(startFrame);
        packet.setNextValueAsInt(length);
        
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ThreadReference::FramesCommand command");
               
        int frames = reply.getNextValueAsInt();
        FrameInfo[] frameInfos = new FrameInfo[frames];
        for (int i = 0; i < frames; i++) {
            long frameID = reply.getNextValueAsLong();
            Location location = reply.getNextValueAsLocation();
            frameInfos[i] = new FrameInfo(frameID, location);
        }
        return frameInfos;
    }
    
    protected Frame.Variable[] jdwpGetVariableTable(long classID, long methodID) {
        List<Variable> variables = debuggeeWrapper.vmMirror.getVariableTable(classID, methodID);
        if (variables == null) {
            // We failed to collect variable table
            fail("No variable table");
        }
        Frame.Variable[] varInfos = new Frame.Variable[variables.size()];
        varInfos = variables.toArray(varInfos);
        return varInfos;
    }

    protected long[] jdwpGetAllThreads() {
        CommandPacket packet = new CommandPacket(
                JDWPCommands.VirtualMachineCommandSet.CommandSetID,
                JDWPCommands.VirtualMachineCommandSet.AllThreadsCommand);
        
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "VirtualMachine::AllThreads command");
        
        int frames = reply.getNextValueAsInt();
        
        long[] frameIDs = new long[frames]; 
        for (int i = 0; i < frames; i++) {
            frameIDs[i] = reply.getNextValueAsLong();
        }
        
        return frameIDs;
    }

    protected String jdwpGetThreadName(long threadID) {
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.NameCommand);
        packet.setNextValueAsThreadID(threadID);
        
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ThreadReference::Name command");
        
        String name= reply.getNextValueAsString();
        return name;
    }
    
    protected void jdwpSuspendThread(long threadID) {
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.SuspendCommand);
        packet.setNextValueAsThreadID(threadID);
        
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ThreadReference::Suspend command");
    }
    
    protected void jdwpResumeThread(long threadID) {
        CommandPacket packet = new CommandPacket(
                JDWPCommands.ThreadReferenceCommandSet.CommandSetID,
                JDWPCommands.ThreadReferenceCommandSet.ResumeCommand);
        packet.setNextValueAsThreadID(threadID);
        
        ReplyPacket reply = debuggeeWrapper.vmMirror.performCommand(packet);
        checkReplyPacket(reply, "ThreadReference::Resume command");
    }
 
    protected void jdwpPopFrames(long threadID, long frameID) {
        CommandPacket popFramesCommand = new CommandPacket(
                JDWPCommands.StackFrameCommandSet.CommandSetID,
                JDWPCommands.StackFrameCommandSet.PopFramesCommand);
        popFramesCommand.setNextValueAsThreadID(threadID);
        popFramesCommand.setNextValueAsFrameID(frameID);

        ReplyPacket reply = debuggeeWrapper.vmMirror
                .performCommand(popFramesCommand);
        checkReplyPacket(reply, "StackFrame::PopFramesCommand command");
    }

}
