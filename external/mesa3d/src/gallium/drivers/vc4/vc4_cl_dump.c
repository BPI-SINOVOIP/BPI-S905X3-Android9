/*
 * Copyright © 2014 Broadcom
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
 */

#include "util/u_math.h"
#include "util/u_prim.h"
#include "util/macros.h"
#include "vc4_context.h"

#define dump_VC4_PACKET_LINE_WIDTH dump_float
#define dump_VC4_PACKET_POINT_SIZE dump_float

static void
dump_float(void *cl, uint32_t offset, uint32_t hw_offset)
{
        void *f = cl + offset;

        fprintf(stderr, "0x%08x 0x%08x:      %f (0x%08x)\n",
                offset, hw_offset, uif(*(uint32_t *)f), *(uint32_t *)f);
}

static void
dump_VC4_PACKET_BRANCH_TO_SUB_LIST(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint32_t *addr = cl + offset;

        fprintf(stderr, "0x%08x 0x%08x:      addr 0x%08x\n",
                offset, hw_offset, *addr);
}

static void
dump_loadstore_full(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint32_t bits = *(uint32_t *)(cl + offset);

        fprintf(stderr, "0x%08x 0x%08x:      addr 0x%08x%s%s%s%s\n",
                offset, hw_offset,
                bits & ~0xf,
                (bits & VC4_LOADSTORE_FULL_RES_DISABLE_CLEAR_ALL) ? "" : " clear",
                (bits & VC4_LOADSTORE_FULL_RES_DISABLE_ZS) ? "" : " zs",
                (bits & VC4_LOADSTORE_FULL_RES_DISABLE_COLOR) ? "" : " color",
                (bits & VC4_LOADSTORE_FULL_RES_EOF) ? " eof" : "");
}

static void
dump_VC4_PACKET_LOAD_FULL_RES_TILE_BUFFER(void *cl, uint32_t offset, uint32_t hw_offset)
{
        dump_loadstore_full(cl, offset, hw_offset);
}

static void
dump_VC4_PACKET_STORE_FULL_RES_TILE_BUFFER(void *cl, uint32_t offset, uint32_t hw_offset)
{
        dump_loadstore_full(cl, offset, hw_offset);
}

static void
dump_loadstore_general(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint8_t *bytes = cl + offset;
        uint32_t *addr = cl + offset + 2;

        const char *fullvg = "";
        const char *fullzs = "";
        const char *fullcolor = "";
        const char *buffer = "???";

        switch ((bytes[0] & 0x7)){
        case 0:
                buffer = "none";
                break;
        case 1:
                buffer = "color";
                break;
        case 2:
                buffer = "zs";
                break;
        case 3:
                buffer = "z";
                break;
        case 4:
                buffer = "vgmask";
                break;
        case 5:
                buffer = "full";
                if (*addr & (1 << 0))
                        fullcolor = " !color";
                if (*addr & (1 << 1))
                        fullzs = " !zs";
                if (*addr & (1 << 2))
                        fullvg = " !vgmask";
                break;
        }

        const char *tiling = "???";
        switch ((bytes[0] >> 4) & 7) {
        case 0:
                tiling = "linear";
                break;
        case 1:
                tiling = "T";
                break;
        case 2:
                tiling = "LT";
                break;
        }

        const char *format = "???";
        switch (bytes[1] & 3) {
        case 0:
                format = "RGBA8888";
                break;
        case 1:
                format = "BGR565_DITHER";
                break;
        case 2:
                format = "BGR565";
                break;
        }

        fprintf(stderr, "0x%08x 0x%08x: 0x%02x %s %s\n",
                offset + 0, hw_offset + 0, bytes[0],
                buffer, tiling);

        fprintf(stderr, "0x%08x 0x%08x: 0x%02x %s\n",
                offset + 1, hw_offset + 1, bytes[1],
                format);

        fprintf(stderr, "0x%08x 0x%08x:      addr 0x%08x %s%s%s%s\n",
                offset + 2, hw_offset + 2, *addr & ~15,
                fullcolor, fullzs, fullvg,
                (*addr & (1 << 3)) ? " EOF" : "");
}

static void
dump_VC4_PACKET_STORE_TILE_BUFFER_GENERAL(void *cl, uint32_t offset, uint32_t hw_offset)
{
        dump_loadstore_general(cl, offset, hw_offset);
}

