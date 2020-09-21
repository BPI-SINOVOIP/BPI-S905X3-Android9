/*
 * Copyright 2008 the original author or authors.
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
package org.mockftpserver.fake

import org.mockftpserver.core.server.AbstractFtpServer
import org.mockftpserver.core.server.AbstractFtpServer_StartTest
import org.mockftpserver.fake.FakeFtpServer



/**
 * Tests for FakeFtpServer that require the server thread to be started.
 *
 * @version $Revision: 54 $ - $Date: 2008-05-13 21:54:53 -0400 (Tue, 13 May 2008) $
 *
 * @author Chris Mair
 */
class FakeFtpServer_StartTest extends AbstractFtpServer_StartTest {

    protected AbstractFtpServer createFtpServer() {
        return new FakeFtpServer();
    }

}
