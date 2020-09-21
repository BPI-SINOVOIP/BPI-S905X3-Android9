/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.voicemail.impl.imap;

/** The payload for a voicemail, usually audio data. */
public class VoicemailPayload {
  private final String mimeType;
  private final byte[] bytes;

  public VoicemailPayload(String mimeType, byte[] bytes) {
    this.mimeType = mimeType;
    this.bytes = bytes;
  }

  public byte[] getBytes() {
    return bytes;
  }

  public String getMimeType() {
    return mimeType;
  }
}
