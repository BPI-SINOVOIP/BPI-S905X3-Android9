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
public  class AddionalDopplerFields extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_AddionalDopplerFields
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public AddionalDopplerFields() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_AddionalDopplerFields;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_AddionalDopplerFields != null) {
      return ImmutableList.of(TAG_AddionalDopplerFields);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new AddionalDopplerFields from encoded stream.
   */
  public static AddionalDopplerFields fromPerUnaligned(byte[] encodedBytes) {
    AddionalDopplerFields result = new AddionalDopplerFields();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new AddionalDopplerFields from encoded stream.
   */
  public static AddionalDopplerFields fromPerAligned(byte[] encodedBytes) {
    AddionalDopplerFields result = new AddionalDopplerFields();
    result.decodePerAligned(new BitStreamReader(encodedBytes));
    return result;
  }



  @Override protected boolean isExtensible() {
    return false;
  }

  @Override public boolean containsExtensionValues() {
    for (SequenceComponent extensionComponent : getExtensionComponents()) {
      if (extensionComponent.isExplicitlySet()) return true;
    }
    return false;
  }

  
  private AddionalDopplerFields.doppler1Type doppler1_;
  public AddionalDopplerFields.doppler1Type getDoppler1() {
    return doppler1_;
  }
  /**
   * @throws ClassCastException if value is not a AddionalDopplerFields.doppler1Type
   */
  public void setDoppler1(Asn1Object value) {
    this.doppler1_ = (AddionalDopplerFields.doppler1Type) value;
  }
  public AddionalDopplerFields.doppler1Type setDoppler1ToNewInstance() {
    doppler1_ = new AddionalDopplerFields.doppler1Type();
    return doppler1_;
  }
  
  private AddionalDopplerFields.dopplerUncertaintyType dopplerUncertainty_;
  public AddionalDopplerFields.dopplerUncertaintyType getDopplerUncertainty() {
    return dopplerUncertainty_;
  }
  /**
   * @throws ClassCastException if value is not a AddionalDopplerFields.dopplerUncertaintyType
   */
  public void setDopplerUncertainty(Asn1Object value) {
    this.dopplerUncertainty_ = (AddionalDopplerFields.dopplerUncertaintyType) value;
  }
  public AddionalDopplerFields.dopplerUncertaintyType setDopplerUncertaintyToNewInstance() {
    dopplerUncertainty_ = new AddionalDopplerFields.dopplerUncertaintyType();
    return dopplerUncertainty_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getDoppler1() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getDoppler1();
          }

          @Override public void setToNewInstance() {
            setDoppler1ToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? AddionalDopplerFields.doppler1Type.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "doppler1 : "
                    + getDoppler1().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getDopplerUncertainty() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getDopplerUncertainty();
          }

          @Override public void setToNewInstance() {
            setDopplerUncertaintyToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? AddionalDopplerFields.dopplerUncertaintyType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "dopplerUncertainty : "
                    + getDopplerUncertainty().toIndentedString(indent);
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
public static class doppler1Type extends Asn1Integer {
  //

  private static final Asn1Tag TAG_doppler1Type
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public doppler1Type() {
    super();
    setValueRange("0", "63");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_doppler1Type;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_doppler1Type != null) {
      return ImmutableList.of(TAG_doppler1Type);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new doppler1Type from encoded stream.
   */
  public static doppler1Type fromPerUnaligned(byte[] encodedBytes) {
    doppler1Type result = new doppler1Type();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new doppler1Type from encoded stream.
   */
  public static doppler1Type fromPerAligned(byte[] encodedBytes) {
    doppler1Type result = new doppler1Type();
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
    return "doppler1Type = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class dopplerUncertaintyType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_dopplerUncertaintyType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public dopplerUncertaintyType() {
    super();
    setValueRange("0", "7");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_dopplerUncertaintyType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_dopplerUncertaintyType != null) {
      return ImmutableList.of(TAG_dopplerUncertaintyType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new dopplerUncertaintyType from encoded stream.
   */
  public static dopplerUncertaintyType fromPerUnaligned(byte[] encodedBytes) {
    dopplerUncertaintyType result = new dopplerUncertaintyType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new dopplerUncertaintyType from encoded stream.
   */
  public static dopplerUncertaintyType fromPerAligned(byte[] encodedBytes) {
    dopplerUncertaintyType result = new dopplerUncertaintyType();
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
    return "dopplerUncertaintyType = " + getInteger() + ";\n";
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
    builder.append("AddionalDopplerFields = {\n");
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