static void
dump_VC4_PACKET_LOAD_TILE_BUFFER_GENERAL(void *cl, uint32_t offset, uint32_t hw_offset)
{
        dump_loadstore_general(cl, offset, hw_offset);
}

static void
dump_VC4_PACKET_GL_INDEXED_PRIMITIVE(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint8_t *b = cl + offset;
        uint32_t *count = cl + offset + 1;
        uint32_t *ib_offset = cl + offset + 5;
        uint32_t *max_index = cl + offset + 9;

        fprintf(stderr, "0x%08x 0x%08x:      0x%02x %s %s\n",
                offset, hw_offset,
                b[0], (b[0] & VC4_INDEX_BUFFER_U16) ? "16-bit" : "8-bit",
                u_prim_name(b[0] & 0x7));
        fprintf(stderr, "0x%08x 0x%08x:           %d verts\n",
                offset + 1, hw_offset + 1, *count);
        fprintf(stderr, "0x%08x 0x%08x:      0x%08x IB offset\n",
                offset + 5, hw_offset + 5, *ib_offset);
        fprintf(stderr, "0x%08x 0x%08x:      0x%08x max index\n",
                offset + 9, hw_offset + 9, *max_index);
}

static void
dump_VC4_PACKET_GL_ARRAY_PRIMITIVE(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint8_t *b = cl + offset;
        uint32_t *count = cl + offset + 1;
        uint32_t *start = cl + offset + 5;

        fprintf(stderr, "0x%08x 0x%08x:      0x%02x %s\n",
                offset, hw_offset, b[0], u_prim_name(b[0] & 0x7));
        fprintf(stderr, "0x%08x 0x%08x:      %d verts\n",
                offset + 1, hw_offset + 1, *count);
        fprintf(stderr, "0x%08x 0x%08x:      0x%08x start\n",
                offset + 5, hw_offset + 5, *start);
}

static void
dump_VC4_PACKET_FLAT_SHADE_FLAGS(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint32_t *bits = cl + offset;

        fprintf(stderr, "0x%08x 0x%08x:      bits 0x%08x\n",
                offset, hw_offset, *bits);
}

static void
dump_VC4_PACKET_VIEWPORT_OFFSET(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint16_t *o = cl + offset;

        fprintf(stderr, "0x%08x 0x%08x:      %f, %f (0x%04x, 0x%04x)\n",
                offset, hw_offset,
                o[0] / 16.0, o[1] / 16.0,
                o[0], o[1]);
}

static void
dump_VC4_PACKET_CLIPPER_XY_SCALING(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint32_t *scale = cl + offset;

        fprintf(stderr, "0x%08x 0x%08x:      %f, %f (%f, %f, 0x%08x, 0x%08x)\n",
                offset, hw_offset,
                uif(scale[0]) / 16.0, uif(scale[1]) / 16.0,
                uif(scale[0]), uif(scale[1]),
                scale[0], scale[1]);
}

static void
dump_VC4_PACKET_CLIPPER_Z_SCALING(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint32_t *translate = cl + offset;
        uint32_t *scale = cl + offset + 8;

        fprintf(stderr, "0x%08x 0x%08x:      %f, %f (0x%08x, 0x%08x)\n",
                offset, hw_offset,
                uif(translate[0]), uif(translate[1]),
                translate[0], translate[1]);

        fprintf(stderr, "0x%08x 0x%08x:      %f, %f (0x%08x, 0x%08x)\n",
                offset + 8, hw_offset + 8,
                uif(scale[0]), uif(scale[1]),
                scale[0], scale[1]);
}

static void
dump_VC4_PACKET_TILE_BINNING_MODE_CONFIG(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint32_t *tile_alloc_addr = cl + offset;
        uint32_t *tile_alloc_size = cl + offset + 4;
        uint32_t *tile_state_addr = cl + offset + 8;
        uint8_t *bin_x = cl + offset + 12;
        uint8_t *bin_y = cl + offset + 13;
        uint8_t *flags = cl + offset + 14;

        fprintf(stderr, "0x%08x 0x%08x:       tile alloc addr 0x%08x\n",
                offset, hw_offset,
                *tile_alloc_addr);

        fprintf(stderr, "0x%08x 0x%08x:       tile alloc size %db\n",
                offset + 4, hw_offset + 4,
                *tile_alloc_size);

        fprintf(stderr, "0x%08x 0x%08x:       tile state addr 0x%08x\n",
                offset + 8, hw_offset + 8,
                *tile_state_addr);

        fprintf(stderr, "0x%08x 0x%08x:       tiles (%d, %d)\n",
                offset + 12, hw_offset + 12,
                *bin_x, *bin_y);

        fprintf(stderr, "0x%08x 0x%08x:       flags 0x%02x\n",
                offset + 14, hw_offset + 14,
                *flags);
}

