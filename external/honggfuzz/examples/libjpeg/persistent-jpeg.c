#ifdef __cplusplus
extern "C" {
#endif

#include <fcntl.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libhfuzz/libhfuzz.h>

#include "cderror.h"
#include "jpeglib.h"

struct jpeg_decompress_struct cinfo;
int null_fd = -1;

struct jpegErrorManager {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

struct jpegErrorManager jerr;

void jpegErrorExit(j_common_ptr cinfo)
{
    struct jpegErrorManager* myerr = (struct jpegErrorManager*)cinfo->err;
    longjmp(myerr->setjmp_buffer, 1);
}

static const char* const cdjpeg_message_table[] = {
#include "cderror.h"
    NULL
};

int LLVMFuzzerInitialize(int* argc, char*** argv)
{
    null_fd = open("/dev/null", O_WRONLY);

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    jerr.pub.addon_message_table = cdjpeg_message_table;
    jerr.pub.first_addon_message = JMSG_FIRSTADDONCODE;
    jerr.pub.last_addon_message = JMSG_LASTADDONCODE;

    jpeg_create_decompress(&cinfo);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t* buf, size_t len)
{
    if (setjmp(jerr.setjmp_buffer)) {
        goto out;
    }

    jpeg_mem_src(&cinfo, buf, len);

    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        goto out;
    }

    if (cinfo.output_height > 10000 || cinfo.output_width > 10000) {
        goto out;
    }

    cinfo.mem->max_memory_to_use = (1024 * 1024 * 1024);
    cinfo.mem->max_alloc_chunk = (1024 * 128 * 256);

    jpeg_start_decompress(&cinfo);

    int row_stride = cinfo.output_width * cinfo.output_components;
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
    while (cinfo.output_scanline < cinfo.output_height) {
#if defined(__clang__)
#if __has_feature(memory_sanitizer)
        __msan_poison(buffer[0], row_stride);
#endif /* __has_feature(memory_sanitizer) */
#endif /* defined(__clang__) */
        jpeg_read_scanlines(&cinfo, buffer, 1);
        write(null_fd, buffer[0], row_stride);
    }

out:
    jpeg_abort_decompress(&cinfo);
    return 0;
}

#ifdef __cplusplus
}
#endif
