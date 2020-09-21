/* Licensed to the Apache Software Foundation (ASF) under one or more
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
package org.apache.harmony.tests.java.nio;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.nio.DirectByteBuffer;
import java.nio.NIOAccess;
import libcore.io.SizeOf;

public class DirectCharBufferTest extends CharBufferTest {
    
    public void setUp(){
        buf = ByteBuffer.allocateDirect(BUFFER_LENGTH*2).asCharBuffer();
        super.loadTestData1(buf);
        baseBuf = buf;
    }
    
    public void tearDown(){
        buf = null;
        baseBuf = null;
    }
    
    public void testHasArray() {
        assertFalse(buf.hasArray());
    }

    public void testArray() {
        try {
            buf.array();
            fail("Should throw UnsupportedOperationException"); //$NON-NLS-1$
        } catch (UnsupportedOperationException e) {
        }
    }
    
    public void testArrayOffset() {
        try {
            buf.arrayOffset();
            fail("Should throw UnsupportedOperationException"); //$NON-NLS-1$
        } catch (UnsupportedOperationException e) {
        }
    }

    // http://b/28964300
    public void testJNIAccessByAddress() throws Exception {
        DirectByteBuffer directByteBuffer = (DirectByteBuffer) ByteBuffer.allocateDirect(10);
        directByteBuffer.put((byte)'a');
        CharBuffer charBuffer = directByteBuffer.asCharBuffer();
        long byteBufferBasePointer = NIOAccess.getBasePointer(directByteBuffer);
        long charBufferBasePointer = NIOAccess.getBasePointer(charBuffer);
        assertEquals(byteBufferBasePointer, charBufferBasePointer);

        // Check if the NIOAccess method adds up the current position value.
        charBuffer.put('b');
        assertEquals(charBufferBasePointer + SizeOf.CHAR, NIOAccess.getBasePointer(charBuffer));
    }

    public void testIsDirect() {
        assertTrue(buf.isDirect());
    }
    
    public void testOrder() {
        assertEquals(ByteOrder.BIG_ENDIAN, buf.order());
    }
}
