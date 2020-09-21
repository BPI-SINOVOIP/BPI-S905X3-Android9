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
public  class GANSSDeltaEpochHeader extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_GANSSDeltaEpochHeader
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public GANSSDeltaEpochHeader() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_GANSSDeltaEpochHeader;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_GANSSDeltaEpochHeader != null) {
      return ImmutableList.of(TAG_GANSSDeltaEpochHeader);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new GANSSDeltaEpochHeader from encoded stream.
   */
  public static GANSSDeltaEpochHeader fromPerUnaligned(byte[] encodedBytes) {
    GANSSDeltaEpochHeader result = new GANSSDeltaEpochHeader();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new GANSSDeltaEpochHeader from encoded stream.
   */
  public static GANSSDeltaEpochHeader fromPerAligned(byte[] encodedBytes) {
    GANSSDeltaEpochHeader result = new GANSSDeltaEpochHeader();
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

  
  private GANSSDeltaEpochHeader.validityPeriodType validityPeriod_;
  public GANSSDeltaEpochHeader.validityPeriodType getValidityPeriod() {
    return validityPeriod_;
  }
  /**
   * @throws ClassCastException if value is not a GANSSDeltaEpochHeader.validityPeriodType
   */
  public void setValidityPeriod(Asn1Object value) {
    this.validityPeriod_ = (GANSSDeltaEpochHeader.validityPeriodType) value;
  }
  public GANSSDeltaEpochHeader.validityPeriodType setValidityPeriodToNewInstance() {
    validityPeriod_ = new GANSSDeltaEpochHeader.validityPeriodType();
    return validityPeriod_;
  }
  
  private GANSSEphemerisDeltaBitSizes ephemerisDeltaSizes_;
  public GANSSEphemerisDeltaBitSizes getEphemerisDeltaSizes() {
    return ephemerisDeltaSizes_;
  }
  /**
   * @throws ClassCastException if value is not a GANSSEphemerisDeltaBitSizes
   */
  public void setEphemerisDeltaSizes(Asn1Object value) {
    this.ephemerisDeltaSizes_ = (GANSSEphemerisDeltaBitSizes) value;
  }
  public GANSSEphemerisDeltaBitSizes setEphemerisDeltaSizesToNewInstance() {
    ephemerisDeltaSizes_ = new GANSSEphemerisDeltaBitSizes();
    return ephemerisDeltaSizes_;
  }
  
  private GANSSEphemerisDeltaScales ephemerisDeltaScales_;
  public GANSSEphemerisDeltaScales getEphemerisDeltaScales() {
    return ephemerisDeltaScales_;
  }
  /**
   * @throws ClassCastException if value is not a GANSSEphemerisDeltaScales
   */
  public void setEphemerisDeltaScales(Asn1Object value) {
    this.ephemerisDeltaScales_ = (GANSSEphemerisDeltaScales) value;
  }
  public GANSSEphemerisDeltaScales setEphemerisDeltaScalesToNewInstance() {
    ephemerisDeltaScales_ = new GANSSEphemerisDeltaScales();
    return ephemerisDeltaScales_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getValidityPeriod() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getValidityPeriod();
          }

          @Override public void setToNewInstance() {
            setValidityPeriodToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GANSSDeltaEpochHeader.validityPeriodType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "validityPeriod : "
                    + getValidityPeriod().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getEphemerisDeltaSizes() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getEphemerisDeltaSizes();
          }

          @Override public void setToNewInstance() {
            setEphemerisDeltaSizesToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GANSSEphemerisDeltaBitSizes.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "ephemerisDeltaSizes : "
                    + getEphemerisDeltaSizes().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 2);

          @Override public boolean isExplicitlySet() {
            return getEphemerisDeltaScales() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getEphemerisDeltaScales();
          }

          @Override public void setToNewInstance() {
            setEphemerisDeltaScalesToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GANSSEphemerisDeltaScales.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "ephemerisDeltaScales : "
                    + getEphemerisDeltaScales().toIndentedString(indent);
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
public static class validityPeriodType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_validityPeriodType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public validityPeriodType() {
    super();
    setValueRange("1", "8");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_validityPeriodType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_validityPeriodType != null) {
      return ImmutableList.of(TAG_validityPeriodType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new validityPeriodType from encoded stream.
   */
  public static validityPeriodType fromPerUnaligned(byte[] encodedBytes) {
    validityPeriodType result = new validityPeriodType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new validityPeriodType from encoded stream.
   */
  public static validityPeriodType fromPerAligned(byte[] encodedBytes) {
    validityPeriodType result = new validityPeriodType();
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
    return "validityPeriodType = " + getInteger() + ";\n";
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
    builder.append("GANSSDeltaEpochHeader = {\n");
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
