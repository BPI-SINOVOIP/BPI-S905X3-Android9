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
public  class UTCModel extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_UTCModel
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public UTCModel() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_UTCModel;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_UTCModel != null) {
      return ImmutableList.of(TAG_UTCModel);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new UTCModel from encoded stream.
   */
  public static UTCModel fromPerUnaligned(byte[] encodedBytes) {
    UTCModel result = new UTCModel();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new UTCModel from encoded stream.
   */
  public static UTCModel fromPerAligned(byte[] encodedBytes) {
    UTCModel result = new UTCModel();
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

  
  private UTCModel.utcA1Type utcA1_;
  public UTCModel.utcA1Type getUtcA1() {
    return utcA1_;
  }
  /**
   * @throws ClassCastException if value is not a UTCModel.utcA1Type
   */
  public void setUtcA1(Asn1Object value) {
    this.utcA1_ = (UTCModel.utcA1Type) value;
  }
  public UTCModel.utcA1Type setUtcA1ToNewInstance() {
    utcA1_ = new UTCModel.utcA1Type();
    return utcA1_;
  }
  
  private UTCModel.utcA0Type utcA0_;
  public UTCModel.utcA0Type getUtcA0() {
    return utcA0_;
  }
  /**
   * @throws ClassCastException if value is not a UTCModel.utcA0Type
   */
  public void setUtcA0(Asn1Object value) {
    this.utcA0_ = (UTCModel.utcA0Type) value;
  }
  public UTCModel.utcA0Type setUtcA0ToNewInstance() {
    utcA0_ = new UTCModel.utcA0Type();
    return utcA0_;
  }
  
  private UTCModel.utcTotType utcTot_;
  public UTCModel.utcTotType getUtcTot() {
    return utcTot_;
  }
  /**
   * @throws ClassCastException if value is not a UTCModel.utcTotType
   */
  public void setUtcTot(Asn1Object value) {
    this.utcTot_ = (UTCModel.utcTotType) value;
  }
  public UTCModel.utcTotType setUtcTotToNewInstance() {
    utcTot_ = new UTCModel.utcTotType();
    return utcTot_;
  }
  
  private UTCModel.utcWNtType utcWNt_;
  public UTCModel.utcWNtType getUtcWNt() {
    return utcWNt_;
  }
  /**
   * @throws ClassCastException if value is not a UTCModel.utcWNtType
   */
  public void setUtcWNt(Asn1Object value) {
    this.utcWNt_ = (UTCModel.utcWNtType) value;
  }
  public UTCModel.utcWNtType setUtcWNtToNewInstance() {
    utcWNt_ = new UTCModel.utcWNtType();
    return utcWNt_;
  }
  
  private UTCModel.utcDeltaTlsType utcDeltaTls_;
  public UTCModel.utcDeltaTlsType getUtcDeltaTls() {
    return utcDeltaTls_;
  }
  /**
   * @throws ClassCastException if value is not a UTCModel.utcDeltaTlsType
   */
  public void setUtcDeltaTls(Asn1Object value) {
    this.utcDeltaTls_ = (UTCModel.utcDeltaTlsType) value;
  }
  public UTCModel.utcDeltaTlsType setUtcDeltaTlsToNewInstance() {
    utcDeltaTls_ = new UTCModel.utcDeltaTlsType();
    return utcDeltaTls_;
  }
  
  private UTCModel.utcWNlsfType utcWNlsf_;
  public UTCModel.utcWNlsfType getUtcWNlsf() {
    return utcWNlsf_;
  }
  /**
   * @throws ClassCastException if value is not a UTCModel.utcWNlsfType
   */
  public void setUtcWNlsf(Asn1Object value) {
    this.utcWNlsf_ = (UTCModel.utcWNlsfType) value;
  }
  public UTCModel.utcWNlsfType setUtcWNlsfToNewInstance() {
    utcWNlsf_ = new UTCModel.utcWNlsfType();
    return utcWNlsf_;
  }
  
  private UTCModel.utcDNType utcDN_;
  public UTCModel.utcDNType getUtcDN() {
    return utcDN_;
  }
  /**
   * @throws ClassCastException if value is not a UTCModel.utcDNType
   */
  public void setUtcDN(Asn1Object value) {
    this.utcDN_ = (UTCModel.utcDNType) value;
  }
  public UTCModel.utcDNType setUtcDNToNewInstance() {
    utcDN_ = new UTCModel.utcDNType();
    return utcDN_;
  }
  
  private UTCModel.utcDeltaTlsfType utcDeltaTlsf_;
  public UTCModel.utcDeltaTlsfType getUtcDeltaTlsf() {
    return utcDeltaTlsf_;
  }
  /**
   * @throws ClassCastException if value is not a UTCModel.utcDeltaTlsfType
   */
  public void setUtcDeltaTlsf(Asn1Object value) {
    this.utcDeltaTlsf_ = (UTCModel.utcDeltaTlsfType) value;
  }
  public UTCModel.utcDeltaTlsfType setUtcDeltaTlsfToNewInstance() {
    utcDeltaTlsf_ = new UTCModel.utcDeltaTlsfType();
    return utcDeltaTlsf_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getUtcA1() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getUtcA1();
          }

          @Override public void setToNewInstance() {
            setUtcA1ToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTCModel.utcA1Type.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "utcA1 : "
                    + getUtcA1().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getUtcA0() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getUtcA0();
          }

          @Override public void setToNewInstance() {
            setUtcA0ToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTCModel.utcA0Type.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "utcA0 : "
                    + getUtcA0().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 2);

          @Override public boolean isExplicitlySet() {
            return getUtcTot() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getUtcTot();
          }

          @Override public void setToNewInstance() {
            setUtcTotToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTCModel.utcTotType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "utcTot : "
                    + getUtcTot().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 3);

          @Override public boolean isExplicitlySet() {
            return getUtcWNt() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getUtcWNt();
          }

          @Override public void setToNewInstance() {
            setUtcWNtToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTCModel.utcWNtType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "utcWNt : "
                    + getUtcWNt().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 4);

          @Override public boolean isExplicitlySet() {
            return getUtcDeltaTls() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getUtcDeltaTls();
          }

          @Override public void setToNewInstance() {
            setUtcDeltaTlsToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTCModel.utcDeltaTlsType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "utcDeltaTls : "
                    + getUtcDeltaTls().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 5);

          @Override public boolean isExplicitlySet() {
            return getUtcWNlsf() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getUtcWNlsf();
          }

          @Override public void setToNewInstance() {
            setUtcWNlsfToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTCModel.utcWNlsfType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "utcWNlsf : "
                    + getUtcWNlsf().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 6);

          @Override public boolean isExplicitlySet() {
            return getUtcDN() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getUtcDN();
          }

          @Override public void setToNewInstance() {
            setUtcDNToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTCModel.utcDNType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "utcDN : "
                    + getUtcDN().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 7);

          @Override public boolean isExplicitlySet() {
            return getUtcDeltaTlsf() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getUtcDeltaTlsf();
          }

          @Override public void setToNewInstance() {
            setUtcDeltaTlsfToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTCModel.utcDeltaTlsfType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "utcDeltaTlsf : "
                    + getUtcDeltaTlsf().toIndentedString(indent);
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
public static class utcA1Type extends Asn1Integer {
  //

  private static final Asn1Tag TAG_utcA1Type
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public utcA1Type() {
    super();
    setValueRange("-8388608", "8388607");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_utcA1Type;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_utcA1Type != null) {
      return ImmutableList.of(TAG_utcA1Type);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new utcA1Type from encoded stream.
   */
  public static utcA1Type fromPerUnaligned(byte[] encodedBytes) {
    utcA1Type result = new utcA1Type();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new utcA1Type from encoded stream.
   */
  public static utcA1Type fromPerAligned(byte[] encodedBytes) {
    utcA1Type result = new utcA1Type();
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
    return "utcA1Type = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class utcA0Type extends Asn1Integer {
  //

  private static final Asn1Tag TAG_utcA0Type
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public utcA0Type() {
    super();
    setValueRange("-2147483648", "2147483647");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_utcA0Type;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_utcA0Type != null) {
      return ImmutableList.of(TAG_utcA0Type);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new utcA0Type from encoded stream.
   */
  public static utcA0Type fromPerUnaligned(byte[] encodedBytes) {
    utcA0Type result = new utcA0Type();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new utcA0Type from encoded stream.
   */
  public static utcA0Type fromPerAligned(byte[] encodedBytes) {
    utcA0Type result = new utcA0Type();
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
    return "utcA0Type = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class utcTotType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_utcTotType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public utcTotType() {
    super();
    setValueRange("0", "255");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_utcTotType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_utcTotType != null) {
      return ImmutableList.of(TAG_utcTotType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new utcTotType from encoded stream.
   */
  public static utcTotType fromPerUnaligned(byte[] encodedBytes) {
    utcTotType result = new utcTotType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new utcTotType from encoded stream.
   */
  public static utcTotType fromPerAligned(byte[] encodedBytes) {
    utcTotType result = new utcTotType();
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
    return "utcTotType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class utcWNtType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_utcWNtType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public utcWNtType() {
    super();
    setValueRange("0", "255");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_utcWNtType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_utcWNtType != null) {
      return ImmutableList.of(TAG_utcWNtType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new utcWNtType from encoded stream.
   */
  public static utcWNtType fromPerUnaligned(byte[] encodedBytes) {
    utcWNtType result = new utcWNtType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new utcWNtType from encoded stream.
   */
  public static utcWNtType fromPerAligned(byte[] encodedBytes) {
    utcWNtType result = new utcWNtType();
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
    return "utcWNtType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class utcDeltaTlsType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_utcDeltaTlsType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public utcDeltaTlsType() {
    super();
    setValueRange("-128", "127");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_utcDeltaTlsType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_utcDeltaTlsType != null) {
      return ImmutableList.of(TAG_utcDeltaTlsType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new utcDeltaTlsType from encoded stream.
   */
  public static utcDeltaTlsType fromPerUnaligned(byte[] encodedBytes) {
    utcDeltaTlsType result = new utcDeltaTlsType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new utcDeltaTlsType from encoded stream.
   */
  public static utcDeltaTlsType fromPerAligned(byte[] encodedBytes) {
    utcDeltaTlsType result = new utcDeltaTlsType();
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
    return "utcDeltaTlsType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class utcWNlsfType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_utcWNlsfType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public utcWNlsfType() {
    super();
    setValueRange("0", "255");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_utcWNlsfType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_utcWNlsfType != null) {
      return ImmutableList.of(TAG_utcWNlsfType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new utcWNlsfType from encoded stream.
   */
  public static utcWNlsfType fromPerUnaligned(byte[] encodedBytes) {
    utcWNlsfType result = new utcWNlsfType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new utcWNlsfType from encoded stream.
   */
  public static utcWNlsfType fromPerAligned(byte[] encodedBytes) {
    utcWNlsfType result = new utcWNlsfType();
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
    return "utcWNlsfType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class utcDNType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_utcDNType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public utcDNType() {
    super();
    setValueRange("-128", "127");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_utcDNType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_utcDNType != null) {
      return ImmutableList.of(TAG_utcDNType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new utcDNType from encoded stream.
   */
  public static utcDNType fromPerUnaligned(byte[] encodedBytes) {
    utcDNType result = new utcDNType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new utcDNType from encoded stream.
   */
  public static utcDNType fromPerAligned(byte[] encodedBytes) {
    utcDNType result = new utcDNType();
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
    return "utcDNType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class utcDeltaTlsfType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_utcDeltaTlsfType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public utcDeltaTlsfType() {
    super();
    setValueRange("-128", "127");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_utcDeltaTlsfType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_utcDeltaTlsfType != null) {
      return ImmutableList.of(TAG_utcDeltaTlsfType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new utcDeltaTlsfType from encoded stream.
   */
  public static utcDeltaTlsfType fromPerUnaligned(byte[] encodedBytes) {
    utcDeltaTlsfType result = new utcDeltaTlsfType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new utcDeltaTlsfType from encoded stream.
   */
  public static utcDeltaTlsfType fromPerAligned(byte[] encodedBytes) {
    utcDeltaTlsfType result = new utcDeltaTlsfType();
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
    return "utcDeltaTlsfType = " + getInteger() + ";\n";
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
    builder.append("UTCModel = {\n");
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
