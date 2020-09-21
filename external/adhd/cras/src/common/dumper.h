/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_DUMPER_H_
#define CRAS_DUMPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* dumper is an interface to output some human-readable information */
struct dumper {
	void (*vprintf)(struct dumper *dumper, const char *format, va_list ap);
	void *data;  /* private to each dumper */
};

/* a convenience function outputs to a dumper */
void dumpf(struct dumper *dumper, const char *format, ...);

/*
 * a dumper outputs to syslog with the given priority
 */
struct dumper *syslog_dumper_create(int priority);
void syslog_dumper_free(struct dumper *dumper);

/*
 * a dumper saves the output in a memory buffer
 */
struct dumper *mem_dumper_create();
void mem_dumper_free(struct dumper *dumper);

/* get the memory buffer of the output */
void mem_dumper_get(struct dumper *dumper, char **buf, int *size);

/* clear the memory buffer */
void mem_dumper_clear(struct dumper *dumper);

/* delete the first n characters in the memory buffer */
void mem_dumper_consume(struct dumper *dumper, int n);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CRAS_DUMPER_H_ */
