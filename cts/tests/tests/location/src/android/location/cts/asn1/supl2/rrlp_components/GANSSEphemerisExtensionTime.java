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
import android.location.cts.asn1.base.Asn1Null;
import android.location.cts.asn1.base.Asn1Object;
import android.location.cts.asn1.base.Asn1Sequence;
import android.location.cts.asn1.base.Asn1Tag;
import android.location.cts.asn1.base.BitStream;
import android.location.cts.asn1.base.BitStreamReader;
import android.location.cts.asn1.base.SequenceComponent;
import com.google.common.collect.ImmutableList;
import java.util.Collection;
import javax.annotation.Nullable;


/**
*/
public  class GANSSEphemerisExtensionTime extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_GANSSEphemerisExtensionTime
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public GANSSEphemerisExtensionTime() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_GANSSEphemerisExtensionTime;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_GANSSEphemerisExtensionTime != null) {
      return ImmutableList.of(TAG_GANSSEphemerisExtensionTime);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new GANSSEphemerisExtensionTime from encoded stream.
   */
  public static GANSSEphemerisExtensionTime fromPerUnaligned(byte[] encodedBytes) {
    GANSSEphemerisExtensionTime result = new GANSSEphemerisExtensionTime();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new GANSSEphemerisExtensionTime from encoded stream.
   */
  public static GANSSEphemerisExtensionTime fromPerAligned(byte[] encodedBytes) {
    GANSSEphemerisExtensionTime result = new GANSSEphemerisExtensionTime();
    result.decodePerAligned(new BitStreamReader(encodedBytes));
    return result;
  }



  @Override protected boolean isExtensible() {
    return true;
  }

  @Override public boolean containsExtensionValues() {
    for (SequenceComponent extensionComponent : getExtensionComponents()) {
      if (extensionComponent.isExplicitlySet()) return true;
    }
    return false;
  }

  
  private GANSSEphemerisExtensionTime.ganssEphExtDayType ganssEphExtDay_;
  public GANSSEphemerisExtensionTime.ganssEphExtDayType getGanssEphExtDay() {
    return ganssEphExtDay_;
  }
  /**
   * @throws ClassCastException if value is not a GANSSEphemerisExtensionTime.ganssEphExtDayType
   */
  public void setGanssEphExtDay(Asn1Object value) {
    this.ganssEphExtDay_ = (GANSSEphemerisExtensionTime.ganssEphExtDayType) value;
  }
  public GANSSEphemerisExtensionTime.ganssEphExtDayType setGanssEphExtDayToNewInstance() {
    ganssEphExtDay_ = new GANSSEphemerisExtensionTime.ganssEphExtDayType();
    return ganssEphExtDay_;
  }
  
  private GANSSTOD ganssEphExtTOD_;
  public GANSSTOD getGanssEphExtTOD() {
    return ganssEphExtTOD_;
  }
  /**
   * @throws ClassCastException if value is not a GANSSTOD
   */
  public void setGanssEphExtTOD(Asn1Object value) {
    this.ganssEphExtTOD_ = (GANSSTOD) value;
  }
  public GANSSTOD setGanssEphExtTODToNewInstance() {
    ganssEphExtTOD_ = new GANSSTOD();
    return ganssEphExtTOD_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getGanssEphExtDay() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGanssEphExtDay();
          }

          @Override public void setToNewInstance() {
            setGanssEphExtDayToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GANSSEphemerisExtensionTime.ganssEphExtDayType.getPossibleFirstTags() : ImmutableList.of(tag);
          }

          @Override
          public Asn1Tag getTag() {
            return tag;
          }

          @Override
          public boolean isImplicitTagging() {
            return true;
          }

          @Override public String toIndentedString(String indent) {
                return "ganssEphExtDay : "
                    + getGanssEphExtDay().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getGanssEphExtTOD() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGanssEphExtTOD();
          }

          @Override public void setToNewInstance() {
            setGanssEphExtTODToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GANSSTOD.getPossibleFirstTags() : ImmutableList.of(tag);
          }

          @Override
          public Asn1Tag getTag() {
            return tag;
          }

          @Override
          public boolean isImplicitTagging() {
            return true;
          }

          @Override public String toIndentedString(String indent) {
                return "ganssEphExtTOD : "
                    + getGanssEphExtTOD().toIndentedString(indent);
              }
        });
    
    return builder.build();
  }

  @Override public Iterable<? extends SequenceComponent>
                                                    getExtensionComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
      
      return builder.build();
    }

  
/*
 */


//

/**
 */
public static class ganssEphExtDayType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_ganssEphExtDayType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public ganssEphExtDayType() {
    super();
    setValueRange("0", "8191");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_ganssEphExtDayType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_ganssEphExtDayType != null) {
      return ImmutableList.of(TAG_ganssEphExtDayType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new ganssEphExtDayType from encoded stream.
   */
  public static ganssEphExtDayType fromPerUnaligned(byte[] encodedBytes) {
    ganssEphExtDayType result = new ganssEphExtDayType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new ganssEphExtDayType from encoded stream.
   */
  public static ganssEphExtDayType fromPerAligned(byte[] encodedBytes) {
    ganssEphExtDayType result = new ganssEphExtDayType();
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
    return "ganssEphExtDayType = " + getInteger() + ";\n";
  }
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
    builder.append("GANSSEphemerisExtensionTime = {\n");
    final String internalIndent = indent + "  ";
    for (SequenceComponent component : getComponents()) {
      if (component.isExplicitlySet()) {
        builder.append(internalIndent)
            .append(component.toIndentedString(internalIndent));
      }
    }
    if (isExtensible()) {
      builder.append(internalIndent).append("...\n");
      for (SequenceComponent component : getExtensionComponents()) {
        if (component.isExplicitlySet()) {
          builder.append(internalIndent)
              .append(component.toIndentedString(internalIndent));
        }
      }
    }
    builder.append(indent).append("};\n");
    return builder.toString();
  }
}
