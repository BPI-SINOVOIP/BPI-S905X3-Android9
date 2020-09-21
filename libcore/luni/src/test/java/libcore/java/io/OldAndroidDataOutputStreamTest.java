/*
 * Copyright (C) 2008 The Android Open Source Project
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

package libcore.java.io;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import junit.framework.TestCase;

/**
 * Basic tests for DataOutputStreams.
 */
public class OldAndroidDataOutputStreamTest extends TestCase {

    public void testDataOutputStream() throws Exception {
        String str = "AbCdEfGhIjKlMnOpQrStUvWxYz";
        ByteArrayOutputStream aa = new ByteArrayOutputStream();
        DataOutputStream a = new DataOutputStream(aa);

        try {
            a.write(str.getBytes(), 0, 26);
            a.write('A');

            assertEquals(27, aa.size());
            assertEquals("AbCdEfGhIjKlMnOpQrStUvWxYzA", aa.toString());

            a.writeByte('B');
            assertEquals("AbCdEfGhIjKlMnOpQrStUvWxYzAB", aa.toString());
            a.writeBytes("BYTES");
            assertEquals("AbCdEfGhIjKlMnOpQrStUvWxYzABBYTES", aa.toString());
        } finally {
            a.close();
        }
    }
}
