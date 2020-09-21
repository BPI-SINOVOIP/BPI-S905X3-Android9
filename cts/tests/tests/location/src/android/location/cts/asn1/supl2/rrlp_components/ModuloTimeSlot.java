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
import android.location.cts.asn1.base.Asn1Integer;
import android.location.cts.asn1.base.Asn1Tag;
import android.location.cts.asn1.base.BitStream;
import android.location.cts.asn1.base.BitStreamReader;
import com.google.common.collect.ImmutableList;
import java.util.Collection;
import javax.annotation.Nullable;


/**
 */
public  class ModuloTimeSlot extends Asn1Integer {
  //

  private static final Asn1Tag TAG_ModuloTimeSlot
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public ModuloTimeSlot() {
    super();
    setValueRange("0", "3");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_ModuloTimeSlot;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_ModuloTimeSlot != null) {
      return ImmutableList.of(TAG_ModuloTimeSlot);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new ModuloTimeSlot from encoded stream.
   */
  public static ModuloTimeSlot fromPerUnaligned(byte[] encodedBytes) {
    ModuloTimeSlot result = new ModuloTimeSlot();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new ModuloTimeSlot from encoded stream.
   */
  public static ModuloTimeSlot fromPerAligned(byte[] encodedBytes) {
    ModuloTimeSlot result = new ModuloTimeSlot();
    result.decodePerAligned(new BitStreamReader(encodedBytes));
    return result;
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
    return "ModuloTimeSlot = " + getInteger() + ";\n";
  }
}
