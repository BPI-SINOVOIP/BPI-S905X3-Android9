/*
 * Copyright (C) 2011 The Guava Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.common.net;

import static com.google.common.base.Charsets.UTF_8;
import static com.google.common.net.MediaType.ANY_APPLICATION_TYPE;
import static com.google.common.net.MediaType.ANY_AUDIO_TYPE;
import static com.google.common.net.MediaType.ANY_IMAGE_TYPE;
import static com.google.common.net.MediaType.ANY_TEXT_TYPE;
import static com.google.common.net.MediaType.ANY_TYPE;
import static com.google.common.net.MediaType.ANY_VIDEO_TYPE;
import static com.google.common.net.MediaType.HTML_UTF_8;
import static com.google.common.net.MediaType.JPEG;
import static com.google.common.net.MediaType.PLAIN_TEXT_UTF_8;

import com.google.common.annotations.Beta;
import com.google.common.annotations.GwtCompatible;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.testing.EqualsTester;

import junit.framework.TestCase;

import java.nio.charset.IllegalCharsetNameException;
import java.nio.charset.UnsupportedCharsetException;

/**
 * Tests for {@link MediaType}.
 *
 * @author Gregory Kick
 */
@Beta
@GwtCompatible(emulated = true)
public class MediaTypeTest extends TestCase {

  public void testCreate_invalidType() {
    try {
      MediaType.create("te><t", "plaintext");
      fail();
    } catch (IllegalArgumentException expected) {}
  }

  public void testCreate_invalidSubtype() {
    try {
      MediaType.create("text", "pl@intext");
      fail();
    } catch (IllegalArgumentException expected) {}
  }

  public void testCreate_wildcardTypeDeclaredSubtype() {
    try {
      MediaType.create("*", "text");
      fail();
    } catch (IllegalArgumentException expected) {}
  }

  public void testCreateApplicationType() {
    MediaType newType = MediaType.createApplicationType("yams");
    assertEquals("application", newType.type());
    assertEquals("yams", newType.subtype());
  }

  public void testCreateAudioType() {
    MediaType newType = MediaType.createAudioType("yams");
    assertEquals("audio", newType.type());
    assertEquals("yams", newType.subtype());
  }

  public void testCreateImageType() {
    MediaType newType = MediaType.createImageType("yams");
    assertEquals("image", newType.type());
    assertEquals("yams", newType.subtype());
  }

  public void testCreateTextType() {
    MediaType newType = MediaType.createTextType("yams");
    assertEquals("text", newType.type());
    assertEquals("yams", newType.subtype());
  }

  public void testCreateVideoType() {
    MediaType newType = MediaType.createVideoType("yams");
    assertEquals("video", newType.type());
    assertEquals("yams", newType.subtype());
  }

  public void testGetType() {
    assertEquals("text", MediaType.parse("text/plain").type());
    assertEquals("application",
        MediaType.parse("application/atom+xml; charset=utf-8").type());
  }

  public void testGetSubtype() {
    assertEquals("plain", MediaType.parse("text/plain").subtype());
    assertEquals("atom+xml",
        MediaType.parse("application/atom+xml; charset=utf-8").subtype());
  }

  private static final ImmutableListMultimap<String, String> PARAMETERS =
      ImmutableListMultimap.of("a", "1", "a", "2", "b", "3");

  public void testGetParameters() {
    assertEquals(ImmutableListMultimap.of(), MediaType.parse("text/plain").parameters());
    assertEquals(ImmutableListMultimap.of("charset", "utf-8"),
        MediaType.parse("application/atom+xml; charset=utf-8").parameters());
    assertEquals(PARAMETERS,
        MediaType.parse("application/atom+xml; a=1; a=2; b=3").parameters());
  }

  public void testWithoutParameters() {
    assertSame(MediaType.parse("image/gif"),
        MediaType.parse("image/gif").withoutParameters());
    assertEquals(MediaType.parse("image/gif"),
        MediaType.parse("image/gif; foo=bar").withoutParameters());
  }

