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

package android.location.cts.asn1.supl2.ver2_ulp_components;

/*
 */


//
//
import android.location.cts.asn1.base.Asn1Choice;
import android.location.cts.asn1.base.Asn1Integer;
import android.location.cts.asn1.base.Asn1Null;
import android.location.cts.asn1.base.Asn1Object;
import android.location.cts.asn1.base.Asn1Sequence;
import android.location.cts.asn1.base.Asn1Tag;
import android.location.cts.asn1.base.BitStream;
import android.location.cts.asn1.base.BitStreamReader;
import android.location.cts.asn1.base.ChoiceComponent;
import android.location.cts.asn1.base.SequenceComponent;
import android.location.cts.asn1.supl2.ulp_components.CellParametersID;
import android.location.cts.asn1.supl2.ulp_components.PrimaryCPICH_Info;
import com.google.common.collect.ImmutableList;
import java.nio.ByteBuffer;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import javax.annotation.Nullable;


/**
*/
public  class UTRAN_GPSReferenceTime extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_UTRAN_GPSReferenceTime
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public UTRAN_GPSReferenceTime() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_UTRAN_GPSReferenceTime;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_UTRAN_GPSReferenceTime != null) {
      return ImmutableList.of(TAG_UTRAN_GPSReferenceTime);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new UTRAN_GPSReferenceTime from encoded stream.
   */
  public static UTRAN_GPSReferenceTime fromPerUnaligned(byte[] encodedBytes) {
    UTRAN_GPSReferenceTime result = new UTRAN_GPSReferenceTime();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new UTRAN_GPSReferenceTime from encoded stream.
   */
  public static UTRAN_GPSReferenceTime fromPerAligned(byte[] encodedBytes) {
    UTRAN_GPSReferenceTime result = new UTRAN_GPSReferenceTime();
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

  
  private UTRAN_GPSReferenceTime.utran_GPSTimingOfCellType utran_GPSTimingOfCell_;
  public UTRAN_GPSReferenceTime.utran_GPSTimingOfCellType getUtran_GPSTimingOfCell() {
    return utran_GPSTimingOfCell_;
  }
  /**
   * @throws ClassCastException if value is not a UTRAN_GPSReferenceTime.utran_GPSTimingOfCellType
   */
  public void setUtran_GPSTimingOfCell(Asn1Object value) {
    this.utran_GPSTimingOfCell_ = (UTRAN_GPSReferenceTime.utran_GPSTimingOfCellType) value;
  }
  public UTRAN_GPSReferenceTime.utran_GPSTimingOfCellType setUtran_GPSTimingOfCellToNewInstance() {
    utran_GPSTimingOfCell_ = new UTRAN_GPSReferenceTime.utran_GPSTimingOfCellType();
    return utran_GPSTimingOfCell_;
  }
  
  private UTRAN_GPSReferenceTime.modeSpecificInfoType modeSpecificInfo_;
  public UTRAN_GPSReferenceTime.modeSpecificInfoType getModeSpecificInfo() {
    return modeSpecificInfo_;
  }
  /**
   * @throws ClassCastException if value is not a UTRAN_GPSReferenceTime.modeSpecificInfoType
   */
  public void setModeSpecificInfo(Asn1Object value) {
    this.modeSpecificInfo_ = (UTRAN_GPSReferenceTime.modeSpecificInfoType) value;
  }
  public UTRAN_GPSReferenceTime.modeSpecificInfoType setModeSpecificInfoToNewInstance() {
    modeSpecificInfo_ = new UTRAN_GPSReferenceTime.modeSpecificInfoType();
    return modeSpecificInfo_;
  }
  
  private UTRAN_GPSReferenceTime.sfnType sfn_;
  public UTRAN_GPSReferenceTime.sfnType getSfn() {
    return sfn_;
  }
  /**
   * @throws ClassCastException if value is not a UTRAN_GPSReferenceTime.sfnType
   */
  public void setSfn(Asn1Object value) {
    this.sfn_ = (UTRAN_GPSReferenceTime.sfnType) value;
  }
  public UTRAN_GPSReferenceTime.sfnType setSfnToNewInstance() {
    sfn_ = new UTRAN_GPSReferenceTime.sfnType();
    return sfn_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getUtran_GPSTimingOfCell() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getUtran_GPSTimingOfCell();
          }

          @Override public void setToNewInstance() {
            setUtran_GPSTimingOfCellToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTRAN_GPSReferenceTime.utran_GPSTimingOfCellType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "utran_GPSTimingOfCell : "
                    + getUtran_GPSTimingOfCell().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getModeSpecificInfo() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getModeSpecificInfo();
          }

          @Override public void setToNewInstance() {
            setModeSpecificInfoToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTRAN_GPSReferenceTime.modeSpecificInfoType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "modeSpecificInfo : "
                    + getModeSpecificInfo().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 2);

          @Override public boolean isExplicitlySet() {
            return getSfn() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getSfn();
          }

          @Override public void setToNewInstance() {
            setSfnToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? UTRAN_GPSReferenceTime.sfnType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "sfn : "
                    + getSfn().toIndentedString(indent);
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
public static class utran_GPSTimingOfCellType extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_utran_GPSTimingOfCellType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public utran_GPSTimingOfCellType() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_utran_GPSTimingOfCellType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_utran_GPSTimingOfCellType != null) {
      return ImmutableList.of(TAG_utran_GPSTimingOfCellType);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new utran_GPSTimingOfCellType from encoded stream.
   */
  public static utran_GPSTimingOfCellType fromPerUnaligned(byte[] encodedBytes) {
    utran_GPSTimingOfCellType result = new utran_GPSTimingOfCellType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new utran_GPSTimingOfCellType from encoded stream.
   */
  public static utran_GPSTimingOfCellType fromPerAligned(byte[] encodedBytes) {
    utran_GPSTimingOfCellType result = new utran_GPSTimingOfCellType();
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

  
  private utran_GPSTimingOfCellType.ms_partType ms_part_;
  public utran_GPSTimingOfCellType.ms_partType getMs_part() {
    return ms_part_;
  }
  /**
   * @throws ClassCastException if value is not a utran_GPSTimingOfCellType.ms_partType
   */
  public void setMs_part(Asn1Object value) {
    this.ms_part_ = (utran_GPSTimingOfCellType.ms_partType) value;
  }
  public utran_GPSTimingOfCellType.ms_partType setMs_partToNewInstance() {
    ms_part_ = new utran_GPSTimingOfCellType.ms_partType();
    return ms_part_;
  }
  
  private utran_GPSTimingOfCellType.ls_partType ls_part_;
  public utran_GPSTimingOfCellType.ls_partType getLs_part() {
    return ls_part_;
  }
  /**
   * @throws ClassCastException if value is not a utran_GPSTimingOfCellType.ls_partType
   */
  public void setLs_part(Asn1Object value) {
    this.ls_part_ = (utran_GPSTimingOfCellType.ls_partType) value;
  }
  public utran_GPSTimingOfCellType.ls_partType setLs_partToNewInstance() {
    ls_part_ = new utran_GPSTimingOfCellType.ls_partType();
    return ls_part_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getMs_part() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getMs_part();
          }

          @Override public void setToNewInstance() {
            setMs_partToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? utran_GPSTimingOfCellType.ms_partType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "ms_part : "
                    + getMs_part().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getLs_part() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getLs_part();
          }

          @Override public void setToNewInstance() {
            setLs_partToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? utran_GPSTimingOfCellType.ls_partType.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "ls_part : "
                    + getLs_part().toIndentedString(indent);
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
public static class ms_partType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_ms_partType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public ms_partType() {
    super();
    setValueRange("0", "1023");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_ms_partType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_ms_partType != null) {
      return ImmutableList.of(TAG_ms_partType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new ms_partType from encoded stream.
   */
  public static ms_partType fromPerUnaligned(byte[] encodedBytes) {
    ms_partType result = new ms_partType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new ms_partType from encoded stream.
   */
  public static ms_partType fromPerAligned(byte[] encodedBytes) {
    ms_partType result = new ms_partType();
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
    return "ms_partType = " + getInteger() + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class ls_partType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_ls_partType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public ls_partType() {
    super();
    setValueRange("0", "4294967295");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_ls_partType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_ls_partType != null) {
      return ImmutableList.of(TAG_ls_partType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new ls_partType from encoded stream.
   */
  public static ls_partType fromPerUnaligned(byte[] encodedBytes) {
    ls_partType result = new ls_partType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new ls_partType from encoded stream.
   */
  public static ls_partType fromPerAligned(byte[] encodedBytes) {
    ls_partType result = new ls_partType();
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
    return "ls_partType = " + getInteger() + ";\n";
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
    builder.append("utran_GPSTimingOfCellType = {\n");
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

  
/*
 */


//

/**
 */
public static class modeSpecificInfoType extends Asn1Choice {
  //

  private static final Asn1Tag TAG_modeSpecificInfoType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  private static final Map<Asn1Tag, Select> tagToSelection = new HashMap<>();

  private boolean extension;
  private ChoiceComponent selection;
  private Asn1Object element;

  static {
    for (Select select : Select.values()) {
      for (Asn1Tag tag : select.getPossibleFirstTags()) {
        Select select0;
        if ((select0 = tagToSelection.put(tag, select)) != null) {
          throw new IllegalStateException(
            "modeSpecificInfoType: " + tag + " maps to both " + select0 + " and " + select);
        }
      }
    }
  }

  public modeSpecificInfoType() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_modeSpecificInfoType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_modeSpecificInfoType != null) {
      return ImmutableList.of(TAG_modeSpecificInfoType);
    } else {
      return tagToSelection.keySet();
    }
  }

  /**
   * Creates a new modeSpecificInfoType from encoded stream.
   */
  public static modeSpecificInfoType fromPerUnaligned(byte[] encodedBytes) {
    modeSpecificInfoType result = new modeSpecificInfoType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new modeSpecificInfoType from encoded stream.
   */
  public static modeSpecificInfoType fromPerAligned(byte[] encodedBytes) {
    modeSpecificInfoType result = new modeSpecificInfoType();
    result.decodePerAligned(new BitStreamReader(encodedBytes));
    return result;
  }

  

  @Override protected boolean hasExtensionValue() {
    return extension;
  }

  @Override protected Integer getSelectionOrdinal() {
    return selection.ordinal();
  }

  @Nullable
  @Override
  protected ChoiceComponent getSelectedComponent() {
    return selection;
  }

  @Override protected int getOptionCount() {
    if (hasExtensionValue()) {
      return Extend.values().length;
    }
    return Select.values().length;
  }

  protected Asn1Object createAndSetValue(boolean isExtensionValue,
                                         int ordinal) {
    extension = isExtensionValue;
    if (isExtensionValue) {
      selection = Extend.values()[ordinal];
    } else {
      selection = Select.values()[ordinal];
    }
    element = selection.createElement();
    return element;
  }

  @Override protected ChoiceComponent createAndSetValue(Asn1Tag tag) {
    Select select = tagToSelection.get(tag);
    if (select == null) {
      throw new IllegalArgumentException("Unknown selection tag: " + tag);
    }
    element = select.createElement();
    selection = select;
    extension = false;
    return select;
  }

  @Override protected boolean isExtensible() {
    return false;
  }

  @Override protected Asn1Object getValue() {
    return element;
  }

  
  private static enum Select implements ChoiceComponent {
    
    $Fdd(Asn1Tag.fromClassAndNumber(2, 0),
        true) {
      @Override
      public Asn1Object createElement() {
        return new modeSpecificInfoType.fddType();
      }

      @Override
      Collection<Asn1Tag> getPossibleFirstTags() {
        return tag == null ? modeSpecificInfoType.fddType.getPossibleFirstTags() : ImmutableList.of(tag);
      }

      @Override
      String elementIndentedString(Asn1Object element, String indent) {
        return toString() + " : " + element.toIndentedString(indent);
      }
    },
    
    $Tdd(Asn1Tag.fromClassAndNumber(2, 1),
        true) {
      @Override
      public Asn1Object createElement() {
        return new modeSpecificInfoType.tddType();
      }

      @Override
      Collection<Asn1Tag> getPossibleFirstTags() {
        return tag == null ? modeSpecificInfoType.tddType.getPossibleFirstTags() : ImmutableList.of(tag);
      }

      @Override
      String elementIndentedString(Asn1Object element, String indent) {
        return toString() + " : " + element.toIndentedString(indent);
      }
    },
    
    ;

    @Nullable final Asn1Tag tag;
    final boolean isImplicitTagging;

    Select(@Nullable Asn1Tag tag, boolean isImplicitTagging) {
      this.tag = tag;
      this.isImplicitTagging = isImplicitTagging;
    }

    @Override
    public Asn1Object createElement() {
      throw new IllegalStateException("Select template error");
    }

    @Override
    @Nullable
    public Asn1Tag getTag() {
      return tag;
    }

    @Override
    public boolean isImplicitTagging() {
      return isImplicitTagging;
    }

    abstract Collection<Asn1Tag> getPossibleFirstTags();

    abstract String elementIndentedString(Asn1Object element, String indent);
  }
  
/*
 */


//

/**
*/
public static class fddType extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_fddType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public fddType() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_fddType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_fddType != null) {
      return ImmutableList.of(TAG_fddType);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new fddType from encoded stream.
   */
  public static fddType fromPerUnaligned(byte[] encodedBytes) {
    fddType result = new fddType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new fddType from encoded stream.
   */
  public static fddType fromPerAligned(byte[] encodedBytes) {
    fddType result = new fddType();
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

  
  private PrimaryCPICH_Info referenceIdentity_;
  public PrimaryCPICH_Info getReferenceIdentity() {
    return referenceIdentity_;
  }
  /**
   * @throws ClassCastException if value is not a PrimaryCPICH_Info
   */
  public void setReferenceIdentity(Asn1Object value) {
    this.referenceIdentity_ = (PrimaryCPICH_Info) value;
  }
  public PrimaryCPICH_Info setReferenceIdentityToNewInstance() {
    referenceIdentity_ = new PrimaryCPICH_Info();
    return referenceIdentity_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getReferenceIdentity() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getReferenceIdentity();
          }

          @Override public void setToNewInstance() {
            setReferenceIdentityToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? PrimaryCPICH_Info.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "referenceIdentity : "
                    + getReferenceIdentity().toIndentedString(indent);
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
    builder.append("fddType = {\n");
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


  public boolean isFdd() {
    return !hasExtensionValue() && Select.$Fdd == selection;
  }

  /**
   * @throws {@code IllegalStateException} if {@code !isFdd}.
   */
  @SuppressWarnings("unchecked")
  public modeSpecificInfoType.fddType getFdd() {
    if (!isFdd()) {
      throw new IllegalStateException("modeSpecificInfoType value not a Fdd");
    }
    return (modeSpecificInfoType.fddType) element;
  }

  public void setFdd(modeSpecificInfoType.fddType selected) {
    selection = Select.$Fdd;
    extension = false;
    element = selected;
  }

  public modeSpecificInfoType.fddType setFddToNewInstance() {
      modeSpecificInfoType.fddType element = new modeSpecificInfoType.fddType();
      setFdd(element);
      return element;
  }
  
/*
 */


//

/**
*/
public static class tddType extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_tddType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public tddType() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_tddType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_tddType != null) {
      return ImmutableList.of(TAG_tddType);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new tddType from encoded stream.
   */
  public static tddType fromPerUnaligned(byte[] encodedBytes) {
    tddType result = new tddType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new tddType from encoded stream.
   */
  public static tddType fromPerAligned(byte[] encodedBytes) {
    tddType result = new tddType();
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

  
  private CellParametersID referenceIdentity_;
  public CellParametersID getReferenceIdentity() {
    return referenceIdentity_;
  }
  /**
   * @throws ClassCastException if value is not a CellParametersID
   */
  public void setReferenceIdentity(Asn1Object value) {
    this.referenceIdentity_ = (CellParametersID) value;
  }
  public CellParametersID setReferenceIdentityToNewInstance() {
    referenceIdentity_ = new CellParametersID();
    return referenceIdentity_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getReferenceIdentity() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getReferenceIdentity();
          }

          @Override public void setToNewInstance() {
            setReferenceIdentityToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? CellParametersID.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "referenceIdentity : "
                    + getReferenceIdentity().toIndentedString(indent);
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
    builder.append("tddType = {\n");
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


  public boolean isTdd() {
    return !hasExtensionValue() && Select.$Tdd == selection;
  }

  /**
   * @throws {@code IllegalStateException} if {@code !isTdd}.
   */
  @SuppressWarnings("unchecked")
  public modeSpecificInfoType.tddType getTdd() {
    if (!isTdd()) {
      throw new IllegalStateException("modeSpecificInfoType value not a Tdd");
    }
    return (modeSpecificInfoType.tddType) element;
  }

  public void setTdd(modeSpecificInfoType.tddType selected) {
    selection = Select.$Tdd;
    extension = false;
    element = selected;
  }

  public modeSpecificInfoType.tddType setTddToNewInstance() {
      modeSpecificInfoType.tddType element = new modeSpecificInfoType.tddType();
      setTdd(element);
      return element;
  }
  

  private static enum Extend implements ChoiceComponent {
    
    ;
    @Nullable private final Asn1Tag tag;
    private final boolean isImplicitTagging;

    Extend(@Nullable Asn1Tag tag, boolean isImplicitTagging) {
      this.tag = tag;
      this.isImplicitTagging = isImplicitTagging;
    }

    public Asn1Object createElement() {
      throw new IllegalStateException("Extend template error");
    }

    @Override
    @Nullable
    public Asn1Tag getTag() {
      return tag;
    }

    @Override
    public boolean isImplicitTagging() {
      return isImplicitTagging;
    }

    String elementIndentedString(Asn1Object element, String indent) {
      throw new IllegalStateException("Extend template error");
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

  private String elementIndentedString(String indent) {
    if (element == null) {
      return "null;\n";
    }
    if (extension) {
      return Extend.values()[selection.ordinal()]
          .elementIndentedString(element, indent + "  ");
    } else {
      return Select.values()[selection.ordinal()]
          .elementIndentedString(element, indent + "  ");
    }
  }

  public String toIndentedString(String indent) {
    return "modeSpecificInfoType = " + elementIndentedString(indent) + indent + ";\n";
  }
}

  
/*
 */


//

/**
 */
public static class sfnType extends Asn1Integer {
  //

  private static final Asn1Tag TAG_sfnType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public sfnType() {
    super();
    setValueRange("0", "4095");

  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_sfnType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_sfnType != null) {
      return ImmutableList.of(TAG_sfnType);
    } else {
      return Asn1Integer.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new sfnType from encoded stream.
   */
  public static sfnType fromPerUnaligned(byte[] encodedBytes) {
    sfnType result = new sfnType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new sfnType from encoded stream.
   */
  public static sfnType fromPerAligned(byte[] encodedBytes) {
    sfnType result = new sfnType();
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
    return "sfnType = " + getInteger() + ";\n";
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
    builder.append("UTRAN_GPSReferenceTime = {\n");
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
