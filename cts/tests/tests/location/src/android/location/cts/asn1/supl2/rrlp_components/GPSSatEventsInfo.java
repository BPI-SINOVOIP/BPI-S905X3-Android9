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
import android.location.cts.asn1.base.Asn1BitString;
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
public  class GPSSatEventsInfo extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_GPSSatEventsInfo
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public GPSSatEventsInfo() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_GPSSatEventsInfo;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_GPSSatEventsInfo != null) {
      return ImmutableList.of(TAG_GPSSatEventsInfo);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new GPSSatEventsInfo from encoded stream.
   */
  public static GPSSatEventsInfo fromPerUnaligned(byte[] encodedBytes) {
    GPSSatEventsInfo result = new GPSSatEventsInfo();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new GPSSatEventsInfo from encoded stream.
   */
  public static GPSSatEventsInfo fromPerAligned(byte[] encodedBytes) {
    GPSSatEventsInfo result = new GPSSatEventsInfo();
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

  
  private GPSSatEventsInfo.eventOccuredType eventOccured_;
  public GPSSatEventsInfo.eventOccuredType getEventOccured() {
    return eventOccured_;
  }
  /**
   * @throws ClassCastException if value is not a GPSSatEventsInfo.eventOccuredType
   */
  public void setEventOccured(Asn1Object value) {
    this.eventOccured_ = (GPSSatEventsInfo.eventOccuredType) value;
  }
  public GPSSatEventsInfo.eventOccuredType setEventOccuredToNewInstance() {
    eventOccured_ = new GPSSatEventsInfo.eventOccuredType();
    return eventOccured_;
  }
  
  private GPSSatEventsInfo.futureEventNotedType futureEventNoted_;
  public GPSSatEventsInfo.futureEventNotedType getFutureEventNoted() {
    return futureEventNoted_;
  }
  /**
   * @throws ClassCastException if value is not a GPSSatEventsInfo.futureEventNotedType
   */
  public void setFutureEventNoted(Asn1Object value) {
    this.futureEventNoted_ = (GPSSatEventsInfo.futureEventNotedType) value;
  }
  public GPSSatEventsInfo.futureEventNotedType setFutureEventNotedToNewInstance() {
    futureEventNoted_ = new GPSSatEventsInfo.futureEventNotedType();
    return futureEventNoted_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getEventOccured() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getEventOccured();
          }

          @Override public void setToNewInstance() {
            setEventOccuredToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GPSSatEventsInfo.eventOccuredType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "eventOccured : "
                    + getEventOccured().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getFutureEventNoted() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getFutureEventNoted();
          }

          @Override public void setToNewInstance() {
            setFutureEventNotedToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GPSSatEventsInfo.futureEventNotedType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "futureEventNoted : "
                    + getFutureEventNoted().toIndentedString(indent);
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
public static class eventOccuredType extends Asn1BitString {
  //

  private static final Asn1Tag TAG_eventOccuredType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public eventOccuredType() {
    super();
    setMinSize(32);
setMaxSize(32);

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_eventOccuredType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_eventOccuredType != null) {
      return ImmutableList.of(TAG_eventOccuredType);
    } else {
      return Asn1BitString.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new eventOccuredType from encoded stream.
   */
  public static eventOccuredType fromPerUnaligned(byte[] encodedBytes) {
    eventOccuredType result = new eventOccuredType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new eventOccuredType from encoded stream.
   */
  public static eventOccuredType fromPerAligned(byte[] encodedBytes) {
    eventOccuredType result = new eventOccuredType();
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
    return "eventOccuredType = " + getValue() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class futureEventNotedType extends Asn1BitString {
  //

  private static final Asn1Tag TAG_futureEventNotedType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public futureEventNotedType() {
    super();
    setMinSize(32);
setMaxSize(32);

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_futureEventNotedType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_futureEventNotedType != null) {
      return ImmutableList.of(TAG_futureEventNotedType);
    } else {
      return Asn1BitString.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new futureEventNotedType from encoded stream.
   */
  public static futureEventNotedType fromPerUnaligned(byte[] encodedBytes) {
    futureEventNotedType result = new futureEventNotedType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new futureEventNotedType from encoded stream.
   */
  public static futureEventNotedType fromPerAligned(byte[] encodedBytes) {
    futureEventNotedType result = new futureEventNotedType();
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
    return "futureEventNotedType = " + getValue() + ";\n";
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
    builder.append("GPSSatEventsInfo = {\n");
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
