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
public  class GANSSReferenceTime extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_GANSSReferenceTime
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public GANSSReferenceTime() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_GANSSReferenceTime;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_GANSSReferenceTime != null) {
      return ImmutableList.of(TAG_GANSSReferenceTime);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new GANSSReferenceTime from encoded stream.
   */
  public static GANSSReferenceTime fromPerUnaligned(byte[] encodedBytes) {
    GANSSReferenceTime result = new GANSSReferenceTime();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new GANSSReferenceTime from encoded stream.
   */
  public static GANSSReferenceTime fromPerAligned(byte[] encodedBytes) {
    GANSSReferenceTime result = new GANSSReferenceTime();
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

  
  private GANSSRefTimeInfo ganssRefTimeInfo_;
  public GANSSRefTimeInfo getGanssRefTimeInfo() {
    return ganssRefTimeInfo_;
  }
  /**
   * @throws ClassCastException if value is not a GANSSRefTimeInfo
   */
  public void setGanssRefTimeInfo(Asn1Object value) {
    this.ganssRefTimeInfo_ = (GANSSRefTimeInfo) value;
  }
  public GANSSRefTimeInfo setGanssRefTimeInfoToNewInstance() {
    ganssRefTimeInfo_ = new GANSSRefTimeInfo();
    return ganssRefTimeInfo_;
  }
  
  private GANSSTOD_GSMTimeAssociation ganssTOD_GSMTimeAssociation_;
  public GANSSTOD_GSMTimeAssociation getGanssTOD_GSMTimeAssociation() {
    return ganssTOD_GSMTimeAssociation_;
  }
  /**
   * @throws ClassCastException if value is not a GANSSTOD_GSMTimeAssociation
   */
  public void setGanssTOD_GSMTimeAssociation(Asn1Object value) {
    this.ganssTOD_GSMTimeAssociation_ = (GANSSTOD_GSMTimeAssociation) value;
  }
  public GANSSTOD_GSMTimeAssociation setGanssTOD_GSMTimeAssociationToNewInstance() {
    ganssTOD_GSMTimeAssociation_ = new GANSSTOD_GSMTimeAssociation();
    return ganssTOD_GSMTimeAssociation_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getGanssRefTimeInfo() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGanssRefTimeInfo();
          }

          @Override public void setToNewInstance() {
            setGanssRefTimeInfoToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GANSSRefTimeInfo.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "ganssRefTimeInfo : "
                    + getGanssRefTimeInfo().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getGanssTOD_GSMTimeAssociation() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getGanssTOD_GSMTimeAssociation();
          }

          @Override public void setToNewInstance() {
            setGanssTOD_GSMTimeAssociationToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GANSSTOD_GSMTimeAssociation.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "ganssTOD_GSMTimeAssociation : "
                    + getGanssTOD_GSMTimeAssociation().toIndentedString(indent);
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
    builder.append("GANSSReferenceTime = {\n");
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
