/*
 * Copyright (c) 2011-2014, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "TypeElement.h"

#include <string>

class CComponentType;

class CComponentInstance : public CTypeElement
{
public:
    CComponentInstance(const std::string &strName);

    // Mapping info
    virtual bool getMappingData(const std::string &strKey, const std::string *&pStrValue) const;
    virtual bool hasMappingData() const;
    /**
     * Returns the mapping associated to the current TypeElement instance
     *
     * @return A std::string containing the mapping as a comma separated key value pairs
     */
    virtual std::string getFormattedMapping() const;

    // From IXmlSink
    virtual bool fromXml(const CXmlElement &xmlElement, CXmlSerializingContext &serializingContext);

    // CElement
    virtual std::string getKind() const;
    std::string getXmlElementName() const override;

private:
    virtual bool childrenAreDynamic() const;
    virtual CInstanceConfigurableElement *doInstantiate() const;
    virtual void populate(CElement *pElement) const;

    // Related component type
    const CComponentType *_pComponentType{nullptr};
};
