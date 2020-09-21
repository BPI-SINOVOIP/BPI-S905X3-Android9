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
import android.location.cts.asn1.base.Asn1Boolean;
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
public  class Almanac_MidiAlmanacSet extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_Almanac_MidiAlmanacSet
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public Almanac_MidiAlmanacSet() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_Almanac_MidiAlmanacSet;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_Almanac_MidiAlmanacSet != null) {
      return ImmutableList.of(TAG_Almanac_MidiAlmanacSet);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new Almanac_MidiAlmanacSet from encoded stream.
   */
  public static Almanac_MidiAlmanacSet fromPerUnaligned(byte[] encodedBytes) {
    Almanac_MidiAlmanacSet result = new Almanac_MidiAlmanacSet();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new Almanac_MidiAlmanacSet from encoded stream.
   */
  public static Almanac_MidiAlmanacSet fromPerAligned(byte[] encodedBytes) {
    Almanac_MidiAlmanacSet result = new Almanac_MidiAlmanacSet();
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

  
  private SVID svID_;
  public SVID getSvID() {
    return svID_;
  }
  /**
   * @throws ClassCastException if value is not a SVID
   */
  public void setSvID(Asn1Object value) {
    this.svID_ = (SVID) value;
  }
  public SVID setSvIDToNewInstance() {
    svID_ = new SVID();
    return svID_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmEType midiAlmE_;
  public Almanac_MidiAlmanacSet.midiAlmEType getMidiAlmE() {
    return midiAlmE_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmEType
   */
  public void setMidiAlmE(Asn1Object value) {
    this.midiAlmE_ = (Almanac_MidiAlmanacSet.midiAlmEType) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmEType setMidiAlmEToNewInstance() {
    midiAlmE_ = new Almanac_MidiAlmanacSet.midiAlmEType();
    return midiAlmE_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmDeltaIType midiAlmDeltaI_;
  public Almanac_MidiAlmanacSet.midiAlmDeltaIType getMidiAlmDeltaI() {
    return midiAlmDeltaI_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmDeltaIType
   */
  public void setMidiAlmDeltaI(Asn1Object value) {
    this.midiAlmDeltaI_ = (Almanac_MidiAlmanacSet.midiAlmDeltaIType) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmDeltaIType setMidiAlmDeltaIToNewInstance() {
    midiAlmDeltaI_ = new Almanac_MidiAlmanacSet.midiAlmDeltaIType();
    return midiAlmDeltaI_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmOmegaDotType midiAlmOmegaDot_;
  public Almanac_MidiAlmanacSet.midiAlmOmegaDotType getMidiAlmOmegaDot() {
    return midiAlmOmegaDot_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmOmegaDotType
   */
  public void setMidiAlmOmegaDot(Asn1Object value) {
    this.midiAlmOmegaDot_ = (Almanac_MidiAlmanacSet.midiAlmOmegaDotType) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmOmegaDotType setMidiAlmOmegaDotToNewInstance() {
    midiAlmOmegaDot_ = new Almanac_MidiAlmanacSet.midiAlmOmegaDotType();
    return midiAlmOmegaDot_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmSqrtAType midiAlmSqrtA_;
  public Almanac_MidiAlmanacSet.midiAlmSqrtAType getMidiAlmSqrtA() {
    return midiAlmSqrtA_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmSqrtAType
   */
  public void setMidiAlmSqrtA(Asn1Object value) {
    this.midiAlmSqrtA_ = (Almanac_MidiAlmanacSet.midiAlmSqrtAType) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmSqrtAType setMidiAlmSqrtAToNewInstance() {
    midiAlmSqrtA_ = new Almanac_MidiAlmanacSet.midiAlmSqrtAType();
    return midiAlmSqrtA_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmOmega0Type midiAlmOmega0_;
  public Almanac_MidiAlmanacSet.midiAlmOmega0Type getMidiAlmOmega0() {
    return midiAlmOmega0_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmOmega0Type
   */
  public void setMidiAlmOmega0(Asn1Object value) {
    this.midiAlmOmega0_ = (Almanac_MidiAlmanacSet.midiAlmOmega0Type) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmOmega0Type setMidiAlmOmega0ToNewInstance() {
    midiAlmOmega0_ = new Almanac_MidiAlmanacSet.midiAlmOmega0Type();
    return midiAlmOmega0_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmOmegaType midiAlmOmega_;
  public Almanac_MidiAlmanacSet.midiAlmOmegaType getMidiAlmOmega() {
    return midiAlmOmega_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmOmegaType
   */
  public void setMidiAlmOmega(Asn1Object value) {
    this.midiAlmOmega_ = (Almanac_MidiAlmanacSet.midiAlmOmegaType) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmOmegaType setMidiAlmOmegaToNewInstance() {
    midiAlmOmega_ = new Almanac_MidiAlmanacSet.midiAlmOmegaType();
    return midiAlmOmega_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmMoType midiAlmMo_;
  public Almanac_MidiAlmanacSet.midiAlmMoType getMidiAlmMo() {
    return midiAlmMo_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmMoType
   */
  public void setMidiAlmMo(Asn1Object value) {
    this.midiAlmMo_ = (Almanac_MidiAlmanacSet.midiAlmMoType) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmMoType setMidiAlmMoToNewInstance() {
    midiAlmMo_ = new Almanac_MidiAlmanacSet.midiAlmMoType();
    return midiAlmMo_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmaf0Type midiAlmaf0_;
  public Almanac_MidiAlmanacSet.midiAlmaf0Type getMidiAlmaf0() {
    return midiAlmaf0_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmaf0Type
   */
  public void setMidiAlmaf0(Asn1Object value) {
    this.midiAlmaf0_ = (Almanac_MidiAlmanacSet.midiAlmaf0Type) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmaf0Type setMidiAlmaf0ToNewInstance() {
    midiAlmaf0_ = new Almanac_MidiAlmanacSet.midiAlmaf0Type();
    return midiAlmaf0_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmaf1Type midiAlmaf1_;
  public Almanac_MidiAlmanacSet.midiAlmaf1Type getMidiAlmaf1() {
    return midiAlmaf1_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmaf1Type
   */
  public void setMidiAlmaf1(Asn1Object value) {
    this.midiAlmaf1_ = (Almanac_MidiAlmanacSet.midiAlmaf1Type) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmaf1Type setMidiAlmaf1ToNewInstance() {
    midiAlmaf1_ = new Almanac_MidiAlmanacSet.midiAlmaf1Type();
    return midiAlmaf1_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmL1HealthType midiAlmL1Health_;
  public Almanac_MidiAlmanacSet.midiAlmL1HealthType getMidiAlmL1Health() {
    return midiAlmL1Health_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmL1HealthType
   */
  public void setMidiAlmL1Health(Asn1Object value) {
    this.midiAlmL1Health_ = (Almanac_MidiAlmanacSet.midiAlmL1HealthType) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmL1HealthType setMidiAlmL1HealthToNewInstance() {
    midiAlmL1Health_ = new Almanac_MidiAlmanacSet.midiAlmL1HealthType();
    return midiAlmL1Health_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmL2HealthType midiAlmL2Health_;
  public Almanac_MidiAlmanacSet.midiAlmL2HealthType getMidiAlmL2Health() {
    return midiAlmL2Health_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmL2HealthType
   */
  public void setMidiAlmL2Health(Asn1Object value) {
    this.midiAlmL2Health_ = (Almanac_MidiAlmanacSet.midiAlmL2HealthType) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmL2HealthType setMidiAlmL2HealthToNewInstance() {
    midiAlmL2Health_ = new Almanac_MidiAlmanacSet.midiAlmL2HealthType();
    return midiAlmL2Health_;
  }
  
  private Almanac_MidiAlmanacSet.midiAlmL5HealthType midiAlmL5Health_;
  public Almanac_MidiAlmanacSet.midiAlmL5HealthType getMidiAlmL5Health() {
    return midiAlmL5Health_;
  }
  /**
   * @throws ClassCastException if value is not a Almanac_MidiAlmanacSet.midiAlmL5HealthType
   */
  public void setMidiAlmL5Health(Asn1Object value) {
    this.midiAlmL5Health_ = (Almanac_MidiAlmanacSet.midiAlmL5HealthType) value;
  }
  public Almanac_MidiAlmanacSet.midiAlmL5HealthType setMidiAlmL5HealthToNewInstance() {
    midiAlmL5Health_ = new Almanac_MidiAlmanacSet.midiAlmL5HealthType();
    return midiAlmL5Health_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getSvID() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSvID();
          }

          @Override public void setToNewInstance() {
            setSvIDToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? SVID.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "svID : "
                    + getSvID().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmE() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmE();
          }

          @Override public void setToNewInstance() {
            setMidiAlmEToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmEType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmE : "
                    + getMidiAlmE().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 2);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmDeltaI() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmDeltaI();
          }

          @Override public void setToNewInstance() {
            setMidiAlmDeltaIToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmDeltaIType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmDeltaI : "
                    + getMidiAlmDeltaI().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 3);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmOmegaDot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmOmegaDot();
          }

          @Override public void setToNewInstance() {
            setMidiAlmOmegaDotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmOmegaDotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmOmegaDot : "
                    + getMidiAlmOmegaDot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 4);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmSqrtA() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmSqrtA();
          }

          @Override public void setToNewInstance() {
            setMidiAlmSqrtAToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmSqrtAType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmSqrtA : "
                    + getMidiAlmSqrtA().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 5);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmOmega0() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmOmega0();
          }

          @Override public void setToNewInstance() {
            setMidiAlmOmega0ToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmOmega0Type.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmOmega0 : "
                    + getMidiAlmOmega0().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 6);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmOmega() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmOmega();
          }

          @Override public void setToNewInstance() {
            setMidiAlmOmegaToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmOmegaType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmOmega : "
                    + getMidiAlmOmega().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 7);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmMo() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmMo();
          }

          @Override public void setToNewInstance() {
            setMidiAlmMoToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmMoType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmMo : "
                    + getMidiAlmMo().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 8);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmaf0() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmaf0();
          }

          @Override public void setToNewInstance() {
            setMidiAlmaf0ToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmaf0Type.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmaf0 : "
                    + getMidiAlmaf0().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 9);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmaf1() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmaf1();
          }

          @Override public void setToNewInstance() {
            setMidiAlmaf1ToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmaf1Type.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmaf1 : "
                    + getMidiAlmaf1().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 10);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmL1Health() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmL1Health();
          }

          @Override public void setToNewInstance() {
            setMidiAlmL1HealthToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmL1HealthType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmL1Health : "
                    + getMidiAlmL1Health().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 11);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmL2Health() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmL2Health();
          }

          @Override public void setToNewInstance() {
            setMidiAlmL2HealthToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmL2HealthType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmL2Health : "
                    + getMidiAlmL2Health().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 12);

          @Override public boolean isExplicitlySet() {
            return getMidiAlmL5Health() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMidiAlmL5Health();
          }

          @Override public void setToNewInstance() {
            setMidiAlmL5HealthToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? Almanac_MidiAlmanacSet.midiAlmL5HealthType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "midiAlmL5Health : "
                    + getMidiAlmL5Health().toIndentedString(indent);
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
public static class midiAlmEType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_midiAlmEType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmEType() {
    super();
    setValueRange("0", "2047");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmEType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmEType != null) {
      return ImmutableList.of(TAG_midiAlmEType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmEType from encoded stream.
   */
  public static midiAlmEType fromPerUnaligned(byte[] encodedBytes) {
    midiAlmEType result = new midiAlmEType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmEType from encoded stream.
   */
  public static midiAlmEType fromPerAligned(byte[] encodedBytes) {
    midiAlmEType result = new midiAlmEType();
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
    return "midiAlmEType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmDeltaIType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_midiAlmDeltaIType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmDeltaIType() {
    super();
    setValueRange("-1024", "1023");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmDeltaIType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmDeltaIType != null) {
      return ImmutableList.of(TAG_midiAlmDeltaIType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmDeltaIType from encoded stream.
   */
  public static midiAlmDeltaIType fromPerUnaligned(byte[] encodedBytes) {
    midiAlmDeltaIType result = new midiAlmDeltaIType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmDeltaIType from encoded stream.
   */
  public static midiAlmDeltaIType fromPerAligned(byte[] encodedBytes) {
    midiAlmDeltaIType result = new midiAlmDeltaIType();
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
    return "midiAlmDeltaIType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmOmegaDotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_midiAlmOmegaDotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmOmegaDotType() {
    super();
    setValueRange("-1024", "1023");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmOmegaDotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmOmegaDotType != null) {
      return ImmutableList.of(TAG_midiAlmOmegaDotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmOmegaDotType from encoded stream.
   */
  public static midiAlmOmegaDotType fromPerUnaligned(byte[] encodedBytes) {
    midiAlmOmegaDotType result = new midiAlmOmegaDotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmOmegaDotType from encoded stream.
   */
  public static midiAlmOmegaDotType fromPerAligned(byte[] encodedBytes) {
    midiAlmOmegaDotType result = new midiAlmOmegaDotType();
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
    return "midiAlmOmegaDotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmSqrtAType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_midiAlmSqrtAType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmSqrtAType() {
    super();
    setValueRange("0", "131071");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmSqrtAType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmSqrtAType != null) {
      return ImmutableList.of(TAG_midiAlmSqrtAType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmSqrtAType from encoded stream.
   */
  public static midiAlmSqrtAType fromPerUnaligned(byte[] encodedBytes) {
    midiAlmSqrtAType result = new midiAlmSqrtAType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmSqrtAType from encoded stream.
   */
  public static midiAlmSqrtAType fromPerAligned(byte[] encodedBytes) {
    midiAlmSqrtAType result = new midiAlmSqrtAType();
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
    return "midiAlmSqrtAType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmOmega0Type extends Asn1Integer {
  //

  private static final Asn1Tag TAG_midiAlmOmega0Type
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmOmega0Type() {
    super();
    setValueRange("-32768", "32767");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmOmega0Type;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmOmega0Type != null) {
      return ImmutableList.of(TAG_midiAlmOmega0Type);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmOmega0Type from encoded stream.
   */
  public static midiAlmOmega0Type fromPerUnaligned(byte[] encodedBytes) {
    midiAlmOmega0Type result = new midiAlmOmega0Type();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmOmega0Type from encoded stream.
   */
  public static midiAlmOmega0Type fromPerAligned(byte[] encodedBytes) {
    midiAlmOmega0Type result = new midiAlmOmega0Type();
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
    return "midiAlmOmega0Type = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmOmegaType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_midiAlmOmegaType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmOmegaType() {
    super();
    setValueRange("-32768", "32767");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmOmegaType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmOmegaType != null) {
      return ImmutableList.of(TAG_midiAlmOmegaType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmOmegaType from encoded stream.
   */
  public static midiAlmOmegaType fromPerUnaligned(byte[] encodedBytes) {
    midiAlmOmegaType result = new midiAlmOmegaType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmOmegaType from encoded stream.
   */
  public static midiAlmOmegaType fromPerAligned(byte[] encodedBytes) {
    midiAlmOmegaType result = new midiAlmOmegaType();
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
    return "midiAlmOmegaType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmMoType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_midiAlmMoType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmMoType() {
    super();
    setValueRange("-32768", "32767");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmMoType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmMoType != null) {
      return ImmutableList.of(TAG_midiAlmMoType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmMoType from encoded stream.
   */
  public static midiAlmMoType fromPerUnaligned(byte[] encodedBytes) {
    midiAlmMoType result = new midiAlmMoType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmMoType from encoded stream.
   */
  public static midiAlmMoType fromPerAligned(byte[] encodedBytes) {
    midiAlmMoType result = new midiAlmMoType();
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
    return "midiAlmMoType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmaf0Type extends Asn1Integer {
  //

  private static final Asn1Tag TAG_midiAlmaf0Type
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmaf0Type() {
    super();
    setValueRange("-1024", "1023");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmaf0Type;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmaf0Type != null) {
      return ImmutableList.of(TAG_midiAlmaf0Type);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmaf0Type from encoded stream.
   */
  public static midiAlmaf0Type fromPerUnaligned(byte[] encodedBytes) {
    midiAlmaf0Type result = new midiAlmaf0Type();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmaf0Type from encoded stream.
   */
  public static midiAlmaf0Type fromPerAligned(byte[] encodedBytes) {
    midiAlmaf0Type result = new midiAlmaf0Type();
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
    return "midiAlmaf0Type = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmaf1Type extends Asn1Integer {
  //

  private static final Asn1Tag TAG_midiAlmaf1Type
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmaf1Type() {
    super();
    setValueRange("-512", "511");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmaf1Type;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmaf1Type != null) {
      return ImmutableList.of(TAG_midiAlmaf1Type);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmaf1Type from encoded stream.
   */
  public static midiAlmaf1Type fromPerUnaligned(byte[] encodedBytes) {
    midiAlmaf1Type result = new midiAlmaf1Type();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmaf1Type from encoded stream.
   */
  public static midiAlmaf1Type fromPerAligned(byte[] encodedBytes) {
    midiAlmaf1Type result = new midiAlmaf1Type();
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
    return "midiAlmaf1Type = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmL1HealthType extends Asn1Boolean {
  //

  private static final Asn1Tag TAG_midiAlmL1HealthType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmL1HealthType() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmL1HealthType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmL1HealthType != null) {
      return ImmutableList.of(TAG_midiAlmL1HealthType);
    } else {
      return Asn1Boolean.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmL1HealthType from encoded stream.
   */
  public static midiAlmL1HealthType fromPerUnaligned(byte[] encodedBytes) {
    midiAlmL1HealthType result = new midiAlmL1HealthType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmL1HealthType from encoded stream.
   */
  public static midiAlmL1HealthType fromPerAligned(byte[] encodedBytes) {
    midiAlmL1HealthType result = new midiAlmL1HealthType();
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
    return "midiAlmL1HealthType = " + getValue() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmL2HealthType extends Asn1Boolean {
  //

  private static final Asn1Tag TAG_midiAlmL2HealthType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmL2HealthType() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmL2HealthType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmL2HealthType != null) {
      return ImmutableList.of(TAG_midiAlmL2HealthType);
    } else {
      return Asn1Boolean.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmL2HealthType from encoded stream.
   */
  public static midiAlmL2HealthType fromPerUnaligned(byte[] encodedBytes) {
    midiAlmL2HealthType result = new midiAlmL2HealthType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmL2HealthType from encoded stream.
   */
  public static midiAlmL2HealthType fromPerAligned(byte[] encodedBytes) {
    midiAlmL2HealthType result = new midiAlmL2HealthType();
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
    return "midiAlmL2HealthType = " + getValue() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class midiAlmL5HealthType extends Asn1Boolean {
  //

  private static final Asn1Tag TAG_midiAlmL5HealthType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public midiAlmL5HealthType() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_midiAlmL5HealthType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_midiAlmL5HealthType != null) {
      return ImmutableList.of(TAG_midiAlmL5HealthType);
    } else {
      return Asn1Boolean.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new midiAlmL5HealthType from encoded stream.
   */
  public static midiAlmL5HealthType fromPerUnaligned(byte[] encodedBytes) {
    midiAlmL5HealthType result = new midiAlmL5HealthType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new midiAlmL5HealthType from encoded stream.
   */
  public static midiAlmL5HealthType fromPerAligned(byte[] encodedBytes) {
    midiAlmL5HealthType result = new midiAlmL5HealthType();
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
    return "midiAlmL5HealthType = " + getValue() + ";\n";
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
    builder.append("Almanac_MidiAlmanacSet = {\n");
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
