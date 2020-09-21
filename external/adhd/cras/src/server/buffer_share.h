/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef BUFFER_SHARE_H_
#define BUFFER_SHARE_H_

#define INITIAL_ID_SIZE 3

struct id_offset {
	unsigned int used;
	unsigned int id;
	unsigned int offset;
	void *data;
};

struct buffer_share {
	unsigned int buf_sz;
	unsigned int id_sz;
	struct id_offset *wr_idx;
};

/*
 * Creates a buffer share object.  This object is used to manage the read or
 * write offsets of several users in one shared buffer.
 */
struct buffer_share *buffer_share_create(unsigned int buf_sz);

/* Destroys a buffer_share returned from buffer_share_create. */
void buffer_share_destroy(struct buffer_share *mix);

/* Adds an ID that shares the buffer. */
int buffer_share_add_id(struct buffer_share *mix, unsigned int id, void *data);

/* Removes an ID that shares the buffer. */
int buffer_share_rm_id(struct buffer_share *mix, unsigned int id);

/* Updates the offset of the given user into the shared buffer. */
int buffer_share_offset_update(struct buffer_share *mix, unsigned int id,
			       unsigned int frames);

/*
 * Updates the write point to the minimum offset from all users.
 * Returns the number of minimum number of frames written.
 */
unsigned int buffer_share_get_new_write_point(struct buffer_share *mix);

/*
 * The amount by which the user given by id is ahead of the current write
 * point.
 */
unsigned int buffer_share_id_offset(const struct buffer_share *mix,
				    unsigned int id);

/*
 * Gets the data pointer for given id.
 */
void *buffer_share_get_data(const struct buffer_share *mix,
			    unsigned int id);

#endif /* BUFFER_SHARE_H_ */
