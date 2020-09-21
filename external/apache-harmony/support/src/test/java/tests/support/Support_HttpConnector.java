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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package tests.support;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * This interface contains the basic methods necessary to open and use a
 * connection to an HTTP server.
 */
public interface Support_HttpConnector {

    public void open(String address) throws IOException;

    public void close() throws IOException;

    public InputStream getInputStream() throws IOException;

    public OutputStream getOutputStream() throws IOException;

    public boolean isChunkedOnFlush();

    public void setRequestProperty(String key, String value) throws IOException;

    public String getHeaderField(int index) throws IOException;

    public String getHeaderFieldKey(int index) throws IOException;

}
