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
package org.mockftpserver.fake.example;

import org.mockftpserver.fake.FakeFtpServer;
import org.mockftpserver.fake.UserAccount;
import org.mockftpserver.fake.filesystem.FileEntry;
import org.mockftpserver.fake.filesystem.FileSystem;
import org.mockftpserver.fake.filesystem.UnixFakeFileSystem;
import org.mockftpserver.stub.example.RemoteFile;
import org.mockftpserver.test.*;
import org.mockftpserver.test.AbstractTestCase;

import java.io.IOException;

/**
 * Example test using FakeFtpServer, with programmatic configuration.
 */
public class RemoteFileTest extends AbstractTestCase implements IntegrationTest {

    private static final String HOME_DIR = "/";
    private static final String FILE = "/dir/sample.txt";
    private static final String CONTENTS = "abcdef 1234567890";

    private RemoteFile remoteFile;
    private FakeFtpServer fakeFtpServer;

    public void testReadFile() throws Exception {
        String contents = remoteFile.readFile(FILE);
        assertEquals("contents", CONTENTS, contents);
    }

    public void testReadFileThrowsException() {
        try {
            remoteFile.readFile("NoSuchFile.txt");
            fail("Expected IOException");
        }
        catch (IOException expected) {
            // Expected this
        }
    }

    protected void setUp() throws Exception {
        super.setUp();
        fakeFtpServer = new FakeFtpServer();
        fakeFtpServer.setServerControlPort(0);  // use any free port

        FileSystem fileSystem = new UnixFakeFileSystem();
        fileSystem.add(new FileEntry(FILE, CONTENTS));
        fakeFtpServer.setFileSystem(fileSystem);

        UserAccount userAccount = new UserAccount(RemoteFile.USERNAME, RemoteFile.PASSWORD, HOME_DIR);
        fakeFtpServer.addUserAccount(userAccount);

        fakeFtpServer.start();
        int port = fakeFtpServer.getServerControlPort();

        remoteFile = new RemoteFile();
        remoteFile.setServer("localhost");
        remoteFile.setPort(port);
    }

    protected void tearDown() throws Exception {
        super.tearDown();
        fakeFtpServer.stop();
    }

}