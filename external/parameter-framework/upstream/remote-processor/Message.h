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

#include "NonCopyable.hpp"
#include <vector>
#include <string>
#include <cstdint>

#include <remote_processor_export.h>

class Socket;

class REMOTE_PROCESSOR_EXPORT CMessage : private utility::NonCopyable
{
public:
    enum class MsgType : std::uint8_t
    {
        ECommandRequest,
        ESuccessAnswer,
        EFailureAnswer,
        EInvalid = static_cast<uint8_t>(-1),
    };
    CMessage(MsgType ucMsgId);
    CMessage();
    virtual ~CMessage() = default;

    enum Result
    {
        success,
        peerDisconnected,
        error
    };

    /** Write or read the message on pSocket.
     *
     * @param[in,out] socket is the socket on wich IO operation will be made.
     * @param[in] bOut if true message should be read,
     *                 if false it should be written.
     * @param[out] strError on failure, a string explaining the error,
     *                      on success, undefined.
     *
     * @return success if a correct message could be recv/send
     *         peerDisconnected if the peer disconnected before the first socket access.
     *         error if the message could not be read/write for any other reason
     */
    Result serialize(Socket &&socket, bool bOut, std::string &strError);

protected:
    // Msg Id
    MsgType getMsgId() const;

    /** Write raw data to the message
    *
    * @param[in] pvData pointer to the data array
    * @param[in] uiSize array size in bytes
    */
    void writeData(const void *pvData, size_t uiSize);

    /** Read raw data from the message
    *
    * @param[out] pvData pointer to the data array
    * @param[in] uiSize array size in bytes
    */
    void readData(void *pvData, size_t uiSize);

    /** Write string to the message
    *
    * @param[in] strData the string to write
    */
    void writeString(const std::string &strData);

    /** Write string to the message
    *
    * @param[out] strData the string to read to
    */
    void readString(std::string &strData);

    /** @return string length plus room to store its length
    *
    * @param[in] strData the string to get the size from
    */
    size_t getStringSize(const std::string &strData) const;

    /** @return remaining data size to read or to write depending on the context
    * (request: write, answer: read)
    */
    size_t getRemainingDataSize() const;

private:
    bool isValidAccess(size_t offset, size_t size) const;

    /** Allocate room to store the message
    *
    * @param[int] uiDataSize the szie to allocate in bytes
    */
    void allocateData(size_t uiDataSize);
    // Fill data to send
    virtual void fillDataToSend() = 0;
    // Collect received data
    virtual void collectReceivedData() = 0;

    /** @return size of the transaction data in bytes
    */
    virtual size_t getDataSize() const = 0;

    // Checksum
    uint8_t computeChecksum() const;

    // MsgId
    MsgType _ucMsgId;

    size_t getMessageDataSize() const { return mData.size(); }

    using Data = std::vector<uint8_t>;
    Data mData;
    /** Read/Write Index used to iterate across the message data */
    size_t _uiIndex;

    static const uint16_t SYNC_WORD = 0xBABE;
};