static void
dump_VC4_PACKET_TILE_RENDERING_MODE_CONFIG(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint32_t *render_offset = cl + offset;
        uint16_t *shorts = cl + offset + 4;
        uint8_t *bytes = cl + offset + 8;

        fprintf(stderr, "0x%08x 0x%08x:       color offset 0x%08x\n",
                offset, hw_offset,
                *render_offset);

        fprintf(stderr, "0x%08x 0x%08x:       width %d\n",
                offset + 4, hw_offset + 4,
                shorts[0]);

        fprintf(stderr, "0x%08x 0x%08x:       height %d\n",
                offset + 6, hw_offset + 6,
                shorts[1]);

        const char *format = "???";
        switch (VC4_GET_FIELD(shorts[2], VC4_RENDER_CONFIG_FORMAT)) {
        case VC4_RENDER_CONFIG_FORMAT_BGR565_DITHERED:
                format = "BGR565_DITHERED";
                break;
        case VC4_RENDER_CONFIG_FORMAT_RGBA8888:
                format = "RGBA8888";
                break;
        case VC4_RENDER_CONFIG_FORMAT_BGR565:
                format = "BGR565";
                break;
        }
        if (shorts[2] & VC4_RENDER_CONFIG_TILE_BUFFER_64BIT)
                format = "64bit";

        const char *tiling = "???";
        switch (VC4_GET_FIELD(shorts[2], VC4_RENDER_CONFIG_MEMORY_FORMAT)) {
        case VC4_TILING_FORMAT_LINEAR:
                tiling = "linear";
                break;
        case VC4_TILING_FORMAT_T:
                tiling = "T";
                break;
        case VC4_TILING_FORMAT_LT:
                tiling = "LT";
                break;
        }

        fprintf(stderr, "0x%08x 0x%08x: 0x%02x %s %s %s %s\n",
                offset + 8, hw_offset + 8,
                bytes[0],
                format, tiling,
                (shorts[2] & VC4_RENDER_CONFIG_MS_MODE_4X) ? "ms" : "ss",
                (shorts[2] & VC4_RENDER_CONFIG_DECIMATE_MODE_4X) ?
                "ms_decimate" : "ss_decimate");

        const char *earlyz = "";
        if (shorts[2] & VC4_RENDER_CONFIG_EARLY_Z_COVERAGE_DISABLE) {
                earlyz = "early_z disabled";
        } else {
                if (shorts[2] & VC4_RENDER_CONFIG_EARLY_Z_DIRECTION_G)
                        earlyz = "early_z >";
                else
                        earlyz = "early_z <";
        }

        fprintf(stderr, "0x%08x 0x%08x: 0x%02x %s\n",
                offset + 9, hw_offset + 9,
                bytes[1],
                earlyz);
}

static void
dump_VC4_PACKET_TILE_COORDINATES(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint8_t *tilecoords = cl + offset;

        fprintf(stderr, "0x%08x 0x%08x:      %d, %d\n",
                offset, hw_offset, tilecoords[0], tilecoords[1]);
}

static void
dump_VC4_PACKET_GEM_HANDLES(void *cl, uint32_t offset, uint32_t hw_offset)
{
        uint32_t *handles = cl + offset;

        fprintf(stderr, "0x%08x 0x%08x:      handle 0: %d, handle 1: %d\n",
                offset, hw_offset, handles[0], handles[1]);
}

#define PACKET_DUMP(name) [name] = { #name, name ## _SIZE, dump_##name }
#define PACKET(name) [name] = { #name, name ## _SIZE, NULL }