  public void testWithParameters() {
    assertEquals(MediaType.parse("text/plain; a=1; a=2; b=3"),
        MediaType.parse("text/plain").withParameters(PARAMETERS));
    assertEquals(MediaType.parse("text/plain; a=1; a=2; b=3"),
        MediaType.parse("text/plain; a=1; a=2; b=3").withParameters(PARAMETERS));
  }

  public void testWithParameters_invalidAttribute() {
    MediaType mediaType = MediaType.parse("text/plain");
    ImmutableListMultimap<String, String> parameters =
        ImmutableListMultimap.of("a", "1", "@", "2", "b", "3");
    try {
      mediaType.withParameters(parameters);
      fail();
    } catch (IllegalArgumentException expected) {}
  }

  public void testWithParameter() {
    assertEquals(MediaType.parse("text/plain; a=1"),
        MediaType.parse("text/plain").withParameter("a", "1"));
    assertEquals(MediaType.parse("text/plain; a=1"),
        MediaType.parse("text/plain; a=1; a=2").withParameter("a", "1"));
    assertEquals(MediaType.parse("text/plain; a=3"),
        MediaType.parse("text/plain; a=1; a=2").withParameter("a", "3"));
    assertEquals(MediaType.parse("text/plain; a=1; a=2; b=3"),
        MediaType.parse("text/plain; a=1; a=2").withParameter("b", "3"));
  }

  public void testWithParameter_invalidAttribute() {
    MediaType mediaType = MediaType.parse("text/plain");
    try {
      mediaType.withParameter("@", "2");
      fail();
    } catch (IllegalArgumentException expected) {}
  }

  public void testWithCharset() {
    assertEquals(MediaType.parse("text/plain; charset=utf-8"),
        MediaType.parse("text/plain").withCharset(UTF_8));
    assertEquals(MediaType.parse("text/plain; charset=utf-8"),
        MediaType.parse("text/plain; charset=utf-16").withCharset(UTF_8));
  }

  public void testHasWildcard() {
    assertFalse(PLAIN_TEXT_UTF_8.hasWildcard());
    assertFalse(JPEG.hasWildcard());
    assertTrue(ANY_TYPE.hasWildcard());
    assertTrue(ANY_APPLICATION_TYPE.hasWildcard());
    assertTrue(ANY_AUDIO_TYPE.hasWildcard());
    assertTrue(ANY_IMAGE_TYPE.hasWildcard());
    assertTrue(ANY_TEXT_TYPE.hasWildcard());
    assertTrue(ANY_VIDEO_TYPE.hasWildcard());
  }

  public void testIs() {
    assertTrue(PLAIN_TEXT_UTF_8.is(ANY_TYPE));
    assertTrue(JPEG.is(ANY_TYPE));
    assertTrue(ANY_TEXT_TYPE.is(ANY_TYPE));
    assertTrue(PLAIN_TEXT_UTF_8.is(ANY_TEXT_TYPE));
    assertTrue(PLAIN_TEXT_UTF_8.withoutParameters().is(ANY_TEXT_TYPE));
    assertFalse(JPEG.is(ANY_TEXT_TYPE));
    assertTrue(PLAIN_TEXT_UTF_8.is(PLAIN_TEXT_UTF_8));
    assertTrue(PLAIN_TEXT_UTF_8.is(PLAIN_TEXT_UTF_8.withoutParameters()));
    assertFalse(PLAIN_TEXT_UTF_8.withoutParameters().is(PLAIN_TEXT_UTF_8));
    assertFalse(PLAIN_TEXT_UTF_8.is(HTML_UTF_8));
    assertFalse(PLAIN_TEXT_UTF_8.withParameter("charset", "UTF-16").is(PLAIN_TEXT_UTF_8));
    assertFalse(PLAIN_TEXT_UTF_8.is(PLAIN_TEXT_UTF_8.withParameter("charset", "UTF-16")));
  }

  public void testParse_empty() {
    try {
      MediaType.parse("");
      fail();
    } catch (IllegalArgumentException expected) {}
  }

