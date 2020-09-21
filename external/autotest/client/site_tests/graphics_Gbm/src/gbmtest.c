/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <gbm.h>

#define CHECK(cond) do {\
	if (!(cond)) {\
		printf("CHECK failed in %s() %s:%d\n", __func__, __FILE__, __LINE__);\
		return 0;\
	}\
} while(0)

#define HANDLE_EINTR(x)                                                  \
	({                                                               \
		int eintr_wrapper_counter = 0;                           \
		int eintr_wrapper_result;                                \
		do {                                                     \
			eintr_wrapper_result = (x);                      \
		} while (eintr_wrapper_result == -1 && errno == EINTR && \
			 eintr_wrapper_counter++ < 100);                 \
		eintr_wrapper_result;                                    \
	})

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(*(A)))

#define ENODRM	   -1
#define ENODISPLAY -2

static int fd;
static struct gbm_device *gbm;

static const uint32_t format_list[] = {
	GBM_FORMAT_C8,
	GBM_FORMAT_RGB332,
	GBM_FORMAT_BGR233,
	GBM_FORMAT_XRGB4444,
	GBM_FORMAT_XBGR4444,
	GBM_FORMAT_RGBX4444,
	GBM_FORMAT_BGRX4444,
	GBM_FORMAT_ARGB4444,
	GBM_FORMAT_ABGR4444,
	GBM_FORMAT_RGBA4444,
	GBM_FORMAT_BGRA4444,
	GBM_FORMAT_XRGB1555,
	GBM_FORMAT_XBGR1555,
	GBM_FORMAT_RGBX5551,
	GBM_FORMAT_BGRX5551,
	GBM_FORMAT_ARGB1555,
	GBM_FORMAT_ABGR1555,
	GBM_FORMAT_RGBA5551,
	GBM_FORMAT_BGRA5551,
	GBM_FORMAT_RGB565,
	GBM_FORMAT_BGR565,
	GBM_FORMAT_RGB888,
	GBM_FORMAT_BGR888,
	GBM_FORMAT_XRGB8888,
	GBM_FORMAT_XBGR8888,
	GBM_FORMAT_RGBX8888,
	GBM_FORMAT_BGRX8888,
	GBM_FORMAT_ARGB8888,
	GBM_FORMAT_ABGR8888,
	GBM_FORMAT_RGBA8888,
	GBM_FORMAT_BGRA8888,
	GBM_FORMAT_XRGB2101010,
	GBM_FORMAT_XBGR2101010,
	GBM_FORMAT_RGBX1010102,
	GBM_FORMAT_BGRX1010102,
	GBM_FORMAT_ARGB2101010,
	GBM_FORMAT_ABGR2101010,
	GBM_FORMAT_RGBA1010102,
	GBM_FORMAT_BGRA1010102,
	GBM_FORMAT_YUYV,
	GBM_FORMAT_YVYU,
	GBM_FORMAT_UYVY,
	GBM_FORMAT_VYUY,
	GBM_FORMAT_AYUV,
	GBM_FORMAT_NV12,
	GBM_FORMAT_YVU420,
};

struct plane_info {
	uint32_t bits_per_pixel;
	uint32_t subsample_rate;
};

#define MAX_PLANES 3
struct format_info {
	uint32_t pixel_format;
	int num_planes;
	struct plane_info planes[MAX_PLANES];
};

