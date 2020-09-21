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
package org.mockftpserver.fake.filesystem

import java.io.IOException
import java.io.OutputStream
import org.mockftpserver.core.util.IoUtilimport org.apache.log4j.Logger

/**
 * Tests for FileEntry
 * 
 * @version $Revision: $ - $Date: $
 *
 * @author Chris Mair
 */
public class FileEntryTest extends AbstractFileSystemEntryTest {

    private static final LOG = Logger.getLogger(FileEntryTest)
    private static final CONTENTS = "abc 123 %^& xxx"
    
    private FileEntry entry

    /**
     * Test the FileEntry(String,String) constructor
     */
    void testConstructorWithStringContents() {
        entry = new FileEntry(PATH, CONTENTS)
        verifyContents(CONTENTS)
    }
    
    /**
     * Test setting the contents of a FileEntry by setting a String
     */
    void testSettingContentsFromString() {
        entry.setContents(CONTENTS)
        verifyContents(CONTENTS)
    }

    /**
     * Test setting the contents of a FileEntry by setting a byte[]
     */
    void testSettingContentsFromBytes() {
        byte[] contents = CONTENTS.getBytes()
        entry.setContents(contents)
        // Now corrupt the original byte array to make sure the file entry is not affected
        contents[1] = (byte)'#'
        verifyContents(CONTENTS)
    }
    
    /**
     * Test the setContents(String) method, passing in null 
     */
    void testSetContents_NullString() {
        shouldFailWithMessageContaining("contents") { entry.setContents((String)null) }
    }
    
    /**
     * Test the setContents(byte[]) method, passing in null 
     */
    void testSetContents_NullBytes() {
        shouldFailWithMessageContaining("contents") { entry.setContents((byte[])null) }
    }
    
    /**
     * Test the createOutputStream() method
     */
    void testCreateOutputStream() {
        // New, empty file
        OutputStream out = entry.createOutputStream(false)
        out.write(CONTENTS.getBytes())
        verifyContents(CONTENTS)
        
        // Another OutputStream, append=false
        out = entry.createOutputStream(false)
        out.write(CONTENTS.getBytes())
        verifyContents(CONTENTS)
        
        // Another OutputStream, append=true
        out = entry.createOutputStream(true)
        out.write(CONTENTS.getBytes())
        verifyContents(CONTENTS + CONTENTS)

        // Set contents directly
        final String NEW_CONTENTS = ",./'\t\r[]-\n="
        entry.setContents(NEW_CONTENTS)
        verifyContents(NEW_CONTENTS)
        
        // New OutputStream, append=true (so should append to contents we set directly)
        out = entry.createOutputStream(true)
        out.write(CONTENTS.getBytes())
        verifyContents(NEW_CONTENTS + CONTENTS)

        // Yet another OutputStream, append=true (so should append to accumulated contents)
        OutputStream out2 = entry.createOutputStream(true)
        out2.write(CONTENTS.getBytes())
        out2.close()       // should have no effect
        verifyContents(NEW_CONTENTS + CONTENTS + CONTENTS)
        
        // Write with the previous OutputStream (simulate 2 OututStreams writing "concurrently")
        out.write(NEW_CONTENTS.getBytes())
        verifyContents(NEW_CONTENTS + CONTENTS + CONTENTS + NEW_CONTENTS)
    }
    
    /**
     * Test the createInputStream() method when the file entry contents have not been initialized
     */
    void testCreateInputStream_NullContents() {
        verifyContents("")
    }
    
    //-------------------------------------------------------------------------
    // Implementation of Required Abstract Methods
    //-------------------------------------------------------------------------
    
    /**
     * @see org.mockftpserver.fake.filesystem.AbstractFileSystemEntryTest#getImplementationClass()
     */
    protected Class getImplementationClass() {
        return FileEntry.class
    }

    /**
     * @see org.mockftpserver.fake.filesystem.AbstractFileSystemEntryTest#isDirectory()
     */
    protected boolean isDirectory() {
        return false
    }

    //-------------------------------------------------------------------------
    // Test setup
    //-------------------------------------------------------------------------

    /**
     * @see org.mockftpserver.test.AbstractTest#setUp()
     */
    void setUp() {
        super.setUp()
        entry = new FileEntry(PATH)
    }

    //-------------------------------------------------------------------------
    // Internal Helper Methods
    //-------------------------------------------------------------------------
    
    /**
     * Verify the expected contents of the file entry, read from its InputSteam
     * @param expectedContents - the expected contents
     * @throws IOException
     */
    private void verifyContents(String expectedContents) {
        byte[] bytes = IoUtil.readBytes(entry.createInputStream())
        LOG.info("bytes=[" + new String(bytes) + "]")
        assertEquals("contents: actual=[" + new String(bytes) + "]", expectedContents.getBytes(), bytes)
        assert entry.getSize() == expectedContents.length()
    }
    
}