  public void testParse_badInput() {
    try {
      MediaType.parse("/");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("te<t/plain");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/pl@in");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain;");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; ");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; a");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; a=");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; a=@");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; a=\"@");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; a=1;");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; a=1; ");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; a=1; b");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; a=1; b=");
      fail();
    } catch (IllegalArgumentException expected) {}
    try {
      MediaType.parse("text/plain; a=\u2025");
      fail();
    } catch (IllegalArgumentException expected) {}
  }

  public void testGetCharset() {
    assertEquals(Optional.absent(), MediaType.parse("text/plain").charset());
    assertEquals(Optional.of(UTF_8),
        MediaType.parse("text/plain; charset=utf-8").charset());
  }

  public void testGetCharset_tooMany() {
    MediaType mediaType = MediaType.parse("text/plain; charset=utf-8; charset=utf-16");
    try {
      mediaType.charset();
      fail();
    } catch (IllegalStateException expected) {}
  }

  public void testGetCharset_illegalCharset() {
    MediaType mediaType = MediaType.parse(
        "text/plain; charset=\"!@#$%^&*()\"");
    try {
      mediaType.charset();
      fail();
    } catch (IllegalCharsetNameException expected) {}
  }

  public void testGetCharset_unsupportedCharset() {
    MediaType mediaType = MediaType.parse(
        "text/plain; charset=utf-wtf");
    try {
      mediaType.charset();
      fail();
    } catch (UnsupportedCharsetException expected) {}
  }

  public void testEquals() {
    new EqualsTester()
        .addEqualityGroup(MediaType.create("text", "plain"),
            MediaType.create("TEXT", "PLAIN"),
            MediaType.parse("text/plain"),
            MediaType.parse("TEXT/PLAIN"),
            MediaType.create("text", "plain").withParameter("a", "1").withoutParameters())
        .addEqualityGroup(
            MediaType.create("text", "plain").withCharset(UTF_8),
            MediaType.create("text", "plain").withParameter("CHARSET", "UTF-8"),
            MediaType.create("text", "plain").withParameters(
                ImmutableMultimap.of("charset", "utf-8")),
            MediaType.parse("text/plain;charset=utf-8"),
            MediaType.parse("text/plain; charset=utf-8"),
            MediaType.parse("text/plain;  charset=utf-8"),
            MediaType.parse("text/plain; \tcharset=utf-8"),
            MediaType.parse("text/plain; \r\n\tcharset=utf-8"),
            MediaType.parse("text/plain; CHARSET=utf-8"),
            MediaType.parse("text/plain; charset=\"utf-8\""),
            MediaType.parse("text/plain; charset=\"\\u\\tf-\\8\""),
            MediaType.parse("text/plain; charset=UTF-8"))
        .addEqualityGroup(MediaType.parse("text/plain; charset=utf-8; charset=utf-8"))
        .addEqualityGroup(MediaType.create("text", "plain").withParameter("a", "value"),
            MediaType.create("text", "plain").withParameter("A", "value"))
        .addEqualityGroup(MediaType.create("text", "plain").withParameter("a", "VALUE"),
            MediaType.create("text", "plain").withParameter("A", "VALUE"))
        .addEqualityGroup(
            MediaType.create("text", "plain")
                .withParameters(ImmutableListMultimap.of("a", "1", "a", "2")),
            MediaType.create("text", "plain")
                .withParameters(ImmutableListMultimap.of("a", "2", "a", "1")))
        .addEqualityGroup(MediaType.create("text", "csv"))
        .addEqualityGroup(MediaType.create("application", "atom+xml"))
        .testEquals();
  }

  public void testToString() {
    assertEquals("text/plain", MediaType.create("text", "plain").toString());
    assertEquals("text/plain; something=\"cr@zy\"; something-else=\"crazy with spaces\"",
        MediaType.create("text", "plain")
            .withParameter("something", "cr@zy")
            .withParameter("something-else", "crazy with spaces")
            .toString());
  }
}