/* Bits per pixel for each. */
static const struct format_info format_info_list[] = {
	{GBM_FORMAT_C8, 1, {{8, 1}}},
	{GBM_FORMAT_RGB332, 1, {{8, 1}}},
	{GBM_FORMAT_BGR233, 1, {{8, 1}}},
	{GBM_FORMAT_XRGB4444, 1, {{16, 1}}},
	{GBM_FORMAT_XBGR4444, 1, {{16, 1}}},
	{GBM_FORMAT_RGBX4444, 1, {{16, 1}}},
	{GBM_FORMAT_BGRX4444, 1, {{16, 1}}},
	{GBM_FORMAT_ARGB4444, 1, {{16, 1}}},
	{GBM_FORMAT_ABGR4444, 1, {{16, 1}}},
	{GBM_FORMAT_RGBA4444, 1, {{16, 1}}},
	{GBM_FORMAT_BGRA4444, 1, {{16, 1}}},
	{GBM_FORMAT_XRGB1555, 1, {{16, 1}}},
	{GBM_FORMAT_XBGR1555, 1, {{16, 1}}},
	{GBM_FORMAT_RGBX5551, 1, {{16, 1}}},
	{GBM_FORMAT_BGRX5551, 1, {{16, 1}}},
	{GBM_FORMAT_ARGB1555, 1, {{16, 1}}},
	{GBM_FORMAT_ABGR1555, 1, {{16, 1}}},
	{GBM_FORMAT_RGBA5551, 1, {{16, 1}}},
	{GBM_FORMAT_BGRA5551, 1, {{16, 1}}},
	{GBM_FORMAT_RGB565, 1, {{16, 1}}},
	{GBM_FORMAT_BGR565, 1, {{16, 1}}},
	{GBM_FORMAT_RGB888, 1, {{24, 1}}},
	{GBM_FORMAT_BGR888, 1, {{24, 1}}},
	{GBM_FORMAT_XRGB8888, 1, {{32, 1}}},
	{GBM_FORMAT_XBGR8888, 1, {{32, 1}}},
	{GBM_FORMAT_RGBX8888, 1, {{32, 1}}},
	{GBM_FORMAT_BGRX8888, 1, {{32, 1}}},
	{GBM_FORMAT_ARGB8888, 1, {{32, 1}}},
	{GBM_FORMAT_ABGR8888, 1, {{32, 1}}},
	{GBM_FORMAT_RGBA8888, 1, {{32, 1}}},
	{GBM_FORMAT_BGRA8888, 1, {{32, 1}}},
	{GBM_FORMAT_XRGB2101010, 1, {{32, 1}}},
	{GBM_FORMAT_XBGR2101010, 1, {{32, 1}}},
	{GBM_FORMAT_RGBX1010102, 1, {{32, 1}}},
	{GBM_FORMAT_BGRX1010102, 1, {{32, 1}}},
	{GBM_FORMAT_ARGB2101010, 1, {{32, 1}}},
	{GBM_FORMAT_ABGR2101010, 1, {{32, 1}}},
	{GBM_FORMAT_RGBA1010102, 1, {{32, 1}}},
	{GBM_FORMAT_BGRA1010102, 1, {{32, 1}}},
	{GBM_FORMAT_YUYV, 1, {{16, 1}}},
	{GBM_FORMAT_YVYU, 1, {{16, 1}}},
	{GBM_FORMAT_UYVY, 1, {{16, 1}}},
	{GBM_FORMAT_VYUY, 1, {{16, 1}}},
	{GBM_FORMAT_AYUV, 1, {{32, 1}}},
	{GBM_FORMAT_NV12, 2, {{8, 1}, {16, 2}}},
	{GBM_FORMAT_YVU420, 3, {{8, 1}, {8, 2}, {8,2}}},
};

static const uint32_t usage_list[] = {
	GBM_BO_USE_SCANOUT,
	GBM_BO_USE_CURSOR_64X64,
	GBM_BO_USE_RENDERING,
	GBM_BO_USE_LINEAR,
	GBM_BO_USE_SW_READ_OFTEN,
	GBM_BO_USE_SW_READ_RARELY,
	GBM_BO_USE_SW_WRITE_OFTEN,
	GBM_BO_USE_SW_WRITE_RARELY,
};

static const uint32_t buffer_list[] = {
	GBM_BO_USE_SCANOUT | GBM_BO_USE_SW_READ_RARELY | GBM_BO_USE_SW_WRITE_RARELY,
	GBM_BO_USE_RENDERING | GBM_BO_USE_SW_READ_RARELY | GBM_BO_USE_SW_WRITE_RARELY,
	GBM_BO_USE_SW_READ_RARELY | GBM_BO_USE_SW_WRITE_RARELY,
	GBM_BO_USE_SW_READ_RARELY | GBM_BO_USE_SW_WRITE_RARELY | GBM_BO_USE_TEXTURING,
	GBM_BO_USE_SW_READ_RARELY | GBM_BO_USE_SW_WRITE_RARELY | GBM_BO_USE_TEXTURING,

	GBM_BO_USE_RENDERING | GBM_BO_USE_SW_READ_RARELY | GBM_BO_USE_SW_WRITE_RARELY |
	GBM_BO_USE_TEXTURING,

	GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT | GBM_BO_USE_SW_READ_RARELY |
	GBM_BO_USE_SW_WRITE_RARELY,

	GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT | GBM_BO_USE_SW_READ_RARELY |
	GBM_BO_USE_SW_WRITE_RARELY | GBM_BO_USE_TEXTURING,
};

