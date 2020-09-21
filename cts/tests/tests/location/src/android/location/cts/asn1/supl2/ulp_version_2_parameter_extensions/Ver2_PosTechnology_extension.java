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
public  class Ver2_PosTechnology_extension extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_Ver2_PosTechnology_extension
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public Ver2_PosTechnology_extension() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_Ver2_PosTechnology_extension;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_Ver2_PosTechnology_extension != null) {
      return ImmutableList.of(TAG_Ver2_PosTechnology_extension);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new Ver2_PosTechnology_extension from encoded stream.
   */
  public static Ver2_PosTechnology_extension fromPerUnaligned(byte[] encodedBytes) {
    Ver2_PosTechnology_extension result = new Ver2_PosTechnology_extension();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new Ver2_PosTechnology_extension from encoded stream.
   */
  public static Ver2_PosTechnology_extension fromPerAligned(byte[] encodedBytes) {
    Ver2_PosTechnology_extension result = new Ver2_PosTechnology_extension();
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

  
  private GANSSPositionMethods gANSSPositionMethods_;
  public GANSSPositionMethods getGANSSPositionMethods() {
    return gANSSPositionMethods_;
  }
  /**
   * @throws ClassCastException if value is not a GANSSPositionMethods
   */
  public void setGANSSPositionMethods(Asn1Object value) {
    this.gANSSPositionMethods_ = (GANSSPositionMethods) value;
  }
  public GANSSPositionMethods setGANSSPositionMethodsToNewInstance() {
    gANSSPositionMethods_ = new GANSSPositionMethods();
    return gANSSPositionMethods_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getGANSSPositionMethods() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getGANSSPositionMethods();
          }

          @Override public void setToNewInstance() {
            setGANSSPositionMethodsToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GANSSPositionMethods.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gANSSPositionMethods : "
                    + getGANSSPositionMethods().toIndentedString(indent);
              }
        });
    
    return builder.build();
  }

  @Override public Iterable<? extends SequenceComponent>
                                                    getExtensionComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
      
      return builder.build();
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
    builder.append("Ver2_PosTechnology_extension = {\n");
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
