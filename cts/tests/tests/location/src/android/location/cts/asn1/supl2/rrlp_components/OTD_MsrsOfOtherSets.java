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
public  class OTD_MsrsOfOtherSets extends Asn1Choice {
  //

  private static final Asn1Tag TAG_OTD_MsrsOfOtherSets
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
            "OTD_MsrsOfOtherSets: " + tag + " maps to both " + select0 + " and " + select);
        }
      }
    }
  }

  public OTD_MsrsOfOtherSets() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_OTD_MsrsOfOtherSets;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_OTD_MsrsOfOtherSets != null) {
      return ImmutableList.of(TAG_OTD_MsrsOfOtherSets);
    } else {
      return tagToSelection.keySet();
    }
  }

  /**
   * Creates a new OTD_MsrsOfOtherSets from encoded stream.
   */
  public static OTD_MsrsOfOtherSets fromPerUnaligned(byte[] encodedBytes) {
    OTD_MsrsOfOtherSets result = new OTD_MsrsOfOtherSets();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new OTD_MsrsOfOtherSets from encoded stream.
   */
  public static OTD_MsrsOfOtherSets fromPerAligned(byte[] encodedBytes) {
    OTD_MsrsOfOtherSets result = new OTD_MsrsOfOtherSets();
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
    
    $IdentityNotPresent(Asn1Tag.fromClassAndNumber(2, 0),
        true) {
      @Override
      public Asn1Object createElement() {
        return new OTD_Measurement();
      }

      @Override
      Collection<Asn1Tag> getPossibleFirstTags() {
        return tag == null ? OTD_Measurement.getPossibleFirstTags() : ImmutableList.of(tag);
      }

      @Override
      String elementIndentedString(Asn1Object element, String indent) {
        return toString() + " : " + element.toIndentedString(indent);
      }
    },
    
    $IdentityPresent(Asn1Tag.fromClassAndNumber(2, 1),
        true) {
      @Override
      public Asn1Object createElement() {
        return new OTD_MeasurementWithID();
      }

      @Override
      Collection<Asn1Tag> getPossibleFirstTags() {
        return tag == null ? OTD_MeasurementWithID.getPossibleFirstTags() : ImmutableList.of(tag);
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
  
  

  public boolean isIdentityNotPresent() {
    return !hasExtensionValue() && Select.$IdentityNotPresent == selection;
  }

  /**
   * @throws {@code IllegalStateException} if {@code !isIdentityNotPresent}.
   */
  @SuppressWarnings("unchecked")
  public OTD_Measurement getIdentityNotPresent() {
    if (!isIdentityNotPresent()) {
      throw new IllegalStateException("OTD_MsrsOfOtherSets value not a IdentityNotPresent");
    }
    return (OTD_Measurement) element;
  }

  public void setIdentityNotPresent(OTD_Measurement selected) {
    selection = Select.$IdentityNotPresent;
    extension = false;
    element = selected;
  }

  public OTD_Measurement setIdentityNotPresentToNewInstance() {
      OTD_Measurement element = new OTD_Measurement();
      setIdentityNotPresent(element);
      return element;
  }
  
  

  public boolean isIdentityPresent() {
    return !hasExtensionValue() && Select.$IdentityPresent == selection;
  }

  /**
   * @throws {@code IllegalStateException} if {@code !isIdentityPresent}.
   */
  @SuppressWarnings("unchecked")
  public OTD_MeasurementWithID getIdentityPresent() {
    if (!isIdentityPresent()) {
      throw new IllegalStateException("OTD_MsrsOfOtherSets value not a IdentityPresent");
    }
    return (OTD_MeasurementWithID) element;
  }

  public void setIdentityPresent(OTD_MeasurementWithID selected) {
    selection = Select.$IdentityPresent;
    extension = false;
    element = selected;
  }

  public OTD_MeasurementWithID setIdentityPresentToNewInstance() {
      OTD_MeasurementWithID element = new OTD_MeasurementWithID();
      setIdentityPresent(element);
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
    return "OTD_MsrsOfOtherSets = " + elementIndentedString(indent) + indent + ";\n";
  }
}