static int check_bo(struct gbm_bo *bo)
{
	uint32_t format;
	size_t num_planes, plane;
	int fd;
	int i;

	CHECK(bo);
	CHECK(gbm_bo_get_width(bo) >= 0);
	CHECK(gbm_bo_get_height(bo) >= 0);
	CHECK(gbm_bo_get_stride(bo) >= gbm_bo_get_width(bo));

	format = gbm_bo_get_format(bo);
	for (i = 0; i < ARRAY_SIZE(format_list); i++)
		if (format_list[i] == format)
			break;
	CHECK(i < ARRAY_SIZE(format_list));

	num_planes = gbm_bo_get_num_planes(bo);
	if (format == GBM_FORMAT_NV12)
		CHECK(num_planes == 2);
	else if (format == GBM_FORMAT_YVU420)
		CHECK(num_planes == 3);
	else
		CHECK(num_planes == 1);

	CHECK(gbm_bo_get_plane_handle(bo, 0).u32 == gbm_bo_get_handle(bo).u32);

	CHECK(gbm_bo_get_plane_offset(bo, 0) == 0);
	CHECK(gbm_bo_get_plane_size(bo, 0) >=
		gbm_bo_get_width(bo) * gbm_bo_get_height(bo));
	CHECK(gbm_bo_get_plane_stride(bo, 0) == gbm_bo_get_stride(bo));

	for (plane = 0; plane < num_planes; plane++) {
		CHECK(gbm_bo_get_plane_handle(bo, plane).u32);

		fd = gbm_bo_get_plane_fd(bo, plane);
		CHECK(fd > 0);
		close(fd);

		gbm_bo_get_plane_offset(bo, plane);
		CHECK(gbm_bo_get_plane_size(bo, plane));
		CHECK(gbm_bo_get_plane_stride(bo, plane));
	}

	return 1;
}

static drmModeConnector *find_first_connected_connector(int fd,
							drmModeRes *resources)
{
	int i;
	for (i = 0; i < resources->count_connectors; i++) {
		drmModeConnector *connector;

		connector = drmModeGetConnector(fd, resources->connectors[i]);
		if (connector) {
			if ((connector->count_modes > 0) &&
					(connector->connection == DRM_MODE_CONNECTED))
				return connector;

			drmModeFreeConnector(connector);
		}
	}
	return NULL;
}

static int drm_open()
{
	int fd;
	unsigned i;
	bool has_drm_device = false;

	for (i = 0; i < DRM_MAX_MINOR; i++) {
		char* dev_name;
		drmModeRes *res = NULL;
		int ret;

		ret = asprintf(&dev_name, DRM_DEV_NAME, DRM_DIR_NAME, i);
		if (ret < 0)
			continue;

		fd = open(dev_name, O_RDWR, 0);
		free(dev_name);
		if (fd < 0)
			continue;

		res = drmModeGetResources(fd);
		if (!res) {
			drmClose(fd);
			continue;
		}

		if (res->count_crtcs > 0 && res->count_connectors > 0) {
			has_drm_device = true;
			if (find_first_connected_connector(fd, res)) {
				drmModeFreeResources(res);
				return fd;
			}
		}

		drmClose(fd);
		drmModeFreeResources(res);
	}

	if (has_drm_device)
		return ENODISPLAY;
	else
		return ENODRM;
}

static int drm_open_vgem()
{
	const char g_sys_card_path_format[] =
		"/sys/bus/platform/devices/vgem/drm/card%d";
	const char g_dev_card_path_format[] =
		"/dev/dri/card%d";
	char *name;
	int i, fd;

	for (i = 0; i < 16; i++) {
		struct stat _stat;
		int ret;
		ret = asprintf(&name, g_sys_card_path_format, i);
		assert(ret != -1);

		if (stat(name, &_stat) == -1) {
			free(name);
			continue;
		}

		free(name);
		ret = asprintf(&name, g_dev_card_path_format, i);
		assert(ret != -1);

		fd = open(name, O_RDWR);
		free(name);
		if (fd == -1) {
			return -1;
		}
		return fd;
	}
	return -1;
}

