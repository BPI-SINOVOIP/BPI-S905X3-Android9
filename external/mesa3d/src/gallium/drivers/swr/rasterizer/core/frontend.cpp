/****************************************************************************
* Copyright (C) 2014-2015 Intel Corporation.   All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*
* @file frontend.cpp
*
* @brief Implementation for Frontend which handles vertex processing,
*        primitive assembly, clipping, binning, etc.
*
******************************************************************************/

#include "api.h"
#include "frontend.h"
#include "backend.h"
#include "context.h"
#include "rdtsc_core.h"
#include "utils.h"
#include "threads.h"
#include "pa.h"
#include "clip.h"
#include "tilemgr.h"
#include "tessellator.h"
#include <limits>

//////////////////////////////////////////////////////////////////////////
/// @brief Helper macro to generate a bitmask
static INLINE uint32_t GenMask(uint32_t numBits)
{
    SWR_ASSERT(numBits <= (sizeof(uint32_t) * 8), "Too many bits (%d) for %s", numBits, __FUNCTION__);
    return ((1U << numBits) - 1);
}

//////////////////////////////////////////////////////////////////////////
/// @brief FE handler for SwrSync.
/// @param pContext - pointer to SWR context.
/// @param pDC - pointer to draw context.
/// @param workerId - thread's worker id. Even thread has a unique id.
/// @param pUserData - Pointer to user data passed back to sync callback.
/// @todo This should go away when we switch this to use compute threading.
void ProcessSync(
    SWR_CONTEXT *pContext,
    DRAW_CONTEXT *pDC,
    uint32_t workerId,
    void *pUserData)
{
    BE_WORK work;
    work.type = SYNC;
    work.pfnWork = ProcessSyncBE;

    MacroTileMgr *pTileMgr = pDC->pTileMgr;
    pTileMgr->enqueue(0, 0, &work);
}

