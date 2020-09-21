/*
 * Copyright (C) 2014 Square, Inc.
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
package okio;

import java.io.EOFException;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

@RunWith(Parameterized.class)
public final class ReadUtf8LineTest {
  private interface Factory {
    BufferedSource create(Buffer data);
  }

  // ANDROID-BEGIN
  //  @Parameterized.Parameters(name = "{0}")
  @Parameterized.Parameters
  // ANDROID-END
  public static List<Object[]> parameters() {
    return Arrays.asList(
        new Object[] { new Factory() {
          @Override public BufferedSource create(Buffer data) {
            return data;
          }

          @Override public String toString() {
            return "Buffer";
          }
        }},
        new Object[] { new Factory() {
          @Override public BufferedSource create(Buffer data) {
            return new RealBufferedSource(data);
          }

          @Override public String toString() {
            return "RealBufferedSource";
          }
        }},
        new Object[] { new Factory() {
          @Override public BufferedSource create(Buffer data) {
            return new RealBufferedSource(new ForwardingSource(data) {
              @Override public long read(Buffer sink, long byteCount) throws IOException {
                return super.read(sink, Math.min(1, byteCount));
              }
            });
          }

          @Override public String toString() {
            return "Slow RealBufferedSource";
          }
        }}
    );
  }

  // ANDROID-BEGIN
  //  @Parameterized.Parameter
  private final Factory factory;
  public ReadUtf8LineTest(Factory factory) {
    this.factory = factory;
  }
  // ANDROID-END

  private Buffer data;
  private BufferedSource source;

  @Before public void setUp() {
    data = new Buffer();
    source = factory.create(data);
  }

  @Test public void readLines() throws IOException {
    data.writeUtf8("abc\ndef\n");
    assertEquals("abc", source.readUtf8LineStrict());
    assertEquals("def", source.readUtf8LineStrict());
    try {
      source.readUtf8LineStrict();
      fail();
    } catch (EOFException expected) {
      assertEquals("\\n not found: size=0 content=...", expected.getMessage());
    }
  }

  @Test public void eofExceptionProvidesLimitedContent() throws IOException {
    data.writeUtf8("aaaaaaaabbbbbbbbccccccccdddddddde");
    try {
      source.readUtf8LineStrict();
      fail();
    } catch (EOFException expected) {
      assertEquals("\\n not found: size=33 content=616161616161616162626262626262626363636363636363"
          + "6464646464646464...", expected.getMessage());
    }
  }

  @Test public void emptyLines() throws IOException {
    data.writeUtf8("\n\n\n");
    assertEquals("", source.readUtf8LineStrict());
    assertEquals("", source.readUtf8LineStrict());
    assertEquals("", source.readUtf8LineStrict());
    assertTrue(source.exhausted());
  }

  @Test public void crDroppedPrecedingLf() throws IOException {
    data.writeUtf8("abc\r\ndef\r\nghi\rjkl\r\n");
    assertEquals("abc", source.readUtf8LineStrict());
    assertEquals("def", source.readUtf8LineStrict());
    assertEquals("ghi\rjkl", source.readUtf8LineStrict());
  }

  @Test public void bufferedReaderCompatible() throws IOException {
    data.writeUtf8("abc\ndef");
    assertEquals("abc", source.readUtf8Line());
    assertEquals("def", source.readUtf8Line());
    assertEquals(null, source.readUtf8Line());
  }

  @Test public void bufferedReaderCompatibleWithTrailingNewline() throws IOException {
    data.writeUtf8("abc\ndef\n");
    assertEquals("abc", source.readUtf8Line());
    assertEquals("def", source.readUtf8Line());
    assertEquals(null, source.readUtf8Line());
  }
}
