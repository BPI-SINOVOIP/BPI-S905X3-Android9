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
public  class ReferenceTime extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_ReferenceTime
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public ReferenceTime() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_ReferenceTime;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_ReferenceTime != null) {
      return ImmutableList.of(TAG_ReferenceTime);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new ReferenceTime from encoded stream.
   */
  public static ReferenceTime fromPerUnaligned(byte[] encodedBytes) {
    ReferenceTime result = new ReferenceTime();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new ReferenceTime from encoded stream.
   */
  public static ReferenceTime fromPerAligned(byte[] encodedBytes) {
    ReferenceTime result = new ReferenceTime();
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

  
  private GPSTime gpsTime_;
  public GPSTime getGpsTime() {
    return gpsTime_;
  }
  /**
   * @throws ClassCastException if value is not a GPSTime
   */
  public void setGpsTime(Asn1Object value) {
    this.gpsTime_ = (GPSTime) value;
  }
  public GPSTime setGpsTimeToNewInstance() {
    gpsTime_ = new GPSTime();
    return gpsTime_;
  }
  
  private GSMTime gsmTime_;
  public GSMTime getGsmTime() {
    return gsmTime_;
  }
  /**
   * @throws ClassCastException if value is not a GSMTime
   */
  public void setGsmTime(Asn1Object value) {
    this.gsmTime_ = (GSMTime) value;
  }
  public GSMTime setGsmTimeToNewInstance() {
    gsmTime_ = new GSMTime();
    return gsmTime_;
  }
  
  private GPSTOWAssist gpsTowAssist_;
  public GPSTOWAssist getGpsTowAssist() {
    return gpsTowAssist_;
  }
  /**
   * @throws ClassCastException if value is not a GPSTOWAssist
   */
  public void setGpsTowAssist(Asn1Object value) {
    this.gpsTowAssist_ = (GPSTOWAssist) value;
  }
  public GPSTOWAssist setGpsTowAssistToNewInstance() {
    gpsTowAssist_ = new GPSTOWAssist();
    return gpsTowAssist_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getGpsTime() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGpsTime();
          }

          @Override public void setToNewInstance() {
            setGpsTimeToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GPSTime.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gpsTime : "
                    + getGpsTime().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getGsmTime() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getGsmTime();
          }

          @Override public void setToNewInstance() {
            setGsmTimeToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GSMTime.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gsmTime : "
                    + getGsmTime().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 2);

          @Override public boolean isExplicitlySet() {
            return getGpsTowAssist() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return true;
          }

          @Override public Asn1Object getComponentValue() {
            return getGpsTowAssist();
          }

          @Override public void setToNewInstance() {
            setGpsTowAssistToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GPSTOWAssist.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gpsTowAssist : "
                    + getGpsTowAssist().toIndentedString(indent);
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
    builder.append("ReferenceTime = {\n");
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
