/*
 * Copyright (c) 2011-2016, Intel Corporation
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
#include "ParameterAccessContext.h"

#define base utility::ErrorContext

CParameterAccessContext::CParameterAccessContext(std::string &strError,
                                                 CParameterBlackboard *pParameterBlackboard,
                                                 bool bValueSpaceIsRaw, bool bOutputRawFormatIsHex,
                                                 size_t baseOffset)
    : base(strError), _pParameterBlackboard(pParameterBlackboard),
      _bValueSpaceIsRaw(bValueSpaceIsRaw), _bOutputRawFormatIsHex(bOutputRawFormatIsHex),
      _uiBaseOffset(baseOffset)
{
}

CParameterAccessContext::CParameterAccessContext(std::string &strError,
                                                 CParameterBlackboard *pParameterBlackboard,
                                                 size_t baseOffset)
    : base(strError), _pParameterBlackboard(pParameterBlackboard), _uiBaseOffset(baseOffset)
{
}

CParameterAccessContext::CParameterAccessContext(std::string &strError) : base(strError)
{
}

// ParameterBlackboard
CParameterBlackboard *CParameterAccessContext::getParameterBlackboard()
{
    return _pParameterBlackboard;
}

void CParameterAccessContext::setParameterBlackboard(CParameterBlackboard *pBlackboard)
{
    _pParameterBlackboard = pBlackboard;
    _uiBaseOffset = 0;
}

// Base offset for blackboard access
void CParameterAccessContext::setBaseOffset(size_t baseOffset)
{
    _uiBaseOffset = baseOffset;
}

size_t CParameterAccessContext::getBaseOffset() const
{
    return _uiBaseOffset;
}

// Value Space
void CParameterAccessContext::setValueSpaceRaw(bool bIsRaw)
{
    _bValueSpaceIsRaw = bIsRaw;
}

bool CParameterAccessContext::valueSpaceIsRaw() const
{
    return _bValueSpaceIsRaw;
}

// Output Raw Format for user get value interpretation
void CParameterAccessContext::setOutputRawFormat(bool bIsHex)
{
    _bOutputRawFormatIsHex = bIsHex;
}

bool CParameterAccessContext::outputRawFormatIsHex() const
{
    return _bOutputRawFormatIsHex;
}

// Automatic synchronization to HW
void CParameterAccessContext::setAutoSync(bool bAutoSync)
{
    _bAutoSync = bAutoSync;
}

bool CParameterAccessContext::getAutoSync() const
{
    return _bAutoSync;
}
