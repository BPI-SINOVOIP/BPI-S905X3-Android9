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
import android.location.cts.asn1.base.Asn1Choice;
import android.location.cts.asn1.base.Asn1Null;
import android.location.cts.asn1.base.Asn1Object;
import android.location.cts.asn1.base.Asn1Tag;
import android.location.cts.asn1.base.BitStream;
import android.location.cts.asn1.base.BitStreamReader;
import android.location.cts.asn1.base.ChoiceComponent;
import com.google.common.collect.ImmutableList;
import java.nio.ByteBuffer;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import javax.annotation.Nullable;


/**
 */
public  class SatStatus extends Asn1Choice {
  //

  private static final Asn1Tag TAG_SatStatus
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
            "SatStatus: " + tag + " maps to both " + select0 + " and " + select);
        }
      }
    }
  }

  public SatStatus() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_SatStatus;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_SatStatus != null) {
      return ImmutableList.of(TAG_SatStatus);
    } else {
      return tagToSelection.keySet();
    }
  }

  /**
   * Creates a new SatStatus from encoded stream.
   */
  public static SatStatus fromPerUnaligned(byte[] encodedBytes) {
    SatStatus result = new SatStatus();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new SatStatus from encoded stream.
   */
  public static SatStatus fromPerAligned(byte[] encodedBytes) {
    SatStatus result = new SatStatus();
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
    return true;
  }

  @Override protected Asn1Object getValue() {
    return element;
  }

  
  private static enum Select implements ChoiceComponent {
    
    $NewSatelliteAndModelUC(Asn1Tag.fromClassAndNumber(2, 0),
        true) {
      @Override
      public Asn1Object createElement() {
        return new UncompressedEphemeris();
      }

      @Override
      Collection<Asn1Tag> getPossibleFirstTags() {
        return tag == null ? UncompressedEphemeris.getPossibleFirstTags() : ImmutableList.of(tag);
      }

      @Override
      String elementIndentedString(Asn1Object element, String indent) {
        return toString() + " : " + element.toIndentedString(indent);
      }
    },
    
    $OldSatelliteAndModel(Asn1Tag.fromClassAndNumber(2, 1),
        true) {
      @Override
      public Asn1Object createElement() {
        return new SatStatus.oldSatelliteAndModelType();
      }

      @Override
      Collection<Asn1Tag> getPossibleFirstTags() {
        return tag == null ? SatStatus.oldSatelliteAndModelType.getPossibleFirstTags() : ImmutableList.of(tag);
      }

      @Override
      String elementIndentedString(Asn1Object element, String indent) {
        return toString() + " : " + element.toIndentedString(indent);
      }
    },
    
    $NewNaviModelUC(Asn1Tag.fromClassAndNumber(2, 2),
        true) {
      @Override
      public Asn1Object createElement() {
        return new UncompressedEphemeris();
      }

      @Override
      Collection<Asn1Tag> getPossibleFirstTags() {
        return tag == null ? UncompressedEphemeris.getPossibleFirstTags() : ImmutableList.of(tag);
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
  
  

  public boolean isNewSatelliteAndModelUC() {
    return !hasExtensionValue() && Select.$NewSatelliteAndModelUC == selection;
  }

  /**
   * @throws {@code IllegalStateException} if {@code !isNewSatelliteAndModelUC}.
   */
  @SuppressWarnings("unchecked")
  public UncompressedEphemeris getNewSatelliteAndModelUC() {
    if (!isNewSatelliteAndModelUC()) {
      throw new IllegalStateException("SatStatus value not a NewSatelliteAndModelUC");
    }
    return (UncompressedEphemeris) element;
  }

  public void setNewSatelliteAndModelUC(UncompressedEphemeris selected) {
    selection = Select.$NewSatelliteAndModelUC;
    extension = false;
    element = selected;
  }

  public UncompressedEphemeris setNewSatelliteAndModelUCToNewInstance() {
      UncompressedEphemeris element = new UncompressedEphemeris();
      setNewSatelliteAndModelUC(element);
      return element;
  }
  
/*
 */


//

/**
 */
public static class oldSatelliteAndModelType extends Asn1Null {
  //

  private static final Asn1Tag TAG_oldSatelliteAndModelType
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public oldSatelliteAndModelType() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_oldSatelliteAndModelType;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_oldSatelliteAndModelType != null) {
      return ImmutableList.of(TAG_oldSatelliteAndModelType);
    } else {
      return Asn1Null.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new oldSatelliteAndModelType from encoded stream.
   */
  public static oldSatelliteAndModelType fromPerUnaligned(byte[] encodedBytes) {
    oldSatelliteAndModelType result = new oldSatelliteAndModelType();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new oldSatelliteAndModelType from encoded stream.
   */
  public static oldSatelliteAndModelType fromPerAligned(byte[] encodedBytes) {
    oldSatelliteAndModelType result = new oldSatelliteAndModelType();
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
    return "oldSatelliteAndModelType (null value);\n";
  }
}


  public boolean isOldSatelliteAndModel() {
    return !hasExtensionValue() && Select.$OldSatelliteAndModel == selection;
  }

  /**
   * @throws {@code IllegalStateException} if {@code !isOldSatelliteAndModel}.
   */
  @SuppressWarnings("unchecked")
  public SatStatus.oldSatelliteAndModelType getOldSatelliteAndModel() {
    if (!isOldSatelliteAndModel()) {
      throw new IllegalStateException("SatStatus value not a OldSatelliteAndModel");
    }
    return (SatStatus.oldSatelliteAndModelType) element;
  }

  public void setOldSatelliteAndModel(SatStatus.oldSatelliteAndModelType selected) {
    selection = Select.$OldSatelliteAndModel;
    extension = false;
    element = selected;
  }

  public SatStatus.oldSatelliteAndModelType setOldSatelliteAndModelToNewInstance() {
      SatStatus.oldSatelliteAndModelType element = new SatStatus.oldSatelliteAndModelType();
      setOldSatelliteAndModel(element);
      return element;
  }
  
  

  public boolean isNewNaviModelUC() {
    return !hasExtensionValue() && Select.$NewNaviModelUC == selection;
  }

  /**
   * @throws {@code IllegalStateException} if {@code !isNewNaviModelUC}.
   */
  @SuppressWarnings("unchecked")
  public UncompressedEphemeris getNewNaviModelUC() {
    if (!isNewNaviModelUC()) {
      throw new IllegalStateException("SatStatus value not a NewNaviModelUC");
    }
    return (UncompressedEphemeris) element;
  }

  public void setNewNaviModelUC(UncompressedEphemeris selected) {
    selection = Select.$NewNaviModelUC;
    extension = false;
    element = selected;
  }

  public UncompressedEphemeris setNewNaviModelUCToNewInstance() {
      UncompressedEphemeris element = new UncompressedEphemeris();
      setNewNaviModelUC(element);
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
    return "SatStatus = " + elementIndentedString(indent) + indent + ";\n";
  }
}