static int create_vgem_bo(int fd, size_t size, uint32_t * handle)
{
	struct drm_mode_create_dumb create;
	int ret;

	memset(&create, 0, sizeof(create));
	create.height = size;
	create.width = 1;
	create.bpp = 8;

	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
	if (ret)
		return ret;

	assert(create.size >= size);

	*handle = create.handle;

	return 0;
}

/*
 * Tests initialization.
 */
static int test_init()
{
	fd = drm_open();
	if (fd == ENODISPLAY)
		return ENODISPLAY;
	CHECK(fd >= 0);

	gbm = gbm_create_device(fd);

	CHECK(gbm_device_get_fd(gbm) == fd);

	const char* backend_name = gbm_device_get_backend_name(gbm);

	CHECK(backend_name);

	return 1;
}

/*
 * Tests reinitialization.
 */
static int test_reinit()
{
	gbm_device_destroy(gbm);
	close(fd);

	fd = drm_open();
	CHECK(fd >= 0);

	gbm = gbm_create_device(fd);

	CHECK(gbm_device_get_fd(gbm) == fd);

	struct gbm_bo *bo;
	bo = gbm_bo_create(gbm, 1024, 1024, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
	CHECK(check_bo(bo));
	gbm_bo_destroy(bo);

	return 1;
}

/*
 * Tests repeated alloc/free.
 */
static int test_alloc_free()
{
	int i;
	for(i = 0; i < 1000; i++) {
		struct gbm_bo *bo;
		bo = gbm_bo_create(gbm, 1024, 1024, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
		CHECK(check_bo(bo));
		gbm_bo_destroy(bo);
	}
	return 1;
}

/*
 * Tests that we can allocate different buffer dimensions.
 */
static int test_alloc_free_sizes()
{
	int i;
	for(i = 1; i < 1920; i++) {
		struct gbm_bo *bo;
		bo = gbm_bo_create(gbm, i, i, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
		CHECK(check_bo(bo));
		gbm_bo_destroy(bo);
	}

	for(i = 1; i < 1920; i++) {
		struct gbm_bo *bo;
		bo = gbm_bo_create(gbm, i, 1, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
		CHECK(check_bo(bo));
		gbm_bo_destroy(bo);
	}

	for(i = 1; i < 1920; i++) {
		struct gbm_bo *bo;
		bo = gbm_bo_create(gbm, 1, i, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
		CHECK(check_bo(bo));
		gbm_bo_destroy(bo);
	}

	return 1;
}

/*
 * Tests that we can allocate different buffer formats.
 */
static int test_alloc_free_formats()
{
	int i;

	for(i = 0; i < ARRAY_SIZE(format_list); i++) {
		uint32_t format = format_list[i];
		if (gbm_device_is_format_supported(gbm, format, GBM_BO_USE_RENDERING)) {
			struct gbm_bo *bo;
			bo = gbm_bo_create(gbm, 1024, 1024, format, GBM_BO_USE_RENDERING);
			CHECK(check_bo(bo));
			gbm_bo_destroy(bo);
		}
	}

	return 1;
}

/*
 * Tests that we find at least one working format for each usage.
 */
static int test_alloc_free_usage()
{
	int i, j;

	for(i = 0; i < ARRAY_SIZE(usage_list); i++) {
		uint32_t usage = usage_list[i];
		int found = 0;
		for(j = 0; j < ARRAY_SIZE(format_list); j++) {
			uint32_t format = format_list[j];
			if (gbm_device_is_format_supported(gbm, format, usage)) {
				struct gbm_bo *bo;
				if (usage == GBM_BO_USE_CURSOR_64X64)
					bo = gbm_bo_create(gbm, 64, 64, format, usage);
				else
					bo = gbm_bo_create(gbm, 1024, 1024, format, usage);
				CHECK(check_bo(bo));
				found = 1;
				gbm_bo_destroy(bo);
			}
		}
		CHECK(found);
	}

	return 1;
}

/*
 * Tests user data.
 */
static int been_there1;
static int been_there2;

void destroy_data1(struct gbm_bo *bo, void *data)
{
	been_there1 = 1;
}

void destroy_data2(struct gbm_bo *bo, void *data)
{
	been_there2 = 1;
}

static int test_user_data()
{
	struct gbm_bo *bo1, *bo2;
	char *data1, *data2;

	been_there1 = 0;
	been_there2 = 0;

	bo1 = gbm_bo_create(gbm, 1024, 1024, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
	bo2 = gbm_bo_create(gbm, 1024, 1024, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
	data1 = (char*)malloc(1);
	data2 = (char*)malloc(1);
	CHECK(data1);
	CHECK(data2);

	gbm_bo_set_user_data(bo1, data1, destroy_data1);
	gbm_bo_set_user_data(bo2, data2, destroy_data2);

	CHECK((char*)gbm_bo_get_user_data(bo1) == data1);
	CHECK((char*)gbm_bo_get_user_data(bo2) == data2);

	gbm_bo_destroy(bo1);
	CHECK(been_there1 == 1);

	gbm_bo_set_user_data(bo2, NULL, NULL);
	gbm_bo_destroy(bo2);
	CHECK(been_there2 == 0);

	free(data1);
	free(data2);

	return 1;
}

/*
 * Tests destruction.
 */
static int test_destroy()
{
	gbm_device_destroy(gbm);
	close(fd);

	return 1;
}

/*
 * Tests prime export.
 */
static int test_export()
{
	struct gbm_bo *bo;
	int prime_fd;

	bo = gbm_bo_create(gbm, 1024, 1024, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
	CHECK(check_bo(bo));

	prime_fd = gbm_bo_get_fd(bo);
	CHECK(prime_fd > 0);
	close(prime_fd);

	gbm_bo_destroy(bo);

	return 1;
}

/*
 * Tests prime import using VGEM sharing buffer.
 */
static int test_import_vgem()
{
	struct gbm_import_fd_data fd_data;
	int vgem_fd = drm_open_vgem();
	struct drm_prime_handle prime_handle;
	struct gbm_bo *bo;
	const int width = 123;
	const int height = 456;
	const int bytes_per_pixel = 4;
	const int size = width * height * bytes_per_pixel;

	if (vgem_fd <= 0)
		return 1;

	CHECK(create_vgem_bo(vgem_fd, size, &prime_handle.handle) == 0);
	prime_handle.flags = DRM_CLOEXEC;
	CHECK(drmIoctl(vgem_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_handle) == 0);

	fd_data.fd = prime_handle.fd;
	fd_data.width = width;
	fd_data.height = height;
	fd_data.stride = width * bytes_per_pixel;
	fd_data.format = GBM_FORMAT_XRGB8888;

	bo = gbm_bo_import(gbm, GBM_BO_IMPORT_FD, &fd_data, GBM_BO_USE_RENDERING);
	CHECK(check_bo(bo));
	gbm_bo_destroy(bo);
	close(prime_handle.fd);

	close(vgem_fd);

	return 1;
}

/*
 * Tests prime import using dma-buf API.
 */
static int test_import_dmabuf()
{
	struct gbm_import_fd_data fd_data;
	struct gbm_bo *bo1, *bo2;
	const int width = 123;
	const int height = 456;
	int prime_fd;

	bo1 = gbm_bo_create(gbm, width, height, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
	CHECK(check_bo(bo1));

	prime_fd = gbm_bo_get_fd(bo1);
	CHECK(prime_fd >= 0);

	fd_data.fd = prime_fd;
	fd_data.width = width;
	fd_data.height = height;
	fd_data.stride = gbm_bo_get_stride(bo1);
	fd_data.format = GBM_FORMAT_XRGB8888;

	gbm_bo_destroy(bo1);

	bo2 = gbm_bo_import(gbm, GBM_BO_IMPORT_FD, &fd_data, GBM_BO_USE_RENDERING);
	CHECK(check_bo(bo2));
	CHECK(fd_data.width == gbm_bo_get_width(bo2));
	CHECK(fd_data.height == gbm_bo_get_height(bo2));
	CHECK(fd_data.stride == gbm_bo_get_stride(bo2));

	gbm_bo_destroy(bo2);
	close(prime_fd);

	return 1;
}


/*
 * Tests GBM_BO_IMPORT_FD_PLANAR entry point.
 */
static int test_import_planar()
{
	struct gbm_import_fd_planar_data fd_data;
	struct gbm_bo *bo1, *bo2;
	const int width = 567;
	const int height = 891;
	size_t num_planes, p;
	int i;

	for (i = 0; i < ARRAY_SIZE(format_list); i++) {
		uint32_t format = format_list[i];
		if (gbm_device_is_format_supported(gbm, format, GBM_BO_USE_RENDERING)) {
			bo1 = gbm_bo_create(gbm, width, height, format, GBM_BO_USE_RENDERING);
			CHECK(check_bo(bo1));

			num_planes = gbm_bo_get_num_planes(bo1);
			for (p = 0; p < num_planes; p++) {
				fd_data.fds[p] = gbm_bo_get_plane_fd(bo1, p);
				CHECK(fd_data.fds[p] >= 0);

				fd_data.strides[p] = gbm_bo_get_plane_stride(bo1, p);
				fd_data.offsets[p] = gbm_bo_get_plane_offset(bo1, p);
				fd_data.format_modifiers[p] =
					gbm_bo_get_plane_format_modifier(bo1, p);
			}

			fd_data.width = width;
			fd_data.height = height;
			fd_data.format = format;

			gbm_bo_destroy(bo1);

			bo2 = gbm_bo_import(gbm, GBM_BO_IMPORT_FD_PLANAR, &fd_data,
					    GBM_BO_USE_RENDERING);

			CHECK(check_bo(bo2));
			CHECK(fd_data.width == gbm_bo_get_width(bo2));
			CHECK(fd_data.height == gbm_bo_get_height(bo2));

			for (p = 0; p < num_planes; p++) {
				CHECK(fd_data.strides[p] == gbm_bo_get_plane_stride(bo2, p));
				CHECK(fd_data.offsets[p] == gbm_bo_get_plane_offset(bo2, p));
				CHECK(fd_data.format_modifiers[p] ==
				      gbm_bo_get_plane_format_modifier(bo2, p));
			}

			gbm_bo_destroy(bo2);

			for (p = 0; p < num_planes; p++)
				close(fd_data.fds[p]);
		}
	}

	return 1;
}

static int test_gem_map()
{
	uint32_t *pixel, pixel_size;
	struct gbm_bo *bo;
	void *map_data, *addr;

	uint32_t stride = 0;
	const int width = 666;
	const int height = 777;

	addr = map_data = NULL;

	bo = gbm_bo_create(gbm, width, height, GBM_FORMAT_ARGB8888,
			   GBM_BO_USE_SW_READ_RARELY | GBM_BO_USE_SW_WRITE_RARELY);
	CHECK(check_bo(bo));

	addr = gbm_bo_map(bo, 0, 0, width, height, GBM_BO_TRANSFER_READ_WRITE, &stride,
			  &map_data, 0);

	CHECK(addr != MAP_FAILED);
	CHECK(map_data);
	CHECK(stride > 0);

	pixel = (uint32_t *)addr;
	pixel_size = sizeof(*pixel);

	pixel[(height / 2) * (stride / pixel_size) + width / 2] = 0xABBAABBA;
	gbm_bo_unmap(bo, map_data);

	/* Re-map and verify written previously data. */
	stride = 0;
	addr = map_data = NULL;

	addr = gbm_bo_map(bo, 0, 0, width, height, GBM_BO_TRANSFER_READ_WRITE, &stride,
			  &map_data, 0);

	CHECK(addr != MAP_FAILED);
	CHECK(map_data);
	CHECK(stride > 0);

	pixel = (uint32_t *)addr;
	CHECK(pixel[(height / 2) * (stride / pixel_size) + width / 2] == 0xABBAABBA);

	gbm_bo_unmap(bo, map_data);
	gbm_bo_destroy(bo);

	return 1;
}

static int test_dmabuf_map()
{
	uint32_t *pixel;
	struct gbm_bo *bo;
	void *addr, *map_data;
	const int width = 666;
	const int height = 777;
	int x, y, ret, prime_fd;
	struct dma_buf_sync sync_end = { 0 };
	struct dma_buf_sync sync_start = { 0 };
	uint32_t pixel_size, stride, stride_pixels, length;

	bo = gbm_bo_create(gbm, width, height, GBM_FORMAT_ARGB8888, GBM_BO_USE_LINEAR);
	CHECK(check_bo(bo));

	prime_fd = gbm_bo_get_fd(bo);
	CHECK(prime_fd > 0);

	stride = gbm_bo_get_stride(bo);
	length = gbm_bo_get_plane_size(bo, 0);
	CHECK(stride > 0);
	CHECK(length > 0);

	addr = mmap(NULL, length, (PROT_READ | PROT_WRITE), MAP_SHARED, prime_fd, 0);
	CHECK(addr != MAP_FAILED);

	pixel = (uint32_t *)addr;
	pixel_size = sizeof(*pixel);
	stride_pixels = stride / pixel_size;

	sync_start.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_WRITE;
	ret = HANDLE_EINTR(ioctl(prime_fd, DMA_BUF_IOCTL_SYNC, &sync_start));
	CHECK(ret == 0);

	for (y = 0; y < height; ++y)
		for (x = 0; x < width; ++x)
			pixel[y * stride_pixels + x] = ((y << 16) | x);

	sync_end.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_WRITE;
	ret = HANDLE_EINTR(ioctl(prime_fd, DMA_BUF_IOCTL_SYNC, &sync_end));
	CHECK(ret == 0);

	ret = munmap(addr, length);
	CHECK(ret == 0);

	ret = close(prime_fd);
	CHECK(ret == 0);

	prime_fd = gbm_bo_get_fd(bo);
	CHECK(prime_fd > 0);

	addr = mmap(NULL, length, (PROT_READ | PROT_WRITE), MAP_SHARED, prime_fd, 0);
	CHECK(addr != MAP_FAILED);

	pixel = (uint32_t *)addr;

	memset(&sync_start, 0, sizeof(sync_start));
	memset(&sync_end, 0, sizeof(sync_end));

	sync_start.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ;
	ret = HANDLE_EINTR(ioctl(prime_fd, DMA_BUF_IOCTL_SYNC, &sync_start));
	CHECK(ret == 0);

	for (y = 0; y < height; ++y)
		for (x = 0; x < width; ++x)
			CHECK(pixel[y * stride_pixels + x] == ((y << 16) | x));

	sync_end.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
	ret = HANDLE_EINTR(ioctl(prime_fd, DMA_BUF_IOCTL_SYNC, &sync_end));
	CHECK(ret == 0);

	ret = munmap(addr, length);
	CHECK(ret == 0);

	ret = close(prime_fd);
	CHECK(ret == 0);

	addr = gbm_bo_map(bo, 0, 0, width, height, GBM_BO_TRANSFER_READ, &stride,
			  &map_data, 0);

	CHECK(addr != MAP_FAILED);
	CHECK(map_data);
	CHECK(stride > 0);

	pixel = (uint32_t *)addr;

	for (y = 0; y < height; ++y)
		for (x = 0; x < width; ++x)
			CHECK(pixel[y * stride_pixels + x] == ((y << 16) | x));

	gbm_bo_unmap(bo, map_data);
	gbm_bo_destroy(bo);

	return 1;
}

static int test_gem_map_tiling(enum gbm_bo_flags buffer_create_flag)
{
	uint32_t *pixel, pixel_size;
	struct gbm_bo *bo;
	void *map_data, *addr;

	uint32_t stride = 0;
	uint32_t stride_pixels = 0;
	const int width = 666;
	const int height = 777;
	int x, y;

	addr = map_data = NULL;

	bo = gbm_bo_create(gbm, width, height, GBM_FORMAT_ARGB8888, buffer_create_flag);
	CHECK(check_bo(bo));

	addr = gbm_bo_map(bo, 0, 0, width, height, GBM_BO_TRANSFER_WRITE, &stride,
			  &map_data, 0);

	CHECK(addr != MAP_FAILED);
	CHECK(map_data);
	CHECK(stride > 0);

	pixel = (uint32_t *)addr;
	pixel_size = sizeof(*pixel);
	stride_pixels = stride / pixel_size;

	for (y = 0; y < height; ++y)
		for (x = 0; x < width; ++x)
			pixel[y * stride_pixels + x] = ((y << 16) | x);

	gbm_bo_unmap(bo, map_data);

	/* Re-map and verify written previously data. */
	stride = 0;
	addr = map_data = NULL;

	addr = gbm_bo_map(bo, 0, 0, width, height, GBM_BO_TRANSFER_READ, &stride,
			  &map_data, 0);

	CHECK(addr != MAP_FAILED);
	CHECK(map_data);
	CHECK(stride > 0);

	pixel = (uint32_t *)addr;
	pixel_size = sizeof(*pixel);
	stride_pixels = stride / pixel_size;

	for (y = 0; y < height; ++y)
		for (x = 0; x < width; ++x)
			CHECK(pixel[y * stride_pixels + x] == ((y << 16) | x));

	gbm_bo_unmap(bo, map_data);
	gbm_bo_destroy(bo);

	return 1;
}


static int test_gem_map_format(int format_index,
			       enum gbm_bo_flags buffer_create_flag)
{
	uint8_t *pixel;
	struct gbm_bo *bo;
	void *map_data, *addr;
	uint32_t x, y, p, w, h, b, planes, bytes_per_pixel, idx;
	uint32_t stride = 0;
	const int width = 333;
	const int height = 444;
	const uint32_t pixel_format = format_info_list[format_index].pixel_format;

	addr = map_data = NULL;
	if (!gbm_device_is_format_supported(gbm, pixel_format, buffer_create_flag))
		return 1;

	bo = gbm_bo_create(gbm, width, height, pixel_format, buffer_create_flag);
	CHECK(check_bo(bo));
	planes = gbm_bo_get_num_planes(bo);
	CHECK(planes == format_info_list[format_index].num_planes);

	for (p = 0; p < planes; ++p) {
		w = width / format_info_list[format_index].planes[p].subsample_rate;
		h = height / format_info_list[format_index].planes[p].subsample_rate;
		addr = gbm_bo_map(bo, 0, 0, w, h, GBM_BO_TRANSFER_WRITE, &stride,
				  &map_data, p);

		CHECK(addr != MAP_FAILED);
		CHECK(map_data);
		CHECK(stride > 0);

		pixel = (uint8_t *)addr;
		bytes_per_pixel = format_info_list[format_index].planes[p].bits_per_pixel / 8;
		for (y = 0; y < h; ++y) {
			for (x = 0; x < w; ++x) {
				idx = y * stride + x * bytes_per_pixel;
				for (b = 0; b < bytes_per_pixel; ++b)
					pixel[idx + b] = y ^ x ^ b;
			}
		}
		gbm_bo_unmap(bo, map_data);
		stride = 0;
		addr = map_data = NULL;
	}

	/* Re-map and verify written previously data. */
	for (p = 0; p < planes; ++p) {
		w = width / format_info_list[format_index].planes[p].subsample_rate;
		h = height / format_info_list[format_index].planes[p].subsample_rate;
		addr = gbm_bo_map(bo, 0, 0, w, h, GBM_BO_TRANSFER_READ, &stride,
				  &map_data, p);

		CHECK(addr != MAP_FAILED);
		CHECK(map_data);
		CHECK(stride > 0);

		pixel = (uint8_t *)addr;
		bytes_per_pixel = format_info_list[format_index].planes[p].bits_per_pixel / 8;
		for (y = 0; y < h; ++y) {
			for (x = 0; x < w; ++x) {
				idx = y * stride + x * bytes_per_pixel;
				for (b = 0; b < bytes_per_pixel; ++b)
					CHECK(pixel[idx + b] == (uint8_t)(y ^ x ^ b));
			}
		}
		gbm_bo_unmap(bo, map_data);
		stride = 0;
		addr = map_data = NULL;
	}

	gbm_bo_destroy(bo);
	return 1;
}


int main(int argc, char *argv[])
{
	int result, i, j;

	result = test_init();
	if (result == ENODISPLAY) {
		printf("[  PASSED  ] graphics_Gbm test no connected display found\n");
		return EXIT_SUCCESS;
	} else if (!result) {
		printf("[  FAILED  ] graphics_Gbm test initialization failed\n");
		return EXIT_FAILURE;
	}

	result &= test_reinit();
	result &= test_alloc_free();
	result &= test_alloc_free_sizes();
	result &= test_alloc_free_formats();
	result &= test_alloc_free_usage();
	result &= test_user_data();
	result &= test_export();
	result &= test_import_vgem();
	result &= test_import_dmabuf();
	result &= test_import_planar();
	result &= test_gem_map();

	// TODO(crbug.com/752669)
	if (strcmp(gbm_device_get_backend_name(gbm), "tegra")) {
		for (i = 0; i < ARRAY_SIZE(buffer_list); ++i) {
			result &= test_gem_map_tiling(buffer_list[i]);
			for (j = 0; j < ARRAY_SIZE(format_info_list); ++j)
				result &= test_gem_map_format(j, buffer_list[i]);
		}

		result &= test_dmabuf_map();
	}
	result &= test_destroy();

	if (!result) {
		printf("[  FAILED  ] graphics_Gbm test failed\n");
		return EXIT_FAILURE;
	} else {
		printf("[  PASSED  ] graphics_Gbm test success\n");
		return EXIT_SUCCESS;
	}
}
