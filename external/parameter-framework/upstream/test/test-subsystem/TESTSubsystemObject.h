/*
* Copyright (c) 2011-2015, Intel Corporation
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

#include "SubsystemObject.h"

class CMappingContext;

class CTESTSubsystemObject : public CSubsystemObject
{
public:
    CTESTSubsystemObject(const std::string &strMappingValue,
                         CInstanceConfigurableElement *pInstanceConfigurableElement,
                         const CMappingContext &context, core::log::Logger &logger);

protected:
    // from CSubsystemObject
    // Sync to/from HW
    virtual bool sendToHW(std::string &strError);
    virtual bool receiveFromHW(std::string &strError);

private:
    void sendToFile(std::ofstream &outputFile);
    void receiveFromFile(std::ifstream &inputFile);
    virtual std::string toString(const void *pvValue, size_t size) const = 0;
    virtual void fromString(const std::string &strValue, void *pvValue, size_t size) = 0;

protected:
    size_t _scalarSize;
    size_t _arraySize;
    std::string _strFilePath;
    bool _bLog;
    bool _bIsScalar;
};
