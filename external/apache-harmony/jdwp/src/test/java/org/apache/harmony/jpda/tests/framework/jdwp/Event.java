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
 * @author Aleksey V. Yantsen
 */

/**
 * Created on 11.29.2004
 */
package org.apache.harmony.jpda.tests.framework.jdwp;

import java.util.Collections;
import java.util.List;

/**
 * This class provides description of event request. 
 */
public class Event {

    public static EventBuilder builder(byte eventKind, byte suspendPolicy) {
        return new EventBuilder(eventKind, suspendPolicy);
    }

    public final byte eventKind;
    public final byte suspendPolicy;

    /** List of event modifiers. */
    public final List<EventMod> mods;

    /**
     * Create new instance with specified data.
     */
    Event(byte eventKind, byte suspendPolicy, List<EventMod> mods) {
        this.eventKind = eventKind;
        this.suspendPolicy = suspendPolicy;
        this.mods = Collections.unmodifiableList(mods);
    }
}
