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
#ifndef __SUPPORT_MACROS_HPP__
#define __SUPPORT_MACROS_HPP__
#include "support.hpp"
//////////////////////////////SPEC DEFINE MACROs/////////////////////////////////
#include "boost/preprocessor.hpp"
#include <algorithm>

// OP_SPEC_NAME is a client defined MACRO, provide unique operation name in current SPEC
// OP_SPEC_REGISTER is a client defined MACRO, which can customize register entry point
//                  then, we can define another api requirment spec for other framework

#ifndef OP_SPEC_REGISTER
#error "OP_SPEC_REGISTER should be defined before include this file"
#endif

#define OP_SPEC_BEGIN()                                     \
    namespace hal {                                         \
    namespace limitation {                                 \
    class BOOST_PP_CAT(OP_SPEC_NAME, _) : public IArgList { \
       public:

#define OP_SPEC_ARG(arg_name)                                                                          \
    BOOST_PP_CAT(OP_SPEC_NAME, _) &                                                                    \
        BOOST_PP_CAT(arg_name, _)(nnrt::OperandType dtype, bool optional = REQUIRED) {                 \
        auto found_arg_at =                                                                            \
            std::find_if(m_Arguments.begin(), m_Arguments.end(), [](const ArgsType::value_type& arg) { \
                return arg.first == std::string(BOOST_PP_STRINGIZE(arg_name));                         \
            });                                                                                        \
        if (m_Arguments.end() == found_arg_at) {                                                       \
            m_Arguments.push_back(                                                                     \
                std::make_pair(                                                                        \
                  std::string(BOOST_PP_STRINGIZE(arg_name)), std::make_pair(dtype, optional)));        \
        } else {                                                                                       \
            found_arg_at->second = std::make_pair(dtype, optional);                                    \
        }                                                                                              \
        return *this;                                                                                  \
    }                                                                                                  \
    int32_t BOOST_PP_CAT(arg_name, _pos)() const {                                                     \
        auto found_arg_at =                                                                            \
            std::find_if(m_Arguments.begin(), m_Arguments.end(), [](const ArgsType::value_type& arg) { \
                return arg.first == std::string(BOOST_PP_STRINGIZE(arg_name));                         \
            });                                                                                        \
        if (m_Arguments.end() == found_arg_at) {                                                       \
            return -1;                                                                                 \
        } else                                                                                         \
            return std::distance(found_arg_at, m_Arguments.begin());                                   \
    }

#define OP_SPEC_END()                                                                              \
    }                                                                                              \
    ; /*End of Concrete Operation Argument Class*/                                                 \
    class BOOST_PP_CAT(OP_SPEC_NAME, _register_helper) {                                           \
       public:                                                                                     \
        BOOST_PP_CAT(OP_SPEC_NAME, _register_helper)(const std::string& name, IArgList& argList) { \
            BOOST_PP_CAT(OP_SPEC_REGISTER, Register)(name, &argList);                              \
        }                                                                                          \
    };                                                                                             \
    } /*end of namespace limitation*/                                                             \
    } /*end of namespace hal*/                                                                     \
    using namespace hal::limitation;

#define MAKE_SPEC(uid)                                                    \
    static BOOST_PP_CAT(OP_SPEC_NAME, _) BOOST_PP_CAT(OP_SPEC_NAME, uid); \
    static BOOST_PP_CAT(OP_SPEC_NAME, _register_helper)                   \
        BOOST_PP_CAT(BOOST_PP_CAT(OP_SPEC_NAME, _register_helper), uid)(  \
            std::string(BOOST_PP_STRINGIZE(OP_SPEC_NAME)), BOOST_PP_CAT(OP_SPEC_NAME, uid)

#define OVERRIDE_SPEC(parentId, uid)                                                               \
    static BOOST_PP_CAT(OP_SPEC_NAME, _) BOOST_PP_CAT(OP_SPEC_NAME, BOOST_PP_CAT(parentId, uid)) = \
        BOOST_PP_CAT(OP_SPEC_NAME, parentId);                                                      \
    static BOOST_PP_CAT(OP_SPEC_NAME, _register_helper)                                            \
        BOOST_PP_CAT(BOOST_PP_CAT(OP_SPEC_NAME, _register_helper), BOOST_PP_CAT(parentId, uid))(   \
            std::string(BOOST_PP_STRINGIZE(OP_SPEC_NAME)), BOOST_PP_CAT(OP_SPEC_NAME, BOOST_PP_CAT(parentId, uid))

#endif