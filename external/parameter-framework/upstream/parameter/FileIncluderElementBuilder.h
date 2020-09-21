/*
 * Copyright (c) 2014, Intel Corporation
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

#include "ElementBuilder.h"
#include "XmlFileIncluderElement.h"

/** This is the XmlFileIncluderElement builder class
 *
 * It is builds the XmlFileIncluderElement with
 * a mandatory option specific to xml:
 *  whether it should be validated with schemas or not
 */
class CFileIncluderElementBuilder : public CElementBuilder
{
public:
    CFileIncluderElementBuilder(bool bValidateWithSchemas, const std::string &schemaBaseUri)
        : CElementBuilder(), _bValidateWithSchemas(bValidateWithSchemas),
          _schemaBaseUri(schemaBaseUri)
    {
    }

    virtual CElement *createElement(const CXmlElement &xmlElement) const
    {
        return new CXmlFileIncluderElement(xmlElement.getNameAttribute(), xmlElement.getType(),
                                           _bValidateWithSchemas, _schemaBaseUri);
    }

private:
    bool _bValidateWithSchemas;
    const std::string _schemaBaseUri;
};
