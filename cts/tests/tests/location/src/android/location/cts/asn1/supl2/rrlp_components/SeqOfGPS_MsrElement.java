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

package android.location.cts.asn1.supl2.rrlp_components;

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
public  class SeqOfGPS_MsrElement
    extends Asn1SequenceOf<GPS_MsrElement> {
  //

  private static final Asn1Tag TAG_SeqOfGPS_MsrElement
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public SeqOfGPS_MsrElement() {
    super();
    setMinSize(1);
setMaxSize(16);

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_SeqOfGPS_MsrElement;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_SeqOfGPS_MsrElement != null) {
      return ImmutableList.of(TAG_SeqOfGPS_MsrElement);
    } else {
      return Asn1SequenceOf.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new SeqOfGPS_MsrElement from encoded stream.
   */
  public static SeqOfGPS_MsrElement fromPerUnaligned(byte[] encodedBytes) {
    SeqOfGPS_MsrElement result = new SeqOfGPS_MsrElement();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new SeqOfGPS_MsrElement from encoded stream.
   */
  public static SeqOfGPS_MsrElement fromPerAligned(byte[] encodedBytes) {
    SeqOfGPS_MsrElement result = new SeqOfGPS_MsrElement();
    result.decodePerAligned(new BitStreamReader(encodedBytes));
    return result;
  }

  
  @Override public GPS_MsrElement createAndAddValue() {
    GPS_MsrElement value = new GPS_MsrElement();
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
    builder.append("SeqOfGPS_MsrElement = [\n");
    final String internalIndent = indent + "  ";
    for (GPS_MsrElement value : getValues()) {
      builder.append(internalIndent)
          .append(value.toIndentedString(internalIndent));
    }
    builder.append(indent).append("];\n");
    return builder.toString();
  }
}
