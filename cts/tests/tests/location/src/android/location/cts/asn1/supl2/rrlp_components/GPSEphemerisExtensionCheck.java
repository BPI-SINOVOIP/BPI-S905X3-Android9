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
public  class GPSEphemerisExtensionCheck extends Asn1Sequence {
  //

  private static final Asn1Tag TAG_GPSEphemerisExtensionCheck
      = Asn1Tag.fromClassAndNumber(-1, -1);

  public GPSEphemerisExtensionCheck() {
    super();
  }

  @Override
  @Nullable
  protected Asn1Tag getTag() {
    return TAG_GPSEphemerisExtensionCheck;
  }

  @Override
  protected boolean isTagImplicit() {
    return true;
  }

  public static Collection<Asn1Tag> getPossibleFirstTags() {
    if (TAG_GPSEphemerisExtensionCheck != null) {
      return ImmutableList.of(TAG_GPSEphemerisExtensionCheck);
    } else {
      return Asn1Sequence.getPossibleFirstTags();
    }
  }

  /**
   * Creates a new GPSEphemerisExtensionCheck from encoded stream.
   */
  public static GPSEphemerisExtensionCheck fromPerUnaligned(byte[] encodedBytes) {
    GPSEphemerisExtensionCheck result = new GPSEphemerisExtensionCheck();
    result.decodePerUnaligned(new BitStreamReader(encodedBytes));
    return result;
  }

  /**
   * Creates a new GPSEphemerisExtensionCheck from encoded stream.
   */
  public static GPSEphemerisExtensionCheck fromPerAligned(byte[] encodedBytes) {
    GPSEphemerisExtensionCheck result = new GPSEphemerisExtensionCheck();
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

  
  private GPSEphemerisExtensionTime gpsBeginTime_;
  public GPSEphemerisExtensionTime getGpsBeginTime() {
    return gpsBeginTime_;
  }
  /**
   * @throws ClassCastException if value is not a GPSEphemerisExtensionTime
   */
  public void setGpsBeginTime(Asn1Object value) {
    this.gpsBeginTime_ = (GPSEphemerisExtensionTime) value;
  }
  public GPSEphemerisExtensionTime setGpsBeginTimeToNewInstance() {
    gpsBeginTime_ = new GPSEphemerisExtensionTime();
    return gpsBeginTime_;
  }
  
  private GPSEphemerisExtensionTime gpsEndTime_;
  public GPSEphemerisExtensionTime getGpsEndTime() {
    return gpsEndTime_;
  }
  /**
   * @throws ClassCastException if value is not a GPSEphemerisExtensionTime
   */
  public void setGpsEndTime(Asn1Object value) {
    this.gpsEndTime_ = (GPSEphemerisExtensionTime) value;
  }
  public GPSEphemerisExtensionTime setGpsEndTimeToNewInstance() {
    gpsEndTime_ = new GPSEphemerisExtensionTime();
    return gpsEndTime_;
  }
  
  private GPSSatEventsInfo gpsSatEventsInfo_;
  public GPSSatEventsInfo getGpsSatEventsInfo() {
    return gpsSatEventsInfo_;
  }
  /**
   * @throws ClassCastException if value is not a GPSSatEventsInfo
   */
  public void setGpsSatEventsInfo(Asn1Object value) {
    this.gpsSatEventsInfo_ = (GPSSatEventsInfo) value;
  }
  public GPSSatEventsInfo setGpsSatEventsInfoToNewInstance() {
    gpsSatEventsInfo_ = new GPSSatEventsInfo();
    return gpsSatEventsInfo_;
  }
  

  

  

  @Override public Iterable<? extends SequenceComponent> getComponents() {
    ImmutableList.Builder<SequenceComponent> builder = ImmutableList.builder();
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 0);

          @Override public boolean isExplicitlySet() {
            return getGpsBeginTime() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGpsBeginTime();
          }

          @Override public void setToNewInstance() {
            setGpsBeginTimeToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GPSEphemerisExtensionTime.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gpsBeginTime : "
                    + getGpsBeginTime().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 1);

          @Override public boolean isExplicitlySet() {
            return getGpsEndTime() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGpsEndTime();
          }

          @Override public void setToNewInstance() {
            setGpsEndTimeToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GPSEphemerisExtensionTime.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gpsEndTime : "
                    + getGpsEndTime().toIndentedString(indent);
              }
        });
    
    builder.add(new SequenceComponent() {
          Asn1Tag tag = Asn1Tag.fromClassAndNumber(2, 2);

          @Override public boolean isExplicitlySet() {
            return getGpsSatEventsInfo() != null;
          }

          @Override public boolean hasDefaultValue() {
            return false;
          }

          @Override public boolean isOptional() {
            return false;
          }

          @Override public Asn1Object getComponentValue() {
            return getGpsSatEventsInfo();
          }

          @Override public void setToNewInstance() {
            setGpsSatEventsInfoToNewInstance();
          }

          @Override public Collection<Asn1Tag> getPossibleFirstTags() {
            return tag == null ? GPSSatEventsInfo.getPossibleFirstTags() : ImmutableList.of(tag);
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
                return "gpsSatEventsInfo : "
                    + getGpsSatEventsInfo().toIndentedString(indent);
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
    builder.append("GPSEphemerisExtensionCheck = {\n");
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
