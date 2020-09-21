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
import android.location.cts.asn1.supl2.map_extensiondatatypes.ExtensionContainer;
import com.google.common.collect.ImmutableList;
import java.util.Collection;
import javax.annotation.Nullable;


/**
*/
public  class PosCapability_Rsp extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_PosCapability_Rsp
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public PosCapability_Rsp() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_PosCapability_Rsp;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_PosCapability_Rsp != null) {
      return ImmutableList.of(TAG_PosCapability_Rsp);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new PosCapability_Rsp from encoded stream.
   */
  public static PosCapability_Rsp fromPerUnaligned(byte[] encodedBytes) {
    PosCapability_Rsp result = new PosCapability_Rsp();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new PosCapability_Rsp from encoded stream.
   */
  public static PosCapability_Rsp fromPerAligned(byte[] encodedBytes) {
    PosCapability_Rsp result = new PosCapability_Rsp();
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

  
  private Extended_reference extended_reference_;
  public Extended_reference getExtended_reference() {
    return extended_reference_;
  }
  /**
   * @throws ClassCastException if value is not a Extended_reference
   */
  public void setExtended_reference(Asn1Object value) {
    this.extended_reference_ = (Extended_reference) value;
  }
  public Extended_reference setExtended_referenceToNewInstance() {
    extended_reference_ = new Extended_reference();
    return extended_reference_;
  }
  
  private PosCapabilities posCapabilities_;
  public PosCapabilities getPosCapabilities() {
    return posCapabilities_;
  }
  /**
   * @throws ClassCastException if value is not a PosCapabilities
   */
  public void setPosCapabilities(Asn1Object value) {
    this.posCapabilities_ = (PosCapabilities) value;
  }
  public PosCapabilities setPosCapabilitiesToNewInstance() {
    posCapabilities_ = new PosCapabilities();
    return posCapabilities_;
  }
  
  private AssistanceSupported assistanceSupported_;
  public AssistanceSupported getAssistanceSupported() {
    return assistanceSupported_;
  }
  /**
   * @throws ClassCastException if value is not a AssistanceSupported
   */
  public void setAssistanceSupported(Asn1Object value) {
    this.assistanceSupported_ = (AssistanceSupported) value;
  }
  public AssistanceSupported setAssistanceSupportedToNewInstance() {
    assistanceSupported_ = new AssistanceSupported();
    return assistanceSupported_;
  }
  
  private AssistanceNeeded assistanceNeeded_;
  public AssistanceNeeded getAssistanceNeeded() {
    return assistanceNeeded_;
  }
  /**
   * @throws ClassCastException if value is not a AssistanceNeeded
   */
  public void setAssistanceNeeded(Asn1Object value) {
    this.assistanceNeeded_ = (AssistanceNeeded) value;
  }
  public AssistanceNeeded setAssistanceNeededToNewInstance() {
    assistanceNeeded_ = new AssistanceNeeded();
    return assistanceNeeded_;
  }
  
  private ExtensionContainer extensionContainer_;
  public ExtensionContainer getExtensionContainer() {
    return extensionContainer_;
  }
  /**
   * @throws ClassCastException if value is not a ExtensionContainer
   */
  public void setExtensionContainer(Asn1Object value) {
    this.extensionContainer_ = (ExtensionContainer) value;
  }
  public ExtensionContainer setExtensionContainerToNewInstance() {
    extensionContainer_ = new ExtensionContainer();
    return extensionContainer_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getExtended_reference() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getExtended_reference();
          }

          @Override public void setToNewInstance() {
            setExtended_referenceToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Extended_reference.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "extended_reference : "
                    + getExtended_reference().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getPosCapabilities() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getPosCapabilities();
          }

          @Override public void setToNewInstance() {
            setPosCapabilitiesToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? PosCapabilities.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "posCapabilities : "
                    + getPosCapabilities().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 2);

          @Override public boolean isExplicitlySet() {
            return getAssistanceSupported() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getAssistanceSupported();
          }

          @Override public void setToNewInstance() {
            setAssistanceSupportedToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? AssistanceSupported.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "assistanceSupported : "
                    + getAssistanceSupported().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 3);

          @Override public boolean isExplicitlySet() {
            return getAssistanceNeeded() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getAssistanceNeeded();
          }

          @Override public void setToNewInstance() {
            setAssistanceNeededToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? AssistanceNeeded.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "assistanceNeeded : "
                    + getAssistanceNeeded().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 4);

          @Override public boolean isExplicitlySet() {
            return getExtensionContainer() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getExtensionContainer();
          }

          @Override public void setToNewInstance() {
            setExtensionContainerToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? ExtensionContainer.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "extensionContainer : "
                    + getExtensionContainer().toIndentedString(indent);
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
    builder.append("PosCapability_Rsp = {\n");
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
