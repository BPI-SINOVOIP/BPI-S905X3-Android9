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
#ifndef __OVX_MEMORY_POOL_H__
#define __OVX_MEMORY_POOL_H__

#include <cstddef>
#include <cstdint>
#include <vector>
#include <list>
#include <map>
#include <tuple>
#include <memory>

enum {
    OVXLIB_MAX_SIZE_OF_IMMEDIATELY_COPIED_VALUES = 128
};

namespace nnrt{
    //forward declare
    class Memory;
    class Model;
}

namespace mem_pool{
//forward declare
namespace alloc{
    struct Mmp_block_infor;
    struct Nml_block_infor;
    struct Sml_block_infor;
}

/**
 * @brief Memory type indentifier
 *
 */
enum class Type : int32_t{
    MMP = 0,    //!< memory source is memory mapped file
    NML = 1,    //!< memory source is memory pointer with proper size
    SML = 2,    //!< memory source is memory pointer with relative samll size then @NML

    CNT,
    INVALID = CNT,
};

/**
 * @brief reference object as unique descriptor for memory block
 * client SHOULD not create instance of Ref but keep a shared_ref
 * object to memory pool managed by memory-pool
 *
 * @code{.cpp}
 * shared_ref reference = mem_pool::global_memory_pool()::add_reference(...);
 * if (reference) {
 *  // Do somthing with your shared_ptr
 * }
 * @endcode
 *
 */
struct Ref {
    // upper-part: user
    const void* address_;     //!< real memory address for read
    size_t len_;              //!< memory block size in bytes

    // lower-part: internal purpose
    Type mem_type_;     //!< memory type @see Type
    uint32_t index_;    //!< used when release reference
    uint32_t offset_;   //!< used when release reference
    const void* src_;   //!< memory address or mmap obj
    //initialize all paramters
    Ref():address_(nullptr), len_(0), mem_type_(Type::INVALID), index_(0),\
          offset_(0), src_(nullptr){};
};

using shared_ref = std::shared_ptr<Ref>;
using weak_ref = std::weak_ptr<Ref>;

typedef nnrt::Memory MmapSegment; // TODO, remove this once renamed nnrt::Memory

/**
 * @brief Data struct managed memory mapped information
 * Memory mapped file may referenced by different operand with different offset and
 * length, if more than two reference created on same memory mapped file, we need
 * maintain a count for it.
 */
struct Mmp_block_list{
    typedef std::map<const MmapSegment*, unsigned int> mem_list_type;
    mem_list_type mem_list_;   //!< second is a shared count

    /**
     * @brief Add reference to given memory mapped block
     *
     * @param infor Which memory mapped space referenced, and offset + length in bytes
     * @param mem_ref if offset + length < upper bound of memory mapped space, set
     *        mem_ref.len_ as requested in infor, or mem_ref.len_ setup by
     *        offset + length - sizeof(memory mapped space)
     * @return true offset not exceed upper bound of memory mapped space
     * @return false offset exceed upper bound of memory mapped space
     */
    bool add_reference(const alloc::Mmp_block_infor& infor, Ref& mem_ref);

    /**
     * @brief delete given memory reference
     *
     * @param mem_ref content of memory reference
     */
    void del_reference(const Ref& mem_ref);
};

/**
 * @brief Data struct manage normal block information
 *
 *  interface description @see Mmp_block_list
 */
struct Nml_block_list{
     /**
     * @brief Read Only Segment descriptor
     */
    struct Ro_segment{
        const void* address_;   //!< source memory address referenced by client
        size_t len_;            //!< size in bytes
    };

    typedef std::map<Ro_segment, int> mem_list_type;
    mem_list_type mem_list_;

    bool add_reference(const alloc::Nml_block_infor& infor, Ref& mem_ref);
    void del_reference(const Ref& mem_ref);
};
/**
 * @brief Data struct manage small blocks, it will allocate/free memory space internally
 *
 */
struct Sml_block_list{
    static const size_t small_block_shreshold = OVXLIB_MAX_SIZE_OF_IMMEDIATELY_COPIED_VALUES;

    std::vector<std::vector<uint8_t>> mem_list_;
    std::map<Ref, int> ref_count_; //!<

    Sml_block_list();

    bool add_reference(const alloc::Sml_block_infor& infor, Ref& mem_ref);
    void del_reference(const Ref& mem_ref);
};

/**
 * @brief manage different type of memory references
 * Provide easy-to-used interface to add/delete memory reference
 *
 */
struct Manager{
    explicit Manager(nnrt::Model&);

    shared_ref add_reference(const void* address, size_t len, bool forceNotAcllocateMem = false);

    shared_ref add_reference(const nnrt::Memory* address, size_t offset, size_t len);

    void del_reference(const Ref& mem_ref) ;

private:
    /**
     * @brief Template for add memory reference
     * return shared_ptr of Ref object. Use it as raw-pointer,
     * client should check it not a null_ptr before access it.
     *
     * @tparam BlockAllocInfor memory "allocate" type
     * @param infor
     * @return shared_ref   if add reference failed, return null_ptr
     */
    template <typename BlockAllocInfor>
    shared_ref add_reference(const BlockAllocInfor& infor) ;

    std::tuple<Mmp_block_list, Nml_block_list, Sml_block_list> space_;

    nnrt::Model& host_model_;
};

}
#endif