//////////////////////////////////////////////////////////////////////////
/// @brief FE handler for SwrDestroyContext.
/// @param pContext - pointer to SWR context.
/// @param pDC - pointer to draw context.
/// @param workerId - thread's worker id. Even thread has a unique id.
/// @param pUserData - Pointer to user data passed back to sync callback.
void ProcessShutdown(
    SWR_CONTEXT *pContext,
    DRAW_CONTEXT *pDC,
    uint32_t workerId,
    void *pUserData)
{
    BE_WORK work;
    work.type = SHUTDOWN;
    work.pfnWork = ProcessShutdownBE;

    MacroTileMgr *pTileMgr = pDC->pTileMgr;
    // Enqueue at least 1 work item for each worker thread
    // account for number of numa nodes
    uint32_t numNumaNodes = pContext->threadPool.numaMask + 1;

    for (uint32_t i = 0; i < pContext->threadPool.numThreads; ++i)
    {
        for (uint32_t n = 0; n < numNumaNodes; ++n)
        {
            pTileMgr->enqueue(i, n, &work);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/// @brief FE handler for SwrClearRenderTarget.
/// @param pContext - pointer to SWR context.
/// @param pDC - pointer to draw context.
/// @param workerId - thread's worker id. Even thread has a unique id.
/// @param pUserData - Pointer to user data passed back to clear callback.
/// @todo This should go away when we switch this to use compute threading.
void ProcessClear(
    SWR_CONTEXT *pContext,
    DRAW_CONTEXT *pDC,
    uint32_t workerId,
    void *pUserData)
{
    CLEAR_DESC *pDesc = (CLEAR_DESC*)pUserData;
    MacroTileMgr *pTileMgr = pDC->pTileMgr;

    // queue a clear to each macro tile
    // compute macro tile bounds for the specified rect
    uint32_t macroTileXMin = pDesc->rect.xmin / KNOB_MACROTILE_X_DIM;
    uint32_t macroTileXMax = (pDesc->rect.xmax - 1) / KNOB_MACROTILE_X_DIM;
    uint32_t macroTileYMin = pDesc->rect.ymin / KNOB_MACROTILE_Y_DIM;
    uint32_t macroTileYMax = (pDesc->rect.ymax - 1) / KNOB_MACROTILE_Y_DIM;

    BE_WORK work;
    work.type = CLEAR;
    work.pfnWork = ProcessClearBE;
    work.desc.clear = *pDesc;

    for (uint32_t y = macroTileYMin; y <= macroTileYMax; ++y)
    {
        for (uint32_t x = macroTileXMin; x <= macroTileXMax; ++x)
        {
            pTileMgr->enqueue(x, y, &work);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/// @brief FE handler for SwrStoreTiles.
/// @param pContext - pointer to SWR context.
/// @param pDC - pointer to draw context.
/// @param workerId - thread's worker id. Even thread has a unique id.
/// @param pUserData - Pointer to user data passed back to callback.
/// @todo This should go away when we switch this to use compute threading.
void ProcessStoreTiles(
    SWR_CONTEXT *pContext,
    DRAW_CONTEXT *pDC,
    uint32_t workerId,
    void *pUserData)
{
    AR_BEGIN(FEProcessStoreTiles, pDC->drawId);
    MacroTileMgr *pTileMgr = pDC->pTileMgr;
    STORE_TILES_DESC* pDesc = (STORE_TILES_DESC*)pUserData;

    // queue a store to each macro tile
    // compute macro tile bounds for the specified rect
    uint32_t macroTileXMin = pDesc->rect.xmin / KNOB_MACROTILE_X_DIM;
    uint32_t macroTileXMax = (pDesc->rect.xmax - 1) / KNOB_MACROTILE_X_DIM;
    uint32_t macroTileYMin = pDesc->rect.ymin / KNOB_MACROTILE_Y_DIM;
    uint32_t macroTileYMax = (pDesc->rect.ymax - 1) / KNOB_MACROTILE_Y_DIM;

    // store tiles
    BE_WORK work;
    work.type = STORETILES;
    work.pfnWork = ProcessStoreTilesBE;
    work.desc.storeTiles = *pDesc;

    for (uint32_t y = macroTileYMin; y <= macroTileYMax; ++y)
    {
        for (uint32_t x = macroTileXMin; x <= macroTileXMax; ++x)
        {
            pTileMgr->enqueue(x, y, &work);
        }
    }

    AR_END(FEProcessStoreTiles, 0);
}

//////////////////////////////////////////////////////////////////////////
/// @brief FE handler for SwrInvalidateTiles.
/// @param pContext - pointer to SWR context.
/// @param pDC - pointer to draw context.
/// @param workerId - thread's worker id. Even thread has a unique id.
/// @param pUserData - Pointer to user data passed back to callback.
/// @todo This should go away when we switch this to use compute threading.
void ProcessDiscardInvalidateTiles(
    SWR_CONTEXT *pContext,
    DRAW_CONTEXT *pDC,
    uint32_t workerId,
    void *pUserData)
{
    AR_BEGIN(FEProcessInvalidateTiles, pDC->drawId);
    DISCARD_INVALIDATE_TILES_DESC *pDesc = (DISCARD_INVALIDATE_TILES_DESC*)pUserData;
    MacroTileMgr *pTileMgr = pDC->pTileMgr;

    // compute macro tile bounds for the specified rect
    uint32_t macroTileXMin = (pDesc->rect.xmin + KNOB_MACROTILE_X_DIM - 1) / KNOB_MACROTILE_X_DIM;
    uint32_t macroTileXMax = (pDesc->rect.xmax / KNOB_MACROTILE_X_DIM) - 1;
    uint32_t macroTileYMin = (pDesc->rect.ymin + KNOB_MACROTILE_Y_DIM - 1) / KNOB_MACROTILE_Y_DIM;
    uint32_t macroTileYMax = (pDesc->rect.ymax / KNOB_MACROTILE_Y_DIM) - 1;

    if (pDesc->fullTilesOnly == false)
    {
        // include partial tiles
        macroTileXMin = pDesc->rect.xmin / KNOB_MACROTILE_X_DIM;
        macroTileXMax = (pDesc->rect.xmax - 1) / KNOB_MACROTILE_X_DIM;
        macroTileYMin = pDesc->rect.ymin / KNOB_MACROTILE_Y_DIM;
        macroTileYMax = (pDesc->rect.ymax - 1) / KNOB_MACROTILE_Y_DIM;
    }

    SWR_ASSERT(macroTileXMax <= KNOB_NUM_HOT_TILES_X);
    SWR_ASSERT(macroTileYMax <= KNOB_NUM_HOT_TILES_Y);

    macroTileXMax = std::min<int32_t>(macroTileXMax, KNOB_NUM_HOT_TILES_X);
    macroTileYMax = std::min<int32_t>(macroTileYMax, KNOB_NUM_HOT_TILES_Y);

    // load tiles
    BE_WORK work;
    work.type = DISCARDINVALIDATETILES;
    work.pfnWork = ProcessDiscardInvalidateTilesBE;
    work.desc.discardInvalidateTiles = *pDesc;

    for (uint32_t x = macroTileXMin; x <= macroTileXMax; ++x)
    {
        for (uint32_t y = macroTileYMin; y <= macroTileYMax; ++y)
        {
            pTileMgr->enqueue(x, y, &work);
        }
    }

    AR_END(FEProcessInvalidateTiles, 0);
}

//////////////////////////////////////////////////////////////////////////
/// @brief Computes the number of primitives given the number of verts.
/// @param mode - primitive topology for draw operation.
/// @param numPrims - number of vertices or indices for draw.
/// @todo Frontend needs to be refactored. This will go in appropriate place then.
uint32_t GetNumPrims(
    PRIMITIVE_TOPOLOGY mode,
    uint32_t numPrims)
{
    switch (mode)
    {
    case TOP_POINT_LIST: return numPrims;
    case TOP_TRIANGLE_LIST: return numPrims / 3;
    case TOP_TRIANGLE_STRIP: return numPrims < 3 ? 0 : numPrims - 2;
    case TOP_TRIANGLE_FAN: return numPrims < 3 ? 0 : numPrims - 2;
    case TOP_TRIANGLE_DISC: return numPrims < 2 ? 0 : numPrims - 1;
    case TOP_QUAD_LIST: return numPrims / 4;
    case TOP_QUAD_STRIP: return numPrims < 4 ? 0 : (numPrims - 2) / 2;
    case TOP_LINE_STRIP: return numPrims < 2 ? 0 : numPrims - 1;
    case TOP_LINE_LIST: return numPrims / 2;
    case TOP_LINE_LOOP: return numPrims;
    case TOP_RECT_LIST: return numPrims / 3;
    case TOP_LINE_LIST_ADJ: return numPrims / 4;
    case TOP_LISTSTRIP_ADJ: return numPrims < 3 ? 0 : numPrims - 3;
    case TOP_TRI_LIST_ADJ: return numPrims / 6;
    case TOP_TRI_STRIP_ADJ: return numPrims < 4 ? 0 : (numPrims / 2) - 2;

    case TOP_PATCHLIST_1:
    case TOP_PATCHLIST_2:
    case TOP_PATCHLIST_3:
    case TOP_PATCHLIST_4:
    case TOP_PATCHLIST_5:
    case TOP_PATCHLIST_6:
    case TOP_PATCHLIST_7:
    case TOP_PATCHLIST_8:
    case TOP_PATCHLIST_9:
    case TOP_PATCHLIST_10:
    case TOP_PATCHLIST_11:
    case TOP_PATCHLIST_12:
    case TOP_PATCHLIST_13:
    case TOP_PATCHLIST_14:
    case TOP_PATCHLIST_15:
    case TOP_PATCHLIST_16:
    case TOP_PATCHLIST_17:
    case TOP_PATCHLIST_18:
    case TOP_PATCHLIST_19:
    case TOP_PATCHLIST_20:
    case TOP_PATCHLIST_21:
    case TOP_PATCHLIST_22:
    case TOP_PATCHLIST_23:
    case TOP_PATCHLIST_24:
    case TOP_PATCHLIST_25:
    case TOP_PATCHLIST_26:
    case TOP_PATCHLIST_27:
    case TOP_PATCHLIST_28:
    case TOP_PATCHLIST_29:
    case TOP_PATCHLIST_30:
    case TOP_PATCHLIST_31:
    case TOP_PATCHLIST_32:
        return numPrims / (mode - TOP_PATCHLIST_BASE);

    case TOP_POLYGON:
    case TOP_POINT_LIST_BF:
    case TOP_LINE_STRIP_CONT:
    case TOP_LINE_STRIP_BF:
    case TOP_LINE_STRIP_CONT_BF:
    case TOP_TRIANGLE_FAN_NOSTIPPLE:
    case TOP_TRI_STRIP_REVERSE:
    case TOP_PATCHLIST_BASE:
    case TOP_UNKNOWN:
        SWR_ASSERT(false, "Unsupported topology: %d", mode);
        return 0;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Computes the number of verts given the number of primitives.
/// @param mode - primitive topology for draw operation.
/// @param numPrims - number of primitives for draw.
uint32_t GetNumVerts(
    PRIMITIVE_TOPOLOGY mode,
    uint32_t numPrims)
{
    switch (mode)
    {
    case TOP_POINT_LIST: return numPrims;
    case TOP_TRIANGLE_LIST: return numPrims * 3;
    case TOP_TRIANGLE_STRIP: return numPrims ? numPrims + 2 : 0;
    case TOP_TRIANGLE_FAN: return numPrims ? numPrims + 2 : 0;
    case TOP_TRIANGLE_DISC: return numPrims ? numPrims + 1 : 0;
    case TOP_QUAD_LIST: return numPrims * 4;
    case TOP_QUAD_STRIP: return numPrims ? numPrims * 2 + 2 : 0;
    case TOP_LINE_STRIP: return numPrims ? numPrims + 1 : 0;
    case TOP_LINE_LIST: return numPrims * 2;
    case TOP_LINE_LOOP: return numPrims;
    case TOP_RECT_LIST: return numPrims * 3;
    case TOP_LINE_LIST_ADJ: return numPrims * 4;
    case TOP_LISTSTRIP_ADJ: return numPrims ? numPrims + 3 : 0;
    case TOP_TRI_LIST_ADJ: return numPrims * 6;
    case TOP_TRI_STRIP_ADJ: return numPrims ? (numPrims + 2) * 2 : 0;

    case TOP_PATCHLIST_1:
    case TOP_PATCHLIST_2:
    case TOP_PATCHLIST_3:
    case TOP_PATCHLIST_4:
    case TOP_PATCHLIST_5:
    case TOP_PATCHLIST_6:
    case TOP_PATCHLIST_7:
    case TOP_PATCHLIST_8:
    case TOP_PATCHLIST_9:
    case TOP_PATCHLIST_10:
    case TOP_PATCHLIST_11:
    case TOP_PATCHLIST_12:
    case TOP_PATCHLIST_13:
    case TOP_PATCHLIST_14:
    case TOP_PATCHLIST_15:
    case TOP_PATCHLIST_16:
    case TOP_PATCHLIST_17:
    case TOP_PATCHLIST_18:
    case TOP_PATCHLIST_19:
    case TOP_PATCHLIST_20:
    case TOP_PATCHLIST_21:
    case TOP_PATCHLIST_22:
    case TOP_PATCHLIST_23:
    case TOP_PATCHLIST_24:
    case TOP_PATCHLIST_25:
    case TOP_PATCHLIST_26:
    case TOP_PATCHLIST_27:
    case TOP_PATCHLIST_28:
    case TOP_PATCHLIST_29:
    case TOP_PATCHLIST_30:
    case TOP_PATCHLIST_31:
    case TOP_PATCHLIST_32:
        return numPrims * (mode - TOP_PATCHLIST_BASE);

    case TOP_POLYGON:
    case TOP_POINT_LIST_BF:
    case TOP_LINE_STRIP_CONT:
    case TOP_LINE_STRIP_BF:
    case TOP_LINE_STRIP_CONT_BF:
    case TOP_TRIANGLE_FAN_NOSTIPPLE:
    case TOP_TRI_STRIP_REVERSE:
    case TOP_PATCHLIST_BASE:
    case TOP_UNKNOWN:
        SWR_ASSERT(false, "Unsupported topology: %d", mode);
        return 0;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Return number of verts per primitive.
/// @param topology - topology
/// @param includeAdjVerts - include adjacent verts in primitive vertices
INLINE uint32_t NumVertsPerPrim(PRIMITIVE_TOPOLOGY topology, bool includeAdjVerts)
{
    uint32_t numVerts = 0;
    switch (topology)
    {
    case TOP_POINT_LIST:
    case TOP_POINT_LIST_BF:
        numVerts = 1;
        break;
    case TOP_LINE_LIST:
    case TOP_LINE_STRIP:
    case TOP_LINE_LIST_ADJ:
    case TOP_LINE_LOOP:
    case TOP_LINE_STRIP_CONT:
    case TOP_LINE_STRIP_BF:
    case TOP_LISTSTRIP_ADJ:
        numVerts = 2;
        break;
    case TOP_TRIANGLE_LIST:
    case TOP_TRIANGLE_STRIP:
    case TOP_TRIANGLE_FAN:
    case TOP_TRI_LIST_ADJ:
    case TOP_TRI_STRIP_ADJ:
    case TOP_TRI_STRIP_REVERSE:
    case TOP_RECT_LIST:
        numVerts = 3;
        break;
    case TOP_QUAD_LIST:
    case TOP_QUAD_STRIP:
        numVerts = 4;
        break;
    case TOP_PATCHLIST_1:
    case TOP_PATCHLIST_2:
    case TOP_PATCHLIST_3:
    case TOP_PATCHLIST_4:
    case TOP_PATCHLIST_5:
    case TOP_PATCHLIST_6:
    case TOP_PATCHLIST_7:
    case TOP_PATCHLIST_8:
    case TOP_PATCHLIST_9:
    case TOP_PATCHLIST_10:
    case TOP_PATCHLIST_11:
    case TOP_PATCHLIST_12:
    case TOP_PATCHLIST_13:
    case TOP_PATCHLIST_14:
    case TOP_PATCHLIST_15:
    case TOP_PATCHLIST_16:
    case TOP_PATCHLIST_17:
    case TOP_PATCHLIST_18:
    case TOP_PATCHLIST_19:
    case TOP_PATCHLIST_20:
    case TOP_PATCHLIST_21:
    case TOP_PATCHLIST_22:
    case TOP_PATCHLIST_23:
    case TOP_PATCHLIST_24:
    case TOP_PATCHLIST_25:
    case TOP_PATCHLIST_26:
    case TOP_PATCHLIST_27:
    case TOP_PATCHLIST_28:
    case TOP_PATCHLIST_29:
    case TOP_PATCHLIST_30:
    case TOP_PATCHLIST_31:
    case TOP_PATCHLIST_32:
        numVerts = topology - TOP_PATCHLIST_BASE;
        break;
    default:
        SWR_ASSERT(false, "Unsupported topology: %d", topology);
        break;
    }

    if (includeAdjVerts)
    {
        switch (topology)
        {
        case TOP_LISTSTRIP_ADJ:
        case TOP_LINE_LIST_ADJ: numVerts = 4; break;
        case TOP_TRI_STRIP_ADJ:
        case TOP_TRI_LIST_ADJ: numVerts = 6; break;
        default: break;
        }
    }

    return numVerts;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Generate mask from remaining work.
/// @param numWorkItems - Number of items being worked on by a SIMD.
static INLINE simdscalari GenerateMask(uint32_t numItemsRemaining)
{
    uint32_t numActive = (numItemsRemaining >= KNOB_SIMD_WIDTH) ? KNOB_SIMD_WIDTH : numItemsRemaining;
    uint32_t mask = (numActive > 0) ? ((1 << numActive) - 1) : 0;
    return _simd_castps_si(vMask(mask));
}

//////////////////////////////////////////////////////////////////////////
/// @brief StreamOut - Streams vertex data out to SO buffers.
///        Generally, we are only streaming out a SIMDs worth of triangles.
/// @param pDC - pointer to draw context.
/// @param workerId - thread's worker id. Even thread has a unique id.
/// @param numPrims - Number of prims to streamout (e.g. points, lines, tris)
static void StreamOut(
    DRAW_CONTEXT* pDC,
    PA_STATE& pa,
    uint32_t workerId,
    uint32_t* pPrimData,
    uint32_t streamIndex)
{
    SWR_CONTEXT *pContext = pDC->pContext;

    AR_BEGIN(FEStreamout, pDC->drawId);

    const API_STATE& state = GetApiState(pDC);
    const SWR_STREAMOUT_STATE &soState = state.soState;

    uint32_t soVertsPerPrim = NumVertsPerPrim(pa.binTopology, false);

    // The pPrimData buffer is sparse in that we allocate memory for all 32 attributes for each vertex.
    uint32_t primDataDwordVertexStride = (KNOB_NUM_ATTRIBUTES * sizeof(float) * 4) / sizeof(uint32_t);

    SWR_STREAMOUT_CONTEXT soContext = { 0 };

    // Setup buffer state pointers.
    for (uint32_t i = 0; i < 4; ++i)
    {
        soContext.pBuffer[i] = &state.soBuffer[i];
    }

    uint32_t numPrims = pa.NumPrims();
    for (uint32_t primIndex = 0; primIndex < numPrims; ++primIndex)
    {
        DWORD slot = 0;
        uint32_t soMask = soState.streamMasks[streamIndex];

        // Write all entries into primitive data buffer for SOS.
        while (_BitScanForward(&slot, soMask))
        {
            __m128 attrib[MAX_NUM_VERTS_PER_PRIM];    // prim attribs (always 4 wide)
            uint32_t paSlot = slot + VERTEX_ATTRIB_START_SLOT;
            pa.AssembleSingle(paSlot, primIndex, attrib);

            // Attribute offset is relative offset from start of vertex.
            // Note that attributes start at slot 1 in the PA buffer. We need to write this
            // to prim data starting at slot 0. Which is why we do (slot - 1).
            // Also note: GL works slightly differently, and needs slot 0
            uint32_t primDataAttribOffset = slot * sizeof(float) * 4 / sizeof(uint32_t);

            // Store each vertex's attrib at appropriate locations in pPrimData buffer.
            for (uint32_t v = 0; v < soVertsPerPrim; ++v)
            {
                uint32_t* pPrimDataAttrib = pPrimData + primDataAttribOffset + (v * primDataDwordVertexStride);

                _mm_store_ps((float*)pPrimDataAttrib, attrib[v]);
            }
            soMask &= ~(1 << slot);
        }

        // Update pPrimData pointer 
        soContext.pPrimData = pPrimData;

        // Call SOS
        SWR_ASSERT(state.pfnSoFunc[streamIndex] != nullptr, "Trying to execute uninitialized streamout jit function.");
        state.pfnSoFunc[streamIndex](soContext);
    }

    // Update SO write offset. The driver provides memory for the update.
    for (uint32_t i = 0; i < 4; ++i)
    {
        if (state.soBuffer[i].pWriteOffset)
        {
            *state.soBuffer[i].pWriteOffset = soContext.pBuffer[i]->streamOffset * sizeof(uint32_t);
        }

        if (state.soBuffer[i].soWriteEnable)
        {
            pDC->dynState.SoWriteOffset[i] = soContext.pBuffer[i]->streamOffset * sizeof(uint32_t);
            pDC->dynState.SoWriteOffsetDirty[i] = true;
        }
    }

    UPDATE_STAT_FE(SoPrimStorageNeeded[streamIndex], soContext.numPrimStorageNeeded);
    UPDATE_STAT_FE(SoNumPrimsWritten[streamIndex], soContext.numPrimsWritten);

    AR_END(FEStreamout, 1);
}

//////////////////////////////////////////////////////////////////////////
/// @brief Computes number of invocations. The current index represents
///        the start of the SIMD. The max index represents how much work
///        items are remaining. If there is less then a SIMD's xmin of work
///        then return the remaining amount of work.
/// @param curIndex - The start index for the SIMD.
/// @param maxIndex - The last index for all work items.
static INLINE uint32_t GetNumInvocations(
    uint32_t curIndex,
    uint32_t maxIndex)
{
    uint32_t remainder = (maxIndex - curIndex);
    return (remainder >= KNOB_SIMD_WIDTH) ? KNOB_SIMD_WIDTH : remainder;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Converts a streamId buffer to a cut buffer for the given stream id.
///        The geometry shader will loop over each active streamout buffer, assembling
///        primitives for the downstream stages. When multistream output is enabled,
///        the generated stream ID buffer from the GS needs to be converted to a cut
///        buffer for the primitive assembler.
/// @param stream - stream id to generate the cut buffer for
/// @param pStreamIdBase - pointer to the stream ID buffer
/// @param numEmittedVerts - Number of total verts emitted by the GS
/// @param pCutBuffer - output buffer to write cuts to
void ProcessStreamIdBuffer(uint32_t stream, uint8_t* pStreamIdBase, uint32_t numEmittedVerts, uint8_t *pCutBuffer)
{
    SWR_ASSERT(stream < MAX_SO_STREAMS);

    uint32_t numInputBytes = (numEmittedVerts * 2  + 7) / 8;
    uint32_t numOutputBytes = std::max(numInputBytes / 2, 1U);

    for (uint32_t b = 0; b < numOutputBytes; ++b)
    {
        uint8_t curInputByte = pStreamIdBase[2*b];
        uint8_t outByte = 0;
        for (uint32_t i = 0; i < 4; ++i)
        {
            if ((curInputByte & 0x3) != stream)
            {
                outByte |= (1 << i);
            }
            curInputByte >>= 2;
        }

        curInputByte = pStreamIdBase[2 * b + 1];
        for (uint32_t i = 0; i < 4; ++i)
        {
            if ((curInputByte & 0x3) != stream)
            {
                outByte |= (1 << (i + 4));
            }
            curInputByte >>= 2;
        }
        
        *pCutBuffer++ = outByte;
    }
}

THREAD SWR_GS_CONTEXT tlsGsContext;

//////////////////////////////////////////////////////////////////////////
/// @brief Implements GS stage.
/// @param pDC - pointer to draw context.
/// @param workerId - thread's worker id. Even thread has a unique id.
/// @param pa - The primitive assembly object.
/// @param pGsOut - output stream for GS
template <
    typename HasStreamOutT,
    typename HasRastT>
static void GeometryShaderStage(
    DRAW_CONTEXT *pDC,
    uint32_t workerId,
    PA_STATE& pa,
    void* pGsOut,
    void* pCutBuffer,
    void* pStreamCutBuffer,
    uint32_t* pSoPrimData,
    simdscalari primID)
{
    SWR_CONTEXT *pContext = pDC->pContext;

    AR_BEGIN(FEGeometryShader, pDC->drawId);

    const API_STATE& state = GetApiState(pDC);
    const SWR_GS_STATE* pState = &state.gsState;

    SWR_ASSERT(pGsOut != nullptr, "GS output buffer should be initialized");
    SWR_ASSERT(pCutBuffer != nullptr, "GS output cut buffer should be initialized");

    tlsGsContext.pStream = (uint8_t*)pGsOut;
    tlsGsContext.pCutOrStreamIdBuffer = (uint8_t*)pCutBuffer;
    tlsGsContext.PrimitiveID = primID;

    uint32_t numVertsPerPrim = NumVertsPerPrim(pa.binTopology, true);
    simdvector attrib[MAX_ATTRIBUTES];

    // assemble all attributes for the input primitive
    for (uint32_t slot = 0; slot < pState->numInputAttribs; ++slot)
    {
        uint32_t attribSlot = VERTEX_ATTRIB_START_SLOT + slot;
        pa.Assemble(attribSlot, attrib);

        for (uint32_t i = 0; i < numVertsPerPrim; ++i)
        {
            tlsGsContext.vert[i].attrib[attribSlot] = attrib[i];
        }
    }
    
    // assemble position
    pa.Assemble(VERTEX_POSITION_SLOT, attrib);
    for (uint32_t i = 0; i < numVertsPerPrim; ++i)
    {
        tlsGsContext.vert[i].attrib[VERTEX_POSITION_SLOT] = attrib[i];
    }

    const uint32_t vertexStride = sizeof(simdvertex);
    const uint32_t numSimdBatches = (state.gsState.maxNumVerts + KNOB_SIMD_WIDTH - 1) / KNOB_SIMD_WIDTH;
    const uint32_t inputPrimStride = numSimdBatches * vertexStride;
    const uint32_t instanceStride = inputPrimStride * KNOB_SIMD_WIDTH;
    uint32_t cutPrimStride;
    uint32_t cutInstanceStride;

    if (pState->isSingleStream)
    {
        cutPrimStride = (state.gsState.maxNumVerts + 7) / 8;
        cutInstanceStride = cutPrimStride * KNOB_SIMD_WIDTH;
    }
    else
    {
        cutPrimStride = AlignUp(state.gsState.maxNumVerts * 2 / 8, 4);
        cutInstanceStride = cutPrimStride * KNOB_SIMD_WIDTH;
    }

    // record valid prims from the frontend to avoid over binning the newly generated
    // prims from the GS
    uint32_t numInputPrims = pa.NumPrims();

    for (uint32_t instance = 0; instance < pState->instanceCount; ++instance)
    {
        tlsGsContext.InstanceID = instance;
        tlsGsContext.mask = GenerateMask(numInputPrims);

        // execute the geometry shader
        state.pfnGsFunc(GetPrivateState(pDC), &tlsGsContext);

        tlsGsContext.pStream += instanceStride;
        tlsGsContext.pCutOrStreamIdBuffer += cutInstanceStride;
    }

    // set up new binner and state for the GS output topology
    PFN_PROCESS_PRIMS pfnClipFunc = nullptr;
    if (HasRastT::value)
    {
        switch (pState->outputTopology)
        {
        case TOP_TRIANGLE_STRIP:    pfnClipFunc = ClipTriangles; break;
        case TOP_LINE_STRIP:        pfnClipFunc = ClipLines; break;
        case TOP_POINT_LIST:        pfnClipFunc = ClipPoints; break;
        default: SWR_ASSERT(false, "Unexpected GS output topology: %d", pState->outputTopology);
        }
    }

    // foreach input prim:
    // - setup a new PA based on the emitted verts for that prim
    // - loop over the new verts, calling PA to assemble each prim
    uint32_t* pVertexCount = (uint32_t*)&tlsGsContext.vertexCount;
    uint32_t* pPrimitiveId = (uint32_t*)&primID;

    uint32_t totalPrimsGenerated = 0;
    for (uint32_t inputPrim = 0; inputPrim < numInputPrims; ++inputPrim)
    {
        uint8_t* pInstanceBase = (uint8_t*)pGsOut + inputPrim * inputPrimStride;
        uint8_t* pCutBufferBase = (uint8_t*)pCutBuffer + inputPrim * cutPrimStride;
        for (uint32_t instance = 0; instance < pState->instanceCount; ++instance)
        {
            uint32_t numEmittedVerts = pVertexCount[inputPrim];
            if (numEmittedVerts == 0)
            {
                continue;
            }

            uint8_t* pBase = pInstanceBase + instance * instanceStride;
            uint8_t* pCutBase = pCutBufferBase + instance * cutInstanceStride;
            
            uint32_t numAttribs = state.feNumAttributes;

            for (uint32_t stream = 0; stream < MAX_SO_STREAMS; ++stream)
            {
                bool processCutVerts = false;

                uint8_t* pCutBuffer = pCutBase;

                // assign default stream ID, only relevant when GS is outputting a single stream
                uint32_t streamID = 0;
                if (pState->isSingleStream)
                {
                    processCutVerts = true;
                    streamID = pState->singleStreamID;
                    if (streamID != stream) continue;
                }
                else
                {
                    // early exit if this stream is not enabled for streamout
                    if (HasStreamOutT::value && !state.soState.streamEnable[stream])
                    {
                        continue;
                    }

                    // multi-stream output, need to translate StreamID buffer to a cut buffer
                    ProcessStreamIdBuffer(stream, pCutBase, numEmittedVerts, (uint8_t*)pStreamCutBuffer);
                    pCutBuffer = (uint8_t*)pStreamCutBuffer;
                    processCutVerts = false;
                }

                PA_STATE_CUT gsPa(pDC, pBase, numEmittedVerts, pCutBuffer, numEmittedVerts, numAttribs, pState->outputTopology, processCutVerts);

                while (gsPa.GetNextStreamOutput())
                {
                    do
                    {
                        bool assemble = gsPa.Assemble(VERTEX_POSITION_SLOT, attrib);

                        if (assemble)
                        {
                            totalPrimsGenerated += gsPa.NumPrims();

                            if (HasStreamOutT::value)
                            {
                                StreamOut(pDC, gsPa, workerId, pSoPrimData, stream);
                            }

                            if (HasRastT::value && state.soState.streamToRasterizer == stream)
                            {
                                simdscalari vPrimId;
                                // pull primitiveID from the GS output if available
                                if (state.gsState.emitsPrimitiveID)
                                {
                                    simdvector primIdAttrib[3];
                                    gsPa.Assemble(VERTEX_PRIMID_SLOT, primIdAttrib);
                                    vPrimId = _simd_castps_si(primIdAttrib[0].x);
                                }
                                else
                                {
                                    vPrimId = _simd_set1_epi32(pPrimitiveId[inputPrim]);
                                }

                                // use viewport array index if GS declares it as an output attribute. Otherwise use index 0.
                                simdscalari vViewPortIdx;
                                if (state.gsState.emitsViewportArrayIndex)
                                {
                                    simdvector vpiAttrib[3];
                                    gsPa.Assemble(VERTEX_VIEWPORT_ARRAY_INDEX_SLOT, vpiAttrib);

                                    // OOB indices => forced to zero.
                                    simdscalari vNumViewports = _simd_set1_epi32(KNOB_NUM_VIEWPORTS_SCISSORS);
                                    simdscalari vClearMask = _simd_cmplt_epi32(_simd_castps_si(vpiAttrib[0].x), vNumViewports);
                                    vpiAttrib[0].x = _simd_and_ps(_simd_castsi_ps(vClearMask), vpiAttrib[0].x);

                                    vViewPortIdx = _simd_castps_si(vpiAttrib[0].x);
                                }
                                else
                                {
                                    vViewPortIdx = _simd_set1_epi32(0);
                                }

                                pfnClipFunc(pDC, gsPa, workerId, attrib, GenMask(gsPa.NumPrims()), vPrimId, vViewPortIdx);
                            }
                        }
                    } while (gsPa.NextPrim());
                }
            }
        }
    }

    // update GS pipeline stats
    UPDATE_STAT_FE(GsInvocations, numInputPrims * pState->instanceCount);
    UPDATE_STAT_FE(GsPrimitives, totalPrimsGenerated);
	AR_EVENT(GSPrimInfo(numInputPrims, totalPrimsGenerated, numVertsPerPrim*numInputPrims));
    AR_END(FEGeometryShader, 1);
}

//////////////////////////////////////////////////////////////////////////
/// @brief Allocate GS buffers
/// @param pDC - pointer to draw context.
/// @param state - API state
/// @param ppGsOut - pointer to GS output buffer allocation
/// @param ppCutBuffer - pointer to GS output cut buffer allocation
static INLINE void AllocateGsBuffers(DRAW_CONTEXT* pDC, const API_STATE& state, void** ppGsOut, void** ppCutBuffer,
    void **ppStreamCutBuffer)
{
    auto pArena = pDC->pArena;
    SWR_ASSERT(pArena != nullptr);
    SWR_ASSERT(state.gsState.gsEnable);
    // allocate arena space to hold GS output verts
    // @todo pack attribs
    // @todo support multiple streams
    const uint32_t vertexStride = sizeof(simdvertex);
    const uint32_t numSimdBatches = (state.gsState.maxNumVerts + KNOB_SIMD_WIDTH - 1) / KNOB_SIMD_WIDTH;
    uint32_t size = state.gsState.instanceCount * numSimdBatches * vertexStride * KNOB_SIMD_WIDTH;
    *ppGsOut = pArena->AllocAligned(size, KNOB_SIMD_WIDTH * sizeof(float));

    const uint32_t cutPrimStride = (state.gsState.maxNumVerts + 7) / 8;
    const uint32_t streamIdPrimStride = AlignUp(state.gsState.maxNumVerts * 2 / 8, 4);
    const uint32_t cutBufferSize = cutPrimStride * state.gsState.instanceCount * KNOB_SIMD_WIDTH;
    const uint32_t streamIdSize = streamIdPrimStride * state.gsState.instanceCount * KNOB_SIMD_WIDTH;

    // allocate arena space to hold cut or streamid buffer, which is essentially a bitfield sized to the
    // maximum vertex output as defined by the GS state, per SIMD lane, per GS instance

    // allocate space for temporary per-stream cut buffer if multi-stream is enabled
    if (state.gsState.isSingleStream)
    {
        *ppCutBuffer = pArena->AllocAligned(cutBufferSize, KNOB_SIMD_WIDTH * sizeof(float));
        *ppStreamCutBuffer = nullptr;
    }
    else
    {
        *ppCutBuffer = pArena->AllocAligned(streamIdSize, KNOB_SIMD_WIDTH * sizeof(float));
        *ppStreamCutBuffer = pArena->AllocAligned(cutBufferSize, KNOB_SIMD_WIDTH * sizeof(float));
    }

}

//////////////////////////////////////////////////////////////////////////
/// @brief Contains all data generated by the HS and passed to the
/// tessellator and DS.
struct TessellationThreadLocalData
{
    SWR_HS_CONTEXT hsContext;
    ScalarPatch patchData[KNOB_SIMD_WIDTH];
    void* pTxCtx;
    size_t tsCtxSize;

    simdscalar* pDSOutput;
    size_t numDSOutputVectors;
};

THREAD TessellationThreadLocalData* gt_pTessellationThreadData = nullptr;

//////////////////////////////////////////////////////////////////////////
/// @brief Allocate tessellation data for this worker thread.
INLINE
static void AllocateTessellationData(SWR_CONTEXT* pContext)
{
    /// @TODO - Don't use thread local storage.  Use Worker local storage instead.
    if (gt_pTessellationThreadData == nullptr)
    {
        gt_pTessellationThreadData = (TessellationThreadLocalData*)
            AlignedMalloc(sizeof(TessellationThreadLocalData), 64);
        memset(gt_pTessellationThreadData, 0, sizeof(*gt_pTessellationThreadData));
    }
}

//////////////////////////////////////////////////////////////////////////
/// @brief Implements Tessellation Stages.
/// @param pDC - pointer to draw context.
/// @param workerId - thread's worker id. Even thread has a unique id.
/// @param pa - The primitive assembly object.
/// @param pGsOut - output stream for GS
template <
    typename HasGeometryShaderT,
    typename HasStreamOutT,
    typename HasRastT>
static void TessellationStages(
    DRAW_CONTEXT *pDC,
    uint32_t workerId,
    PA_STATE& pa,
    void* pGsOut,
    void* pCutBuffer,
    void* pCutStreamBuffer,
    uint32_t* pSoPrimData,
    simdscalari primID)
{
    SWR_CONTEXT *pContext = pDC->pContext;
    const API_STATE& state = GetApiState(pDC);
    const SWR_TS_STATE& tsState = state.tsState;

    SWR_ASSERT(gt_pTessellationThreadData);

    HANDLE tsCtx = TSInitCtx(
        tsState.domain,
        tsState.partitioning,
        tsState.tsOutputTopology,
        gt_pTessellationThreadData->pTxCtx,
        gt_pTessellationThreadData->tsCtxSize);
    if (tsCtx == nullptr)
    {
        gt_pTessellationThreadData->pTxCtx = AlignedMalloc(gt_pTessellationThreadData->tsCtxSize, 64);
        tsCtx = TSInitCtx(
            tsState.domain,
            tsState.partitioning,
            tsState.tsOutputTopology,
            gt_pTessellationThreadData->pTxCtx,
            gt_pTessellationThreadData->tsCtxSize);
    }
    SWR_ASSERT(tsCtx);

    PFN_PROCESS_PRIMS pfnClipFunc = nullptr;
    if (HasRastT::value)
    {
        switch (tsState.postDSTopology)
        {
        case TOP_TRIANGLE_LIST: pfnClipFunc = ClipTriangles; break;
        case TOP_LINE_LIST:     pfnClipFunc = ClipLines; break;
        case TOP_POINT_LIST:    pfnClipFunc = ClipPoints; break;
        default: SWR_ASSERT(false, "Unexpected DS output topology: %d", tsState.postDSTopology);
        }
    }

    SWR_HS_CONTEXT& hsContext = gt_pTessellationThreadData->hsContext;
    hsContext.pCPout = gt_pTessellationThreadData->patchData;
    hsContext.PrimitiveID = primID;

    uint32_t numVertsPerPrim = NumVertsPerPrim(pa.binTopology, false);
    // Max storage for one attribute for an entire simdprimitive
    simdvector simdattrib[MAX_NUM_VERTS_PER_PRIM];

    // assemble all attributes for the input primitives
    for (uint32_t slot = 0; slot < tsState.numHsInputAttribs; ++slot)
    {
        uint32_t attribSlot = VERTEX_ATTRIB_START_SLOT + slot;
        pa.Assemble(attribSlot, simdattrib);

        for (uint32_t i = 0; i < numVertsPerPrim; ++i)
        {
            hsContext.vert[i].attrib[attribSlot] = simdattrib[i];
        }
    }

#if defined(_DEBUG)
    memset(hsContext.pCPout, 0x90, sizeof(ScalarPatch) * KNOB_SIMD_WIDTH);
#endif

    uint32_t numPrims = pa.NumPrims();
    hsContext.mask = GenerateMask(numPrims);

    // Run the HS
    AR_BEGIN(FEHullShader, pDC->drawId);
    state.pfnHsFunc(GetPrivateState(pDC), &hsContext);
    AR_END(FEHullShader, 0);

    UPDATE_STAT_FE(HsInvocations, numPrims);

    const uint32_t* pPrimId = (const uint32_t*)&primID;

    for (uint32_t p = 0; p < numPrims; ++p)
    {
        // Run Tessellator
        SWR_TS_TESSELLATED_DATA tsData = { 0 };
        AR_BEGIN(FETessellation, pDC->drawId);
        TSTessellate(tsCtx, hsContext.pCPout[p].tessFactors, tsData);
		AR_EVENT(TessPrimCount(1));
        AR_END(FETessellation, 0);

        if (tsData.NumPrimitives == 0)
        {
            continue;
        }
        SWR_ASSERT(tsData.NumDomainPoints);

        // Allocate DS Output memory
        uint32_t requiredDSVectorInvocations = AlignUp(tsData.NumDomainPoints, KNOB_SIMD_WIDTH) / KNOB_SIMD_WIDTH;
        size_t requiredDSOutputVectors = requiredDSVectorInvocations * tsState.numDsOutputAttribs;
        size_t requiredAllocSize = sizeof(simdvector) * requiredDSOutputVectors;
        if (requiredDSOutputVectors > gt_pTessellationThreadData->numDSOutputVectors)
        {
            AlignedFree(gt_pTessellationThreadData->pDSOutput);
            gt_pTessellationThreadData->pDSOutput = (simdscalar*)AlignedMalloc(requiredAllocSize, 64);
            gt_pTessellationThreadData->numDSOutputVectors = requiredDSOutputVectors;
        }
        SWR_ASSERT(gt_pTessellationThreadData->pDSOutput);
        SWR_ASSERT(gt_pTessellationThreadData->numDSOutputVectors >= requiredDSOutputVectors);

#if defined(_DEBUG)
        memset(gt_pTessellationThreadData->pDSOutput, 0x90, requiredAllocSize);
#endif

        // Run Domain Shader
        SWR_DS_CONTEXT dsContext;
        dsContext.PrimitiveID = pPrimId[p];
        dsContext.pCpIn = &hsContext.pCPout[p];
        dsContext.pDomainU = (simdscalar*)tsData.pDomainPointsU;
        dsContext.pDomainV = (simdscalar*)tsData.pDomainPointsV;
        dsContext.pOutputData = gt_pTessellationThreadData->pDSOutput;
        dsContext.vectorStride = requiredDSVectorInvocations;

        uint32_t dsInvocations = 0;

        for (dsContext.vectorOffset = 0; dsContext.vectorOffset < requiredDSVectorInvocations; ++dsContext.vectorOffset)
        {
            dsContext.mask = GenerateMask(tsData.NumDomainPoints - dsInvocations);

            AR_BEGIN(FEDomainShader, pDC->drawId);
            state.pfnDsFunc(GetPrivateState(pDC), &dsContext);
            AR_END(FEDomainShader, 0);

            dsInvocations += KNOB_SIMD_WIDTH;
        }
        UPDATE_STAT_FE(DsInvocations, tsData.NumDomainPoints);

        PA_TESS tessPa(
            pDC,
            dsContext.pOutputData,
            dsContext.vectorStride,
            tsState.numDsOutputAttribs,
            tsData.ppIndices,
            tsData.NumPrimitives,
            tsState.postDSTopology);

        while (tessPa.HasWork())
        {
            if (HasGeometryShaderT::value)
            {
                GeometryShaderStage<HasStreamOutT, HasRastT>(
                    pDC, workerId, tessPa, pGsOut, pCutBuffer, pCutStreamBuffer, pSoPrimData,
                    _simd_set1_epi32(dsContext.PrimitiveID));
            }
            else
            {
                if (HasStreamOutT::value)
                {
                    StreamOut(pDC, tessPa, workerId, pSoPrimData, 0);
                }

                if (HasRastT::value)
                {
                    simdvector prim[3]; // Only deal with triangles, lines, or points
                    AR_BEGIN(FEPAAssemble, pDC->drawId);
#if SWR_ENABLE_ASSERTS
                    bool assemble =
#endif
                        tessPa.Assemble(VERTEX_POSITION_SLOT, prim);
                    AR_END(FEPAAssemble, 1);
                    SWR_ASSERT(assemble);

                    SWR_ASSERT(pfnClipFunc);
                    pfnClipFunc(pDC, tessPa, workerId, prim,
                        GenMask(tessPa.NumPrims()), _simd_set1_epi32(dsContext.PrimitiveID), _simd_set1_epi32(0));
                }
            }

            tessPa.NextPrim();

        } // while (tessPa.HasWork())
    } // for (uint32_t p = 0; p < numPrims; ++p)

    TSDestroyCtx(tsCtx);
}

//////////////////////////////////////////////////////////////////////////
/// @brief FE handler for SwrDraw.
/// @tparam IsIndexedT - Is indexed drawing enabled
/// @tparam HasTessellationT - Is tessellation enabled
/// @tparam HasGeometryShaderT::value - Is the geometry shader stage enabled
/// @tparam HasStreamOutT - Is stream-out enabled
/// @tparam HasRastT - Is rasterization enabled
/// @param pContext - pointer to SWR context.
/// @param pDC - pointer to draw context.
/// @param workerId - thread's worker id.
/// @param pUserData - Pointer to DRAW_WORK
template <
    typename IsIndexedT,
    typename IsCutIndexEnabledT,
    typename HasTessellationT,
    typename HasGeometryShaderT,
    typename HasStreamOutT,
    typename HasRastT>
void ProcessDraw(
    SWR_CONTEXT *pContext,
    DRAW_CONTEXT *pDC,
    uint32_t workerId,
    void *pUserData)
{

#if KNOB_ENABLE_TOSS_POINTS
    if (KNOB_TOSS_QUEUE_FE)
    {
        return;
    }
#endif

    AR_BEGIN(FEProcessDraw, pDC->drawId);

    DRAW_WORK&          work = *(DRAW_WORK*)pUserData;
    const API_STATE&    state = GetApiState(pDC);
    __m256i             vScale = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
    SWR_VS_CONTEXT      vsContext;
    simdvertex          vin;

    int indexSize = 0;
    uint32_t endVertex = work.numVerts; 

    const int32_t* pLastRequestedIndex = nullptr;
    if (IsIndexedT::value)
    {
        switch (work.type)
        {
        case R32_UINT:
            indexSize = sizeof(uint32_t);
            pLastRequestedIndex = &(work.pIB[endVertex]);
            break;
        case R16_UINT:
            indexSize = sizeof(uint16_t);
            // nasty address offset to last index
            pLastRequestedIndex = (int32_t*)(&(((uint16_t*)work.pIB)[endVertex]));
            break;
        case R8_UINT:
            indexSize = sizeof(uint8_t);
            // nasty address offset to last index
            pLastRequestedIndex = (int32_t*)(&(((uint8_t*)work.pIB)[endVertex]));
            break;
        default:
            SWR_ASSERT(0);
        }
    }
    else
    {
        // No cuts, prune partial primitives.
        endVertex = GetNumVerts(state.topology, GetNumPrims(state.topology, work.numVerts));
    }

    SWR_FETCH_CONTEXT fetchInfo = { 0 };
    fetchInfo.pStreams = &state.vertexBuffers[0];
    fetchInfo.StartInstance = work.startInstance;
    fetchInfo.StartVertex = 0;

    vsContext.pVin = &vin;

    if (IsIndexedT::value)
    {
        fetchInfo.BaseVertex = work.baseVertex;

        // if the entire index buffer isn't being consumed, set the last index
        // so that fetches < a SIMD wide will be masked off
        fetchInfo.pLastIndex = (const int32_t*)(((uint8_t*)state.indexBuffer.pIndices) + state.indexBuffer.size);
        if (pLastRequestedIndex < fetchInfo.pLastIndex)
        {
            fetchInfo.pLastIndex = pLastRequestedIndex;
        }
    }
    else
    {
        fetchInfo.StartVertex = work.startVertex;
    }

#if defined(KNOB_ENABLE_RDTSC) || defined(KNOB_ENABLE_AR)
    uint32_t numPrims = GetNumPrims(state.topology, work.numVerts);
#endif

    void* pGsOut = nullptr;
    void* pCutBuffer = nullptr;
    void* pStreamCutBuffer = nullptr;
    if (HasGeometryShaderT::value)
    {
        AllocateGsBuffers(pDC, state, &pGsOut, &pCutBuffer, &pStreamCutBuffer);
    }

    if (HasTessellationT::value)
    {
        SWR_ASSERT(state.tsState.tsEnable == true);
        SWR_ASSERT(state.pfnHsFunc != nullptr);
        SWR_ASSERT(state.pfnDsFunc != nullptr);

        AllocateTessellationData(pContext);
    }
    else
    {
        SWR_ASSERT(state.tsState.tsEnable == false);
        SWR_ASSERT(state.pfnHsFunc == nullptr);
        SWR_ASSERT(state.pfnDsFunc == nullptr);
    }

    // allocate space for streamout input prim data
    uint32_t* pSoPrimData = nullptr;
    if (HasStreamOutT::value)
    {
        pSoPrimData = (uint32_t*)pDC->pArena->AllocAligned(4096, 16);
    }

    // choose primitive assembler
    PA_FACTORY<IsIndexedT, IsCutIndexEnabledT> paFactory(pDC, state.topology, work.numVerts);
    PA_STATE& pa = paFactory.GetPA();

    /// @todo: temporarily move instance loop in the FE to ensure SO ordering
    for (uint32_t instanceNum = 0; instanceNum < work.numInstances; instanceNum++)
    {
        simdscalari vIndex;
        uint32_t  i = 0;

        if (IsIndexedT::value)
        {
            fetchInfo.pIndices = work.pIB;
        }
        else
        {
            vIndex = _simd_add_epi32(_simd_set1_epi32(work.startVertexID), vScale);
            fetchInfo.pIndices = (const int32_t*)&vIndex;
        }

        fetchInfo.CurInstance = instanceNum;
        vsContext.InstanceID = instanceNum;

        while (pa.HasWork())
        {
            // PaGetNextVsOutput currently has the side effect of updating some PA state machine state.
            // So we need to keep this outside of (i < endVertex) check.
            simdmask* pvCutIndices = nullptr;
            if (IsIndexedT::value)
            {
                pvCutIndices = &pa.GetNextVsIndices();
            }

            simdvertex& vout = pa.GetNextVsOutput();
            vsContext.pVout = &vout;

            if (i < endVertex)
            {

                // 1. Execute FS/VS for a single SIMD.
                AR_BEGIN(FEFetchShader, pDC->drawId);
                state.pfnFetchFunc(fetchInfo, vin);
                AR_END(FEFetchShader, 0);

                // forward fetch generated vertex IDs to the vertex shader
                vsContext.VertexID = fetchInfo.VertexID;

                // Setup active mask for vertex shader.
                vsContext.mask = GenerateMask(endVertex - i);

                // forward cut mask to the PA
                if (IsIndexedT::value)
                {
                    *pvCutIndices = _simd_movemask_ps(_simd_castsi_ps(fetchInfo.CutMask));
                }

                UPDATE_STAT_FE(IaVertices, GetNumInvocations(i, endVertex));

#if KNOB_ENABLE_TOSS_POINTS
                if (!KNOB_TOSS_FETCH)
#endif
                {
                    AR_BEGIN(FEVertexShader, pDC->drawId);
                    state.pfnVertexFunc(GetPrivateState(pDC), &vsContext);
                    AR_END(FEVertexShader, 0);

                    UPDATE_STAT_FE(VsInvocations, GetNumInvocations(i, endVertex));
                }
            }

            // 2. Assemble primitives given the last two SIMD.
            do
            {
                simdvector prim[MAX_NUM_VERTS_PER_PRIM];
                // PaAssemble returns false if there is not enough verts to assemble.
                AR_BEGIN(FEPAAssemble, pDC->drawId);
                bool assemble = pa.Assemble(VERTEX_POSITION_SLOT, prim);
                AR_END(FEPAAssemble, 1);

#if KNOB_ENABLE_TOSS_POINTS
                if (!KNOB_TOSS_FETCH)
#endif
                {
#if KNOB_ENABLE_TOSS_POINTS
                    if (!KNOB_TOSS_VS)
#endif
                    {
                        if (assemble)
                        {
                            UPDATE_STAT_FE(IaPrimitives, pa.NumPrims());

                            if (HasTessellationT::value)
                            {
                                TessellationStages<HasGeometryShaderT, HasStreamOutT, HasRastT>(
                                    pDC, workerId, pa, pGsOut, pCutBuffer, pStreamCutBuffer, pSoPrimData, pa.GetPrimID(work.startPrimID));
                            }
                            else if (HasGeometryShaderT::value)
                            {
                                GeometryShaderStage<HasStreamOutT, HasRastT>(
                                    pDC, workerId, pa, pGsOut, pCutBuffer, pStreamCutBuffer, pSoPrimData, pa.GetPrimID(work.startPrimID));
                            }
                            else
                            {
                                // If streamout is enabled then stream vertices out to memory.
                                if (HasStreamOutT::value)
                                {
                                    StreamOut(pDC, pa, workerId, pSoPrimData, 0);
                                }

                                if (HasRastT::value)
                                {
                                    SWR_ASSERT(pDC->pState->pfnProcessPrims);
                                    pDC->pState->pfnProcessPrims(pDC, pa, workerId, prim,
                                        GenMask(pa.NumPrims()), pa.GetPrimID(work.startPrimID), _simd_set1_epi32(0));
                                }
                            }
                        }
                    }
                }
            } while (pa.NextPrim());

            i += KNOB_SIMD_WIDTH;
            if (IsIndexedT::value)
            {
                fetchInfo.pIndices = (int*)((uint8_t*)fetchInfo.pIndices + KNOB_SIMD_WIDTH * indexSize);
            }
            else
            {
                vIndex = _simd_add_epi32(vIndex, _simd_set1_epi32(KNOB_SIMD_WIDTH));
            }
        }
        pa.Reset();
    }


    AR_END(FEProcessDraw, numPrims * work.numInstances);
}

struct FEDrawChooser
{
    typedef PFN_FE_WORK_FUNC FuncType;

    template <typename... ArgsB>
    static FuncType GetFunc()
    {
        return ProcessDraw<ArgsB...>;
    }
};


// Selector for correct templated Draw front-end function
PFN_FE_WORK_FUNC GetProcessDrawFunc(
    bool IsIndexed,
    bool IsCutIndexEnabled,
    bool HasTessellation,
    bool HasGeometryShader,
    bool HasStreamOut,
    bool HasRasterization)
{
    return TemplateArgUnroller<FEDrawChooser>::GetFunc(IsIndexed, IsCutIndexEnabled, HasTessellation, HasGeometryShader, HasStreamOut, HasRasterization);
}