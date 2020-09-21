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
public  class OTD_MsrElementFirst_R98_Ext extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_OTD_MsrElementFirst_R98_Ext
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public OTD_MsrElementFirst_R98_Ext() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_OTD_MsrElementFirst_R98_Ext;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_OTD_MsrElementFirst_R98_Ext != null) {
      return ImmutableList.of(TAG_OTD_MsrElementFirst_R98_Ext);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new OTD_MsrElementFirst_R98_Ext from encoded stream.
   */
  public static OTD_MsrElementFirst_R98_Ext fromPerUnaligned(byte[] encodedBytes) {
    OTD_MsrElementFirst_R98_Ext result = new OTD_MsrElementFirst_R98_Ext();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new OTD_MsrElementFirst_R98_Ext from encoded stream.
   */
  public static OTD_MsrElementFirst_R98_Ext fromPerAligned(byte[] encodedBytes) {
    OTD_MsrElementFirst_R98_Ext result = new OTD_MsrElementFirst_R98_Ext();
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

  
  private SeqOfOTD_FirstSetMsrs_R98_Ext otd_FirstSetMsrs_R98_Ext_;
  public SeqOfOTD_FirstSetMsrs_R98_Ext getOtd_FirstSetMsrs_R98_Ext() {
    return otd_FirstSetMsrs_R98_Ext_;
  }
  /**
   * @throws ClassCastException if value is not a SeqOfOTD_FirstSetMsrs_R98_Ext
   */
  public void setOtd_FirstSetMsrs_R98_Ext(Asn1Object value) {
    this.otd_FirstSetMsrs_R98_Ext_ = (SeqOfOTD_FirstSetMsrs_R98_Ext) value;
  }
  public SeqOfOTD_FirstSetMsrs_R98_Ext setOtd_FirstSetMsrs_R98_ExtToNewInstance() {
    otd_FirstSetMsrs_R98_Ext_ = new SeqOfOTD_FirstSetMsrs_R98_Ext();
    return otd_FirstSetMsrs_R98_Ext_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getOtd_FirstSetMsrs_R98_Ext() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getOtd_FirstSetMsrs_R98_Ext();
          }

          @Override public void setToNewInstance() {
            setOtd_FirstSetMsrs_R98_ExtToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? SeqOfOTD_FirstSetMsrs_R98_Ext.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "otd_FirstSetMsrs_R98_Ext : "
                    + getOtd_FirstSetMsrs_R98_Ext().toIndentedString(indent);
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
    builder.append("OTD_MsrElementFirst_R98_Ext = {\n");
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
