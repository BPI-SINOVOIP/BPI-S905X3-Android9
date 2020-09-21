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

/**
 * Created on 29.01.2005
 */
package org.apache.harmony.jpda.tests.jdwp.share;

import java.io.IOException;

import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.framework.jdwp.TransportWrapper;
import org.apache.harmony.jpda.tests.share.JPDATestOptions;

/**
 * This class provides DebuggeeWrapper implementation based on JUnit framework.
 * Debuggee is always launched on local machine and attaches to debugger.
 */
public class JDWPUnitDebuggeeWrapper extends JDWPUnitDebuggeeProcessWrapper {

    /**
     * Auxiliary options passed to the target VM on its launch.
     */
    public String savedVMOptions = null;

    /**
     * Wrapper around JDWP transport connection.
     */
    protected TransportWrapper transport;

    /**
     * JDWP transport address.
     */
    protected String address;

    /**
     * Is this a "listen" JDWP connection? (If false, it is a an "attach" connection.)
     */
    boolean isListenConnection;


    /**
     * Creates new instance with given data.
     *
     * @param settings
     *            test run options
     * @param logWriter
     *            where to print log messages
     */
    public JDWPUnitDebuggeeWrapper(JPDATestOptions settings, LogWriter logWriter) {
        super(settings, logWriter);
    }

    /**
     * Set up server side JDWP connection before launching the debuggee.
     */
    public void setUpConnection() {
        isListenConnection = settings.isListenConnectorKind();
        transport = createTransportWrapper();
        address = settings.getTransportAddress();

        if (isListenConnection) {
            logWriter.println("Start listening on: " + address);
            try {
                address = transport.startListening(address);
            } catch (IOException e) {
                throw new TestErrorException(e);
            }
            logWriter.println("Listening on: " + address);
        } else {
            logWriter.println("Attach to: " + address);
        }
    }

    /**
     * Launches new debuggee process according to test run options and
     * establishes JDWP connection.
     */
    @Override
    public void start() {
        String cmdLine = settings.getDebuggeeJavaPath() + " -cp \""
                + settings.getDebuggeeClassPath() + "\" "
                + settings.getDebuggeeAgentArgument()
                + settings.getDebuggeeAgentName() + "="
                + settings.getDebuggeeAgentOptions(address, isListenConnection)
                + " " + settings.getDebuggeeVMExtraOptions() + " "
                + (savedVMOptions != null ? savedVMOptions : "") + " "
                + settings.getDebuggeeClassName();

        logWriter.println("Launch: " + cmdLine);

        try {
            launchProcessAndRedirectors(cmdLine);
            logWriter.println("Launched debuggee process");
            openConnection();
            logWriter.println("Established transport connection");
        } catch (Exception e) {
            throw new TestErrorException(e);
        }
    }

    /**
     * Closes all connections, stops redirectors, and waits for debuggee process
     * exit for default timeout.
     */
    @Override
    public void stop() {
        disposeConnection();

        try {
            finishProcessAndRedirectors();
        } finally {
            // If the test has failed (e.g. a TestErrorException is
            // thrown), make sure that the transport server socket (if
            // any) is closed before leaving, as we may otherwise
            // block the transport port for subsequent JDWP tests.
            tearDownConnection();
        }
    }

    /**
     * Opens connection with debuggee.
     */
    protected void openConnection() {
        try {
            if (settings.isListenConnectorKind()) {
                logWriter.println("Accepting JDWP connection");
                transport.accept(settings.getTimeout(), settings.getTimeout());
            } else {
                String address = settings.getTransportAddress();
                logWriter.println("Attaching for JDWP connection");
                transport.attach(address, settings.getTimeout(), settings.getTimeout());
            }
            setConnection(transport);
        } catch (IOException e) {
            logWriter.printError(e);
            throw new TestErrorException(e);
        }
    }

    /**
     * Disposes JDWP connection stored in VmMirror.
     */
    protected void disposeConnection() {
        if (vmMirror != null) {
            try {
                vmMirror.dispose();
            } catch (Exception e) {
                logWriter.println("Ignoring exception in disposing debuggee VM: " + e);
            }
        }
    }

    /**
     * Closes JDWP connection stored in VmMirror.
     */
    protected void closeConnection() {
        if (vmMirror != null) {
            try {
                vmMirror.closeConnection();
            } catch (IOException e) {
                logWriter.println("Ignoring exception in closing connection: " + e);
            }
        }
    }

    /**
     * Closes JDWP connection and for listen connectors, stops listening to transport.
     */
    private void tearDownConnection() {
        closeConnection();
        if (settings.isListenConnectorKind()) {
            try {
                transport.stopListening();
            } catch (IOException e) {
                logWriter.println("IOException in transport listening stopping: " + e);
            }
        }
    }
}
