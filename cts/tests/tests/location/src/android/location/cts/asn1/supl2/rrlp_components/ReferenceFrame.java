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
public  class ReferenceFrame extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_ReferenceFrame
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public ReferenceFrame() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_ReferenceFrame;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_ReferenceFrame != null) {
      return ImmutableList.of(TAG_ReferenceFrame);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new ReferenceFrame from encoded stream.
   */
  public static ReferenceFrame fromPerUnaligned(byte[] encodedBytes) {
    ReferenceFrame result = new ReferenceFrame();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new ReferenceFrame from encoded stream.
   */
  public static ReferenceFrame fromPerAligned(byte[] encodedBytes) {
    ReferenceFrame result = new ReferenceFrame();
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

  
  private ReferenceFrame.referenceFNType referenceFN_;
  public ReferenceFrame.referenceFNType getReferenceFN() {
    return referenceFN_;
  }
  /**
   * @throws ClassCastException if value is not a ReferenceFrame.referenceFNType
   */
  public void setReferenceFN(Asn1Object value) {
    this.referenceFN_ = (ReferenceFrame.referenceFNType) value;
  }
  public ReferenceFrame.referenceFNType setReferenceFNToNewInstance() {
    referenceFN_ = new ReferenceFrame.referenceFNType();
    return referenceFN_;
  }
  
  private ReferenceFrame.referenceFNMSBType referenceFNMSB_;
  public ReferenceFrame.referenceFNMSBType getReferenceFNMSB() {
    return referenceFNMSB_;
  }
  /**
   * @throws ClassCastException if value is not a ReferenceFrame.referenceFNMSBType
   */
  public void setReferenceFNMSB(Asn1Object value) {
    this.referenceFNMSB_ = (ReferenceFrame.referenceFNMSBType) value;
  }
  public ReferenceFrame.referenceFNMSBType setReferenceFNMSBToNewInstance() {
    referenceFNMSB_ = new ReferenceFrame.referenceFNMSBType();
    return referenceFNMSB_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getReferenceFN() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getReferenceFN();
          }

          @Override public void setToNewInstance() {
            setReferenceFNToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? ReferenceFrame.referenceFNType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "referenceFN : "
                    + getReferenceFN().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getReferenceFNMSB() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getReferenceFNMSB();
          }

          @Override public void setToNewInstance() {
            setReferenceFNMSBToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? ReferenceFrame.referenceFNMSBType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "referenceFNMSB : "
                    + getReferenceFNMSB().toIndentedString(indent);
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
public static class referenceFNType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_referenceFNType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public referenceFNType() {
    super();
    setValueRange("0", "65535");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_referenceFNType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_referenceFNType != null) {
      return ImmutableList.of(TAG_referenceFNType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new referenceFNType from encoded stream.
   */
  public static referenceFNType fromPerUnaligned(byte[] encodedBytes) {
    referenceFNType result = new referenceFNType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new referenceFNType from encoded stream.
   */
  public static referenceFNType fromPerAligned(byte[] encodedBytes) {
    referenceFNType result = new referenceFNType();
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
    return "referenceFNType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class referenceFNMSBType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_referenceFNMSBType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public referenceFNMSBType() {
    super();
    setValueRange("0", "63");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_referenceFNMSBType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_referenceFNMSBType != null) {
      return ImmutableList.of(TAG_referenceFNMSBType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new referenceFNMSBType from encoded stream.
   */
  public static referenceFNMSBType fromPerUnaligned(byte[] encodedBytes) {
    referenceFNMSBType result = new referenceFNMSBType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new referenceFNMSBType from encoded stream.
   */
  public static referenceFNMSBType fromPerAligned(byte[] encodedBytes) {
    referenceFNMSBType result = new referenceFNMSBType();
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
    return "referenceFNMSBType = " + getInteger() + ";\n";
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
    builder.append("ReferenceFrame = {\n");
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