static const struct packet_info {
        const char *name;
        uint8_t size;
        void (*dump_func)(void *cl, uint32_t offset, uint32_t hw_offset);
} packet_info[] = {
        PACKET(VC4_PACKET_HALT),
        PACKET(VC4_PACKET_NOP),

        PACKET(VC4_PACKET_FLUSH),
        PACKET(VC4_PACKET_FLUSH_ALL),
        PACKET(VC4_PACKET_START_TILE_BINNING),
        PACKET(VC4_PACKET_INCREMENT_SEMAPHORE),
        PACKET(VC4_PACKET_WAIT_ON_SEMAPHORE),

        PACKET(VC4_PACKET_BRANCH),
        PACKET_DUMP(VC4_PACKET_BRANCH_TO_SUB_LIST),

        PACKET(VC4_PACKET_STORE_MS_TILE_BUFFER),
        PACKET(VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF),
        PACKET_DUMP(VC4_PACKET_STORE_FULL_RES_TILE_BUFFER),
        PACKET_DUMP(VC4_PACKET_LOAD_FULL_RES_TILE_BUFFER),
        PACKET_DUMP(VC4_PACKET_STORE_TILE_BUFFER_GENERAL),
        PACKET_DUMP(VC4_PACKET_LOAD_TILE_BUFFER_GENERAL),

        PACKET_DUMP(VC4_PACKET_GL_INDEXED_PRIMITIVE),
        PACKET_DUMP(VC4_PACKET_GL_ARRAY_PRIMITIVE),

        PACKET(VC4_PACKET_COMPRESSED_PRIMITIVE),
        PACKET(VC4_PACKET_CLIPPED_COMPRESSED_PRIMITIVE),

        PACKET(VC4_PACKET_PRIMITIVE_LIST_FORMAT),

        PACKET(VC4_PACKET_GL_SHADER_STATE),
        PACKET(VC4_PACKET_NV_SHADER_STATE),
        PACKET(VC4_PACKET_VG_SHADER_STATE),

        PACKET(VC4_PACKET_CONFIGURATION_BITS),
        PACKET_DUMP(VC4_PACKET_FLAT_SHADE_FLAGS),
        PACKET_DUMP(VC4_PACKET_POINT_SIZE),
        PACKET_DUMP(VC4_PACKET_LINE_WIDTH),
        PACKET(VC4_PACKET_RHT_X_BOUNDARY),
        PACKET(VC4_PACKET_DEPTH_OFFSET),
        PACKET(VC4_PACKET_CLIP_WINDOW),
        PACKET_DUMP(VC4_PACKET_VIEWPORT_OFFSET),
        PACKET(VC4_PACKET_Z_CLIPPING),
        PACKET_DUMP(VC4_PACKET_CLIPPER_XY_SCALING),
        PACKET_DUMP(VC4_PACKET_CLIPPER_Z_SCALING),

        PACKET_DUMP(VC4_PACKET_TILE_BINNING_MODE_CONFIG),
        PACKET_DUMP(VC4_PACKET_TILE_RENDERING_MODE_CONFIG),
        PACKET(VC4_PACKET_CLEAR_COLORS),
        PACKET_DUMP(VC4_PACKET_TILE_COORDINATES),

        PACKET_DUMP(VC4_PACKET_GEM_HANDLES),
};

void
vc4_dump_cl(void *cl, uint32_t size, bool is_render)
{
        uint32_t offset = 0, hw_offset = 0;
        uint8_t *cmds = cl;

        while (offset < size) {
                uint8_t header = cmds[offset];

                if (header >= ARRAY_SIZE(packet_info) ||
                    !packet_info[header].name) {
                        fprintf(stderr, "0x%08x 0x%08x: Unknown packet 0x%02x (%d)!\n",
                                offset, hw_offset, header, header);
                        return;
                }

                const struct packet_info *p = packet_info + header;
                fprintf(stderr, "0x%08x 0x%08x: 0x%02x %s\n",
                        offset,
                        header != VC4_PACKET_GEM_HANDLES ? hw_offset : 0,
                        header, p->name);

                if (offset + p->size <= size &&
                    p->dump_func) {
                        p->dump_func(cmds, offset + 1, hw_offset + 1);
                } else {
                        for (uint32_t i = 1; i < p->size; i++) {
                                if (offset + i >= size) {
                                        fprintf(stderr, "0x%08x 0x%08x: CL overflow!\n",
                                                offset + i, hw_offset + i);
                                        return;
                                }
                                fprintf(stderr, "0x%08x 0x%08x: 0x%02x\n",
                                        offset + i,
                                        header != VC4_PACKET_GEM_HANDLES ? hw_offset + i : 0,
                                        cmds[offset + i]);
                        }
                }

                switch (header) {
                case VC4_PACKET_HALT:
                case VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF:
                        return;
                default:
                        break;
                }

                offset += p->size;
                if (header != VC4_PACKET_GEM_HANDLES)
                        hw_offset += p->size;
        }
}

