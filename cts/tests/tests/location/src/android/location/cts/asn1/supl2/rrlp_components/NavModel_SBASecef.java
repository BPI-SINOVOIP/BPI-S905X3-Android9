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
public  class NavModel_SBASecef extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_NavModel_SBASecef
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public NavModel_SBASecef() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_NavModel_SBASecef;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_NavModel_SBASecef != null) {
      return ImmutableList.of(TAG_NavModel_SBASecef);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new NavModel_SBASecef from encoded stream.
   */
  public static NavModel_SBASecef fromPerUnaligned(byte[] encodedBytes) {
    NavModel_SBASecef result = new NavModel_SBASecef();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new NavModel_SBASecef from encoded stream.
   */
  public static NavModel_SBASecef fromPerAligned(byte[] encodedBytes) {
    NavModel_SBASecef result = new NavModel_SBASecef();
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

  
  private NavModel_SBASecef.sbasToType sbasTo_;
  public NavModel_SBASecef.sbasToType getSbasTo() {
    return sbasTo_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasToType
   */
  public void setSbasTo(Asn1Object value) {
    this.sbasTo_ = (NavModel_SBASecef.sbasToType) value;
  }
  public NavModel_SBASecef.sbasToType setSbasToToNewInstance() {
    sbasTo_ = new NavModel_SBASecef.sbasToType();
    return sbasTo_;
  }
  
  private NavModel_SBASecef.sbasAccuracyType sbasAccuracy_;
  public NavModel_SBASecef.sbasAccuracyType getSbasAccuracy() {
    return sbasAccuracy_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasAccuracyType
   */
  public void setSbasAccuracy(Asn1Object value) {
    this.sbasAccuracy_ = (NavModel_SBASecef.sbasAccuracyType) value;
  }
  public NavModel_SBASecef.sbasAccuracyType setSbasAccuracyToNewInstance() {
    sbasAccuracy_ = new NavModel_SBASecef.sbasAccuracyType();
    return sbasAccuracy_;
  }
  
  private NavModel_SBASecef.sbasXgType sbasXg_;
  public NavModel_SBASecef.sbasXgType getSbasXg() {
    return sbasXg_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasXgType
   */
  public void setSbasXg(Asn1Object value) {
    this.sbasXg_ = (NavModel_SBASecef.sbasXgType) value;
  }
  public NavModel_SBASecef.sbasXgType setSbasXgToNewInstance() {
    sbasXg_ = new NavModel_SBASecef.sbasXgType();
    return sbasXg_;
  }
  
  private NavModel_SBASecef.sbasYgType sbasYg_;
  public NavModel_SBASecef.sbasYgType getSbasYg() {
    return sbasYg_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasYgType
   */
  public void setSbasYg(Asn1Object value) {
    this.sbasYg_ = (NavModel_SBASecef.sbasYgType) value;
  }
  public NavModel_SBASecef.sbasYgType setSbasYgToNewInstance() {
    sbasYg_ = new NavModel_SBASecef.sbasYgType();
    return sbasYg_;
  }
  
  private NavModel_SBASecef.sbasZgType sbasZg_;
  public NavModel_SBASecef.sbasZgType getSbasZg() {
    return sbasZg_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasZgType
   */
  public void setSbasZg(Asn1Object value) {
    this.sbasZg_ = (NavModel_SBASecef.sbasZgType) value;
  }
  public NavModel_SBASecef.sbasZgType setSbasZgToNewInstance() {
    sbasZg_ = new NavModel_SBASecef.sbasZgType();
    return sbasZg_;
  }
  
  private NavModel_SBASecef.sbasXgDotType sbasXgDot_;
  public NavModel_SBASecef.sbasXgDotType getSbasXgDot() {
    return sbasXgDot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasXgDotType
   */
  public void setSbasXgDot(Asn1Object value) {
    this.sbasXgDot_ = (NavModel_SBASecef.sbasXgDotType) value;
  }
  public NavModel_SBASecef.sbasXgDotType setSbasXgDotToNewInstance() {
    sbasXgDot_ = new NavModel_SBASecef.sbasXgDotType();
    return sbasXgDot_;
  }
  
  private NavModel_SBASecef.sbasYgDotType sbasYgDot_;
  public NavModel_SBASecef.sbasYgDotType getSbasYgDot() {
    return sbasYgDot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasYgDotType
   */
  public void setSbasYgDot(Asn1Object value) {
    this.sbasYgDot_ = (NavModel_SBASecef.sbasYgDotType) value;
  }
  public NavModel_SBASecef.sbasYgDotType setSbasYgDotToNewInstance() {
    sbasYgDot_ = new NavModel_SBASecef.sbasYgDotType();
    return sbasYgDot_;
  }
  
  private NavModel_SBASecef.sbasZgDotType sbasZgDot_;
  public NavModel_SBASecef.sbasZgDotType getSbasZgDot() {
    return sbasZgDot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasZgDotType
   */
  public void setSbasZgDot(Asn1Object value) {
    this.sbasZgDot_ = (NavModel_SBASecef.sbasZgDotType) value;
  }
  public NavModel_SBASecef.sbasZgDotType setSbasZgDotToNewInstance() {
    sbasZgDot_ = new NavModel_SBASecef.sbasZgDotType();
    return sbasZgDot_;
  }
  
  private NavModel_SBASecef.sbasXgDotDotType sbasXgDotDot_;
  public NavModel_SBASecef.sbasXgDotDotType getSbasXgDotDot() {
    return sbasXgDotDot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasXgDotDotType
   */
  public void setSbasXgDotDot(Asn1Object value) {
    this.sbasXgDotDot_ = (NavModel_SBASecef.sbasXgDotDotType) value;
  }
  public NavModel_SBASecef.sbasXgDotDotType setSbasXgDotDotToNewInstance() {
    sbasXgDotDot_ = new NavModel_SBASecef.sbasXgDotDotType();
    return sbasXgDotDot_;
  }
  
  private NavModel_SBASecef.sbagYgDotDotType sbagYgDotDot_;
  public NavModel_SBASecef.sbagYgDotDotType getSbagYgDotDot() {
    return sbagYgDotDot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbagYgDotDotType
   */
  public void setSbagYgDotDot(Asn1Object value) {
    this.sbagYgDotDot_ = (NavModel_SBASecef.sbagYgDotDotType) value;
  }
  public NavModel_SBASecef.sbagYgDotDotType setSbagYgDotDotToNewInstance() {
    sbagYgDotDot_ = new NavModel_SBASecef.sbagYgDotDotType();
    return sbagYgDotDot_;
  }
  
  private NavModel_SBASecef.sbasZgDotDotType sbasZgDotDot_;
  public NavModel_SBASecef.sbasZgDotDotType getSbasZgDotDot() {
    return sbasZgDotDot_;
  }
  /**
   * @throws ClassCastException if value is not a NavModel_SBASecef.sbasZgDotDotType
   */
  public void setSbasZgDotDot(Asn1Object value) {
    this.sbasZgDotDot_ = (NavModel_SBASecef.sbasZgDotDotType) value;
  }
  public NavModel_SBASecef.sbasZgDotDotType setSbasZgDotDotToNewInstance() {
    sbasZgDotDot_ = new NavModel_SBASecef.sbasZgDotDotType();
    return sbasZgDotDot_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getSbasTo() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasTo();
          }

          @Override public void setToNewInstance() {
            setSbasToToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasToType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasTo : "
                    + getSbasTo().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getSbasAccuracy() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasAccuracy();
          }

          @Override public void setToNewInstance() {
            setSbasAccuracyToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasAccuracyType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasAccuracy : "
                    + getSbasAccuracy().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 2);

          @Override public boolean isExplicitlySet() {
            return getSbasXg() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasXg();
          }

          @Override public void setToNewInstance() {
            setSbasXgToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasXgType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasXg : "
                    + getSbasXg().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 3);

          @Override public boolean isExplicitlySet() {
            return getSbasYg() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasYg();
          }

          @Override public void setToNewInstance() {
            setSbasYgToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasYgType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasYg : "
                    + getSbasYg().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 4);

          @Override public boolean isExplicitlySet() {
            return getSbasZg() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasZg();
          }

          @Override public void setToNewInstance() {
            setSbasZgToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasZgType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasZg : "
                    + getSbasZg().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 5);

          @Override public boolean isExplicitlySet() {
            return getSbasXgDot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasXgDot();
          }

          @Override public void setToNewInstance() {
            setSbasXgDotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasXgDotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasXgDot : "
                    + getSbasXgDot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 6);

          @Override public boolean isExplicitlySet() {
            return getSbasYgDot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasYgDot();
          }

          @Override public void setToNewInstance() {
            setSbasYgDotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasYgDotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasYgDot : "
                    + getSbasYgDot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 7);

          @Override public boolean isExplicitlySet() {
            return getSbasZgDot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasZgDot();
          }

          @Override public void setToNewInstance() {
            setSbasZgDotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasZgDotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasZgDot : "
                    + getSbasZgDot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 8);

          @Override public boolean isExplicitlySet() {
            return getSbasXgDotDot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasXgDotDot();
          }

          @Override public void setToNewInstance() {
            setSbasXgDotDotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasXgDotDotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasXgDotDot : "
                    + getSbasXgDotDot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 9);

          @Override public boolean isExplicitlySet() {
            return getSbagYgDotDot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbagYgDotDot();
          }

          @Override public void setToNewInstance() {
            setSbagYgDotDotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbagYgDotDotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbagYgDotDot : "
                    + getSbagYgDotDot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 10);

          @Override public boolean isExplicitlySet() {
            return getSbasZgDotDot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSbasZgDotDot();
          }

          @Override public void setToNewInstance() {
            setSbasZgDotDotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? NavModel_SBASecef.sbasZgDotDotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sbasZgDotDot : "
                    + getSbasZgDotDot().toIndentedString(indent);
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
public static class sbasToType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbasToType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasToType() {
    super();
    setValueRange("0", "5399");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasToType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasToType != null) {
      return ImmutableList.of(TAG_sbasToType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasToType from encoded stream.
   */
  public static sbasToType fromPerUnaligned(byte[] encodedBytes) {
    sbasToType result = new sbasToType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasToType from encoded stream.
   */
  public static sbasToType fromPerAligned(byte[] encodedBytes) {
    sbasToType result = new sbasToType();
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
    return "sbasToType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbasAccuracyType extends Asn1BitString {
  //

  private static final Asn1Tag TAG_sbasAccuracyType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasAccuracyType() {
    super();
    setMinSize(4);
setMaxSize(4);

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasAccuracyType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasAccuracyType != null) {
      return ImmutableList.of(TAG_sbasAccuracyType);
    } else {
      return Asn1BitString.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasAccuracyType from encoded stream.
   */
  public static sbasAccuracyType fromPerUnaligned(byte[] encodedBytes) {
    sbasAccuracyType result = new sbasAccuracyType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasAccuracyType from encoded stream.
   */
  public static sbasAccuracyType fromPerAligned(byte[] encodedBytes) {
    sbasAccuracyType result = new sbasAccuracyType();
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
    return "sbasAccuracyType = " + getValue() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbasXgType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbasXgType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasXgType() {
    super();
    setValueRange("-536870912", "536870911");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasXgType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasXgType != null) {
      return ImmutableList.of(TAG_sbasXgType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasXgType from encoded stream.
   */
  public static sbasXgType fromPerUnaligned(byte[] encodedBytes) {
    sbasXgType result = new sbasXgType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasXgType from encoded stream.
   */
  public static sbasXgType fromPerAligned(byte[] encodedBytes) {
    sbasXgType result = new sbasXgType();
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
    return "sbasXgType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbasYgType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbasYgType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasYgType() {
    super();
    setValueRange("-536870912", "536870911");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasYgType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasYgType != null) {
      return ImmutableList.of(TAG_sbasYgType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasYgType from encoded stream.
   */
  public static sbasYgType fromPerUnaligned(byte[] encodedBytes) {
    sbasYgType result = new sbasYgType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasYgType from encoded stream.
   */
  public static sbasYgType fromPerAligned(byte[] encodedBytes) {
    sbasYgType result = new sbasYgType();
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
    return "sbasYgType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbasZgType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbasZgType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasZgType() {
    super();
    setValueRange("-16777216", "16777215");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasZgType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasZgType != null) {
      return ImmutableList.of(TAG_sbasZgType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasZgType from encoded stream.
   */
  public static sbasZgType fromPerUnaligned(byte[] encodedBytes) {
    sbasZgType result = new sbasZgType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasZgType from encoded stream.
   */
  public static sbasZgType fromPerAligned(byte[] encodedBytes) {
    sbasZgType result = new sbasZgType();
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
    return "sbasZgType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbasXgDotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbasXgDotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasXgDotType() {
    super();
    setValueRange("-65536", "65535");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasXgDotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasXgDotType != null) {
      return ImmutableList.of(TAG_sbasXgDotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasXgDotType from encoded stream.
   */
  public static sbasXgDotType fromPerUnaligned(byte[] encodedBytes) {
    sbasXgDotType result = new sbasXgDotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasXgDotType from encoded stream.
   */
  public static sbasXgDotType fromPerAligned(byte[] encodedBytes) {
    sbasXgDotType result = new sbasXgDotType();
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
    return "sbasXgDotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbasYgDotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbasYgDotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasYgDotType() {
    super();
    setValueRange("-65536", "65535");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasYgDotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasYgDotType != null) {
      return ImmutableList.of(TAG_sbasYgDotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasYgDotType from encoded stream.
   */
  public static sbasYgDotType fromPerUnaligned(byte[] encodedBytes) {
    sbasYgDotType result = new sbasYgDotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasYgDotType from encoded stream.
   */
  public static sbasYgDotType fromPerAligned(byte[] encodedBytes) {
    sbasYgDotType result = new sbasYgDotType();
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
    return "sbasYgDotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbasZgDotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbasZgDotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasZgDotType() {
    super();
    setValueRange("-131072", "131071");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasZgDotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasZgDotType != null) {
      return ImmutableList.of(TAG_sbasZgDotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasZgDotType from encoded stream.
   */
  public static sbasZgDotType fromPerUnaligned(byte[] encodedBytes) {
    sbasZgDotType result = new sbasZgDotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasZgDotType from encoded stream.
   */
  public static sbasZgDotType fromPerAligned(byte[] encodedBytes) {
    sbasZgDotType result = new sbasZgDotType();
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
    return "sbasZgDotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbasXgDotDotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbasXgDotDotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasXgDotDotType() {
    super();
    setValueRange("-512", "511");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasXgDotDotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasXgDotDotType != null) {
      return ImmutableList.of(TAG_sbasXgDotDotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasXgDotDotType from encoded stream.
   */
  public static sbasXgDotDotType fromPerUnaligned(byte[] encodedBytes) {
    sbasXgDotDotType result = new sbasXgDotDotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasXgDotDotType from encoded stream.
   */
  public static sbasXgDotDotType fromPerAligned(byte[] encodedBytes) {
    sbasXgDotDotType result = new sbasXgDotDotType();
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
    return "sbasXgDotDotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbagYgDotDotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbagYgDotDotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbagYgDotDotType() {
    super();
    setValueRange("-512", "511");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbagYgDotDotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbagYgDotDotType != null) {
      return ImmutableList.of(TAG_sbagYgDotDotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbagYgDotDotType from encoded stream.
   */
  public static sbagYgDotDotType fromPerUnaligned(byte[] encodedBytes) {
    sbagYgDotDotType result = new sbagYgDotDotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbagYgDotDotType from encoded stream.
   */
  public static sbagYgDotDotType fromPerAligned(byte[] encodedBytes) {
    sbagYgDotDotType result = new sbagYgDotDotType();
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
    return "sbagYgDotDotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sbasZgDotDotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sbasZgDotDotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sbasZgDotDotType() {
    super();
    setValueRange("-512", "511");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sbasZgDotDotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sbasZgDotDotType != null) {
      return ImmutableList.of(TAG_sbasZgDotDotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sbasZgDotDotType from encoded stream.
   */
  public static sbasZgDotDotType fromPerUnaligned(byte[] encodedBytes) {
    sbasZgDotDotType result = new sbasZgDotDotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sbasZgDotDotType from encoded stream.
   */
  public static sbasZgDotDotType fromPerAligned(byte[] encodedBytes) {
    sbasZgDotDotType result = new sbasZgDotDotType();
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
    return "sbasZgDotDotType = " + getInteger() + ";\n";
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
    builder.append("NavModel_SBASecef = {\n");
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
