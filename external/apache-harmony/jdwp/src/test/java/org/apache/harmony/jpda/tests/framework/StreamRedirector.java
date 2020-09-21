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
package org.apache.harmony.jpda.tests.framework;

import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.IOException;
import java.io.Reader;

/**
 * <p>This class provides redirection of debuggee output and error streams to logWriter.
 */
public class StreamRedirector extends Thread {

    String name;
    LogWriter logWriter;
    Reader br;
    boolean doExit;

    /**
     * Creates new redirector for the specified stream.
     * 
     * @param is stream to be redirected
     * @param logWriter logWriter to redirect stream to
     * @param name stream name used as prefix for redirected output
     */
    public StreamRedirector(InputStream is, LogWriter logWriter, String name) {
        super("Redirector for " + name);
        this.name = name;
        this.logWriter = logWriter;
        br = new InputStreamReader(is);
        doExit = false;
    }

    /**
     * Reads all lines from stream and puts them to logWriter.
     */
    @Override
    public void run() {
        logWriter.println("Redirector started: " + name);
        try {
            StringBuilder cur = new StringBuilder();
            while (!doExit || br.ready()) {
                try {
                    int nc = br.read();
                    if (nc == -1) {
                        if (cur.length() != 0) {
                            logWriter.println(name + "> " + cur.toString());
                            cur.setLength(0);
                        }
                        break;
                    } else if (nc == (int)'\n') {
                        logWriter.println(name + "> " + cur.toString());
                        cur.setLength(0);
                    } else {
                        cur.appendCodePoint(nc);
                    }
                } catch (IllegalStateException e) {
                     //logWriter.printError("Illegal state exception! " + e);
                    //ignore
                }
            }
            if (cur.length() != 0) {
                logWriter.println(name + "> " + cur.toString());
                cur.setLength(0);
            }
            logWriter.println("Redirector completed: " + name);
        } catch (IOException e) {
            logWriter.printError(e);
        }
    }

    /**
     * Notifies redirector to stop stream redirection.
     */
    public void exit() {
        doExit = true;
    }
}
