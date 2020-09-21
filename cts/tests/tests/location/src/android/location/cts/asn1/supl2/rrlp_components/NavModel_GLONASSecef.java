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
public  class NavModel_GLONASSecef extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_NavModel_GLONASSecef
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public NavModel_GLONASSecef() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_NavModel_GLONASSecef;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_NavModel_GLONASSecef != null) {
      return ImmutableList.of(TAG_NavModel_GLONASSecef);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new NavModel_GLONASSecef from encoded stream.
   */
  public static NavModel_GLONASSecef fromPerUnaligned(byte[] encodedBytes) {
    NavModel_GLONASSecef result = new NavModel_GLONASSecef();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new NavModel_GLONASSecef from encoded stream.
   */
  public static NavModel_GLONASSecef fromPerAligned(byte[] encodedBytes) {
    NavModel_GLONASSecef result = new NavModel_GLONASSecef();
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

  
  private NavModel_GLONASSecef.gloEnType gloEn_;
  public NavModel_GLONASSecef.gloEnType getGloEn() {
    return gloEn_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloEnType
   */
  public void setGloEn(Asn1Object value) {
    this.gloEn_ = (NavModel_GLONASSecef.gloEnType) value;
  }
  public NavModel_GLONASSecef.gloEnType setGloEnToNewInstance() {
    gloEn_ = new NavModel_GLONASSecef.gloEnType();
    return gloEn_;
  }
  
  private NavModel_GLONASSecef.gloP1Type gloP1_;
  public NavModel_GLONASSecef.gloP1Type getGloP1() {
    return gloP1_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloP1Type
   */
  public void setGloP1(Asn1Object value) {
    this.gloP1_ = (NavModel_GLONASSecef.gloP1Type) value;
  }
  public NavModel_GLONASSecef.gloP1Type setGloP1ToNewInstance() {
    gloP1_ = new NavModel_GLONASSecef.gloP1Type();
    return gloP1_;
  }
  
  private NavModel_GLONASSecef.gloP2Type gloP2_;
  public NavModel_GLONASSecef.gloP2Type getGloP2() {
    return gloP2_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloP2Type
   */
  public void setGloP2(Asn1Object value) {
    this.gloP2_ = (NavModel_GLONASSecef.gloP2Type) value;
  }
  public NavModel_GLONASSecef.gloP2Type setGloP2ToNewInstance() {
    gloP2_ = new NavModel_GLONASSecef.gloP2Type();
    return gloP2_;
  }
  
  private NavModel_GLONASSecef.gloMType gloM_;
  public NavModel_GLONASSecef.gloMType getGloM() {
    return gloM_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloMType
   */
  public void setGloM(Asn1Object value) {
    this.gloM_ = (NavModel_GLONASSecef.gloMType) value;
  }
  public NavModel_GLONASSecef.gloMType setGloMToNewInstance() {
    gloM_ = new NavModel_GLONASSecef.gloMType();
    return gloM_;
  }
  
  private NavModel_GLONASSecef.gloXType gloX_;
  public NavModel_GLONASSecef.gloXType getGloX() {
    return gloX_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloXType
   */
  public void setGloX(Asn1Object value) {
    this.gloX_ = (NavModel_GLONASSecef.gloXType) value;
  }
  public NavModel_GLONASSecef.gloXType setGloXToNewInstance() {
    gloX_ = new NavModel_GLONASSecef.gloXType();
    return gloX_;
  }
  
  private NavModel_GLONASSecef.gloXdotType gloXdot_;
  public NavModel_GLONASSecef.gloXdotType getGloXdot() {
    return gloXdot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloXdotType
   */
  public void setGloXdot(Asn1Object value) {
    this.gloXdot_ = (NavModel_GLONASSecef.gloXdotType) value;
  }
  public NavModel_GLONASSecef.gloXdotType setGloXdotToNewInstance() {
    gloXdot_ = new NavModel_GLONASSecef.gloXdotType();
    return gloXdot_;
  }
  
  private NavModel_GLONASSecef.gloXdotdotType gloXdotdot_;
  public NavModel_GLONASSecef.gloXdotdotType getGloXdotdot() {
    return gloXdotdot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloXdotdotType
   */
  public void setGloXdotdot(Asn1Object value) {
    this.gloXdotdot_ = (NavModel_GLONASSecef.gloXdotdotType) value;
  }
  public NavModel_GLONASSecef.gloXdotdotType setGloXdotdotToNewInstance() {
    gloXdotdot_ = new NavModel_GLONASSecef.gloXdotdotType();
    return gloXdotdot_;
  }
  
  private NavModel_GLONASSecef.gloYType gloY_;
  public NavModel_GLONASSecef.gloYType getGloY() {
    return gloY_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloYType
   */
  public void setGloY(Asn1Object value) {
    this.gloY_ = (NavModel_GLONASSecef.gloYType) value;
  }
  public NavModel_GLONASSecef.gloYType setGloYToNewInstance() {
    gloY_ = new NavModel_GLONASSecef.gloYType();
    return gloY_;
  }
  
  private NavModel_GLONASSecef.gloYdotType gloYdot_;
  public NavModel_GLONASSecef.gloYdotType getGloYdot() {
    return gloYdot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloYdotType
   */
  public void setGloYdot(Asn1Object value) {
    this.gloYdot_ = (NavModel_GLONASSecef.gloYdotType) value;
  }
  public NavModel_GLONASSecef.gloYdotType setGloYdotToNewInstance() {
    gloYdot_ = new NavModel_GLONASSecef.gloYdotType();
    return gloYdot_;
  }
  
  private NavModel_GLONASSecef.gloYdotdotType gloYdotdot_;
  public NavModel_GLONASSecef.gloYdotdotType getGloYdotdot() {
    return gloYdotdot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloYdotdotType
   */
  public void setGloYdotdot(Asn1Object value) {
    this.gloYdotdot_ = (NavModel_GLONASSecef.gloYdotdotType) value;
  }
  public NavModel_GLONASSecef.gloYdotdotType setGloYdotdotToNewInstance() {
    gloYdotdot_ = new NavModel_GLONASSecef.gloYdotdotType();
    return gloYdotdot_;
  }
  
  private NavModel_GLONASSecef.gloZType gloZ_;
  public NavModel_GLONASSecef.gloZType getGloZ() {
    return gloZ_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloZType
   */
  public void setGloZ(Asn1Object value) {
    this.gloZ_ = (NavModel_GLONASSecef.gloZType) value;
  }
  public NavModel_GLONASSecef.gloZType setGloZToNewInstance() {
    gloZ_ = new NavModel_GLONASSecef.gloZType();
    return gloZ_;
  }
  
  private NavModel_GLONASSecef.gloZdotType gloZdot_;
  public NavModel_GLONASSecef.gloZdotType getGloZdot() {
    return gloZdot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloZdotType
   */
  public void setGloZdot(Asn1Object value) {
    this.gloZdot_ = (NavModel_GLONASSecef.gloZdotType) value;
  }
  public NavModel_GLONASSecef.gloZdotType setGloZdotToNewInstance() {
    gloZdot_ = new NavModel_GLONASSecef.gloZdotType();
    return gloZdot_;
  }
  
  private NavModel_GLONASSecef.gloZdotdotType gloZdotdot_;
  public NavModel_GLONASSecef.gloZdotdotType getGloZdotdot() {
    return gloZdotdot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_GLONASSecef.gloZdotdotType
   */
  public void setGloZdotdot(Asn1Object value) {
    this.gloZdotdot_ = (NavModel_GLONASSecef.gloZdotdotType) value;
  }
  public NavModel_GLONASSecef.gloZdotdotType setGloZdotdotToNewInstance() {
    gloZdotdot_ = new NavModel_GLONASSecef.gloZdotdotType();
    return gloZdotdot_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getGloEn() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloEn();
          }

          @Override public void setToNewInstance() {
            setGloEnToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloEnType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloEn : "
                    + getGloEn().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getGloP1() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloP1();
          }

          @Override public void setToNewInstance() {
            setGloP1ToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloP1Type.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloP1 : "
                    + getGloP1().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 2);

          @Override public boolean isExplicitlySet() {
            return getGloP2() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloP2();
          }

          @Override public void setToNewInstance() {
            setGloP2ToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloP2Type.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloP2 : "
                    + getGloP2().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 3);

          @Override public boolean isExplicitlySet() {
            return getGloM() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloM();
          }

          @Override public void setToNewInstance() {
            setGloMToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloMType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloM : "
                    + getGloM().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 4);

          @Override public boolean isExplicitlySet() {
            return getGloX() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloX();
          }

          @Override public void setToNewInstance() {
            setGloXToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloXType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloX : "
                    + getGloX().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 5);

          @Override public boolean isExplicitlySet() {
            return getGloXdot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloXdot();
          }

          @Override public void setToNewInstance() {
            setGloXdotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloXdotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloXdot : "
                    + getGloXdot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 6);

          @Override public boolean isExplicitlySet() {
            return getGloXdotdot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloXdotdot();
          }

          @Override public void setToNewInstance() {
            setGloXdotdotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloXdotdotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloXdotdot : "
                    + getGloXdotdot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 7);

          @Override public boolean isExplicitlySet() {
            return getGloY() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloY();
          }

          @Override public void setToNewInstance() {
            setGloYToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloYType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloY : "
                    + getGloY().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 8);

          @Override public boolean isExplicitlySet() {
            return getGloYdot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloYdot();
          }

          @Override public void setToNewInstance() {
            setGloYdotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloYdotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloYdot : "
                    + getGloYdot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 9);

          @Override public boolean isExplicitlySet() {
            return getGloYdotdot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloYdotdot();
          }

          @Override public void setToNewInstance() {
            setGloYdotdotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloYdotdotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloYdotdot : "
                    + getGloYdotdot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 10);

          @Override public boolean isExplicitlySet() {
            return getGloZ() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloZ();
          }

          @Override public void setToNewInstance() {
            setGloZToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloZType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloZ : "
                    + getGloZ().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 11);

          @Override public boolean isExplicitlySet() {
            return getGloZdot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloZdot();
          }

          @Override public void setToNewInstance() {
            setGloZdotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloZdotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloZdot : "
                    + getGloZdot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 12);

          @Override public boolean isExplicitlySet() {
            return getGloZdotdot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGloZdotdot();
          }

          @Override public void setToNewInstance() {
            setGloZdotdotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_GLONASSecef.gloZdotdotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gloZdotdot : "
                    + getGloZdotdot().toIndentedString(indent);
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
public static class gloEnType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloEnType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloEnType() {
    super();
    setValueRange("0", "31");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloEnType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloEnType != null) {
      return ImmutableList.of(TAG_gloEnType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloEnType from encoded stream.
   */
  public static gloEnType fromPerUnaligned(byte[] encodedBytes) {
    gloEnType result = new gloEnType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloEnType from encoded stream.
   */
  public static gloEnType fromPerAligned(byte[] encodedBytes) {
    gloEnType result = new gloEnType();
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
    return "gloEnType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloP1Type extends Asn1BitString {
  //

  private static final Asn1Tag TAG_gloP1Type
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloP1Type() {
    super();
    setMinSize(2);
setMaxSize(2);

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloP1Type;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloP1Type != null) {
      return ImmutableList.of(TAG_gloP1Type);
    } else {
      return Asn1BitString.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloP1Type from encoded stream.
   */
  public static gloP1Type fromPerUnaligned(byte[] encodedBytes) {
    gloP1Type result = new gloP1Type();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloP1Type from encoded stream.
   */
  public static gloP1Type fromPerAligned(byte[] encodedBytes) {
    gloP1Type result = new gloP1Type();
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
    return "gloP1Type = " + getValue() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloP2Type extends Asn1Boolean {
  //

  private static final Asn1Tag TAG_gloP2Type
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloP2Type() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloP2Type;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloP2Type != null) {
      return ImmutableList.of(TAG_gloP2Type);
    } else {
      return Asn1Boolean.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloP2Type from encoded stream.
   */
  public static gloP2Type fromPerUnaligned(byte[] encodedBytes) {
    gloP2Type result = new gloP2Type();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloP2Type from encoded stream.
   */
  public static gloP2Type fromPerAligned(byte[] encodedBytes) {
    gloP2Type result = new gloP2Type();
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
    return "gloP2Type = " + getValue() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloMType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloMType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloMType() {
    super();
    setValueRange("0", "3");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloMType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloMType != null) {
      return ImmutableList.of(TAG_gloMType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloMType from encoded stream.
   */
  public static gloMType fromPerUnaligned(byte[] encodedBytes) {
    gloMType result = new gloMType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloMType from encoded stream.
   */
  public static gloMType fromPerAligned(byte[] encodedBytes) {
    gloMType result = new gloMType();
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
    return "gloMType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloXType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloXType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloXType() {
    super();
    setValueRange("-67108864", "67108863");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloXType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloXType != null) {
      return ImmutableList.of(TAG_gloXType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloXType from encoded stream.
   */
  public static gloXType fromPerUnaligned(byte[] encodedBytes) {
    gloXType result = new gloXType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloXType from encoded stream.
   */
  public static gloXType fromPerAligned(byte[] encodedBytes) {
    gloXType result = new gloXType();
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
    return "gloXType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloXdotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloXdotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloXdotType() {
    super();
    setValueRange("-8388608", "8388607");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloXdotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloXdotType != null) {
      return ImmutableList.of(TAG_gloXdotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloXdotType from encoded stream.
   */
  public static gloXdotType fromPerUnaligned(byte[] encodedBytes) {
    gloXdotType result = new gloXdotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloXdotType from encoded stream.
   */
  public static gloXdotType fromPerAligned(byte[] encodedBytes) {
    gloXdotType result = new gloXdotType();
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
    return "gloXdotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloXdotdotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloXdotdotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloXdotdotType() {
    super();
    setValueRange("-16", "15");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloXdotdotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloXdotdotType != null) {
      return ImmutableList.of(TAG_gloXdotdotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloXdotdotType from encoded stream.
   */
  public static gloXdotdotType fromPerUnaligned(byte[] encodedBytes) {
    gloXdotdotType result = new gloXdotdotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloXdotdotType from encoded stream.
   */
  public static gloXdotdotType fromPerAligned(byte[] encodedBytes) {
    gloXdotdotType result = new gloXdotdotType();
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
    return "gloXdotdotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloYType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloYType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloYType() {
    super();
    setValueRange("-67108864", "67108863");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloYType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloYType != null) {
      return ImmutableList.of(TAG_gloYType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloYType from encoded stream.
   */
  public static gloYType fromPerUnaligned(byte[] encodedBytes) {
    gloYType result = new gloYType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloYType from encoded stream.
   */
  public static gloYType fromPerAligned(byte[] encodedBytes) {
    gloYType result = new gloYType();
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
    return "gloYType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloYdotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloYdotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloYdotType() {
    super();
    setValueRange("-8388608", "8388607");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloYdotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloYdotType != null) {
      return ImmutableList.of(TAG_gloYdotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloYdotType from encoded stream.
   */
  public static gloYdotType fromPerUnaligned(byte[] encodedBytes) {
    gloYdotType result = new gloYdotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloYdotType from encoded stream.
   */
  public static gloYdotType fromPerAligned(byte[] encodedBytes) {
    gloYdotType result = new gloYdotType();
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
    return "gloYdotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloYdotdotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloYdotdotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloYdotdotType() {
    super();
    setValueRange("-16", "15");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloYdotdotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloYdotdotType != null) {
      return ImmutableList.of(TAG_gloYdotdotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloYdotdotType from encoded stream.
   */
  public static gloYdotdotType fromPerUnaligned(byte[] encodedBytes) {
    gloYdotdotType result = new gloYdotdotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloYdotdotType from encoded stream.
   */
  public static gloYdotdotType fromPerAligned(byte[] encodedBytes) {
    gloYdotdotType result = new gloYdotdotType();
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
    return "gloYdotdotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloZType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloZType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloZType() {
    super();
    setValueRange("-67108864", "67108863");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloZType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloZType != null) {
      return ImmutableList.of(TAG_gloZType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloZType from encoded stream.
   */
  public static gloZType fromPerUnaligned(byte[] encodedBytes) {
    gloZType result = new gloZType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloZType from encoded stream.
   */
  public static gloZType fromPerAligned(byte[] encodedBytes) {
    gloZType result = new gloZType();
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
    return "gloZType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloZdotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloZdotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloZdotType() {
    super();
    setValueRange("-8388608", "8388607");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloZdotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloZdotType != null) {
      return ImmutableList.of(TAG_gloZdotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloZdotType from encoded stream.
   */
  public static gloZdotType fromPerUnaligned(byte[] encodedBytes) {
    gloZdotType result = new gloZdotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloZdotType from encoded stream.
   */
  public static gloZdotType fromPerAligned(byte[] encodedBytes) {
    gloZdotType result = new gloZdotType();
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
    return "gloZdotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class gloZdotdotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_gloZdotdotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public gloZdotdotType() {
    super();
    setValueRange("-16", "15");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_gloZdotdotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_gloZdotdotType != null) {
      return ImmutableList.of(TAG_gloZdotdotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new gloZdotdotType from encoded stream.
   */
  public static gloZdotdotType fromPerUnaligned(byte[] encodedBytes) {
    gloZdotdotType result = new gloZdotdotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new gloZdotdotType from encoded stream.
   */
  public static gloZdotdotType fromPerAligned(byte[] encodedBytes) {
    gloZdotdotType result = new gloZdotdotType();
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
    return "gloZdotdotType = " + getInteger() + ";\n";
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
    builder.append("NavModel_GLONASSecef = {\n");
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
