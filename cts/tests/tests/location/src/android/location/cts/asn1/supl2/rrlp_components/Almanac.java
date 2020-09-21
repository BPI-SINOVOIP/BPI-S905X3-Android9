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
public  class Almanac extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_Almanac
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public Almanac() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_Almanac;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_Almanac != null) {
      return ImmutableList.of(TAG_Almanac);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new Almanac from encoded stream.
   */
  public static Almanac fromPerUnaligned(byte[] encodedBytes) {
    Almanac result = new Almanac();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new Almanac from encoded stream.
   */
  public static Almanac fromPerAligned(byte[] encodedBytes) {
    Almanac result = new Almanac();
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

  
  private Almanac.alamanacWNaType alamanacWNa_;
  public Almanac.alamanacWNaType getAlamanacWNa() {
    return alamanacWNa_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac.alamanacWNaType
   */
  public void setAlamanacWNa(Asn1Object value) {
    this.alamanacWNa_ = (Almanac.alamanacWNaType) value;
  }
  public Almanac.alamanacWNaType setAlamanacWNaToNewInstance() {
    alamanacWNa_ = new Almanac.alamanacWNaType();
    return alamanacWNa_;
  }
  
  private SeqOfAlmanacElement almanacList_;
  public SeqOfAlmanacElement getAlmanacList() {
    return almanacList_;
  }
  /**
   * @throws ClassCastException if value is not a SeqOfAlmanacElement
   */
  public void setAlmanacList(Asn1Object value) {
    this.almanacList_ = (SeqOfAlmanacElement) value;
  }
  public SeqOfAlmanacElement setAlmanacListToNewInstance() {
    almanacList_ = new SeqOfAlmanacElement();
    return almanacList_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getAlamanacWNa() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getAlamanacWNa();
          }

          @Override public void setToNewInstance() {
            setAlamanacWNaToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac.alamanacWNaType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "alamanacWNa : "
                    + getAlamanacWNa().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getAlmanacList() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getAlmanacList();
          }

          @Override public void setToNewInstance() {
            setAlmanacListToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? SeqOfAlmanacElement.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "almanacList : "
                    + getAlmanacList().toIndentedString(indent);
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
public static class alamanacWNaType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_alamanacWNaType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public alamanacWNaType() {
    super();
    setValueRange("0", "255");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_alamanacWNaType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_alamanacWNaType != null) {
      return ImmutableList.of(TAG_alamanacWNaType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new alamanacWNaType from encoded stream.
   */
  public static alamanacWNaType fromPerUnaligned(byte[] encodedBytes) {
    alamanacWNaType result = new alamanacWNaType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new alamanacWNaType from encoded stream.
   */
  public static alamanacWNaType fromPerAligned(byte[] encodedBytes) {
    alamanacWNaType result = new alamanacWNaType();
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
    return "alamanacWNaType = " + getInteger() + ";\n";
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
    builder.append("Almanac = {\n");
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
