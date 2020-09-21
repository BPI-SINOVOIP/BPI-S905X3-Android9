/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.location.cts.asn1.supl2.ulp_version_2_parameter_extensions;

/*
 */


//
//
import android.location.cts.asn1.base.Asn1Null;
import android.location.cts.asn1.base.Asn1SequenceOf;
import android.location.cts.asn1.base.Asn1Tag;
import android.location.cts.asn1.base.BitStream;
import android.location.cts.asn1.base.BitStreamReader;
import com.google.common.collect.ImmutableList;
import java.util.Collection;
import javax.annotation.Nullable;


/**
 */
public  class GanssRequestedGenericAssistanceDataList
    extends Asn1SequenceOf<GanssReqGenericData> {
  //

  private static final Asn1Tag TAG_GanssRequestedGenericAssistanceDataList
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public GanssRequestedGenericAssistanceDataList() {
    super();
    setMinSize(1);
setMaxSize(16);

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_GanssRequestedGenericAssistanceDataList;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_GanssRequestedGenericAssistanceDataList != null) {
      return ImmutableList.of(TAG_GanssRequestedGenericAssistanceDataList);
    } else {
      return Asn1SequenceOf.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new GanssRequestedGenericAssistanceDataList from encoded stream.
   */
  public static GanssRequestedGenericAssistanceDataList fromPerUnaligned(byte[] encodedBytes) {
    GanssRequestedGenericAssistanceDataList result = new GanssRequestedGenericAssistanceDataList();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new GanssRequestedGenericAssistanceDataList from encoded stream.
   */
  public static GanssRequestedGenericAssistanceDataList fromPerAligned(byte[] encodedBytes) {
    GanssRequestedGenericAssistanceDataList result = new GanssRequestedGenericAssistanceDataList();
    result.decodePerAligned(new BitStreamReader(encodedBytes));
    return result;
  }

  
  @Override public GanssReqGenericData createAndAddValue() {
    GanssReqGenericData value = new GanssReqGenericData();
    add(value);
    return value;
  }

  

  @Override public Iterable<BitStream> encodePerUnaligned() {
    return super.encodePerUnaligned();
  }

  @Override public Iterable<BitStream> encodePerAligned() {
    return super.encodePerAligned();
  }

  @Override public void decodePerUnaligned(BitStreamReader reader) {
    super.decodePerUnaligned(reader);
  }

  @Override public void decodePerAligned(BitStreamReader reader) {
    super.decodePerAligned(reader);
  }

  @Override public String toString() {
    return toIndentedString("");
  }

  public String toIndentedString(String indent) {
    StringBuilder builder = new StringBuilder();
    builder.append("GanssRequestedGenericAssistanceDataList = [\n");
    final String internalIndent = indent + "  ";
    for (GanssReqGenericData value : getValues()) {
      builder.append(internalIndent)
          .append(value.toIndentedString(internalIndent));
    }
    builder.append(indent).append("];\n");
    return builder.toString();
  }
}
