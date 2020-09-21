/**
 * Copyright (c) 2008, http://www.snakeyaml.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.yaml.snakeyaml.issues.issue311;

public class BeanWithEnum {

    private boolean boolField;
    private String name;
    private BooleanEnum enumField;

    public BeanWithEnum() {
        this(true, "", BooleanEnum.UNKNOWN);
    }

    public BeanWithEnum(boolean boolField, String name, BooleanEnum enumField) {
        this.boolField = boolField;
        this.name = name;
        this.enumField = enumField;
    }

    public boolean isBoolField() {
        return boolField;
    }

    public String getName() {
        return name;
    }

    public BooleanEnum getEnumField() {
        return enumField;
    }

    public void setBoolField(boolean boolField) {
        this.boolField = boolField;
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setEnumField(BooleanEnum enumField) {
        this.enumField = enumField;
    }
}
