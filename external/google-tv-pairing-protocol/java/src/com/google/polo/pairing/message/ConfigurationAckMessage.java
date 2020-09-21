/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
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

package com.google.polo.pairing.message;

/**
 * Object implementing the internal representation of the protocol message
 * 'CONFIGURATION_ACK'.
 */
public class ConfigurationAckMessage extends PoloMessage {
  
  public ConfigurationAckMessage() {
    super(PoloMessage.PoloMessageType.CONFIGURATION_ACK);
  }
  
  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (!(obj instanceof ConfigurationAckMessage)) {
      return false;
    }

    return true;
  }
  
}
