/****************************************************************************
*
*    Copyright (c) 2020 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#ifndef __HAL_LIMITATION_SUPPORT_HPP__
#define __HAL_LIMITATION_SUPPORT_HPP__
#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "nnrt/types.hpp"
namespace hal {
namespace limitation {

static constexpr bool REQUIRED = false;
static constexpr bool OPTIONAL = !REQUIRED;

class IArgList {
   public:
    // can not use map here: argument order is important to us
    using ArgsType = std::vector<std::pair<std::string, std::pair<nnrt::OperandType, bool /*optional*/>>>;

    /**
     * @brief try to match given arguments' data type with this argument definition
     *
     * @param args ordered arguments' data type
     * @return true matched with current argument list definition
     * @return false argmuent mis-match
     */
    bool match(const std::vector<nnrt::OperandType>& args) const;

    /**
     * @brief Extract give argmument position in current Argument list
     *
     * @param argName argument name, which defined by the api requirement file
     * @return -1 if argument not found
     * @return [0, argc)
     */
    int32_t ArgPos(const std::string& argName) const;

    void printf() const;

    ArgsType m_Arguments;
};

class MatchedArgument {
   public:
    MatchedArgument(uint32_t providedArgc, const IArgList* matchedArglist)
        : m_ProvidedArgc(providedArgc), m_Arglist(matchedArglist) {}

    int32_t ArgPos(const std::string& argName) const {
        int32_t posInMatchedArgList = m_Arglist->ArgPos(argName);
        if ((uint32_t)posInMatchedArgList >= m_ProvidedArgc) return -1;
        return posInMatchedArgList;
    }

   private:
    uint32_t m_ProvidedArgc;  //!< This is provided by client
    const IArgList* m_Arglist;
};

using MatchedArgumentPtr = std::shared_ptr<MatchedArgument>;
class OpSpecCollection {
   public:
    const IArgList* match(const std::string& opName, const std::vector<nnrt::OperandType>& args) const;

    std::map<std::string, std::vector<const IArgList*>> m_Collection;
};
}
}
#endif
