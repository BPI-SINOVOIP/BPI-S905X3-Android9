/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "dumper.h"

void dumpf(struct dumper *dumper, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	dumper->vprintf(dumper, format, ap);
	va_end(ap);
}

/* dumper which outputs to syslog */

struct syslog_data {
	int priority;
	struct dumper *mem_dumper;
};

static void syslog_vprintf(struct dumper *dumper, const char *fmt, va_list ap)
{
	char *buf;
	int size, i;
	struct syslog_data *data = (struct syslog_data *)dumper->data;
	struct dumper *mem_dumper = data->mem_dumper;

	/* We cannot use syslog() directly each time we are called,
	 * because syslog() will always append a newline to the
	 * output, so the user will not be able to construct a line
	 * incrementally using multiple calls. What we do here is to
	 * collect the output in a buffer until a newline is given by
	 * the user. */

	mem_dumper->vprintf(mem_dumper, fmt, ap);
again:
	mem_dumper_get(mem_dumper, &buf, &size);
	for (i = 0; i < size; i++) {
		if (buf[i] == '\n') {
			syslog(data->priority, "%.*s", i + 1, buf);
			mem_dumper_consume(mem_dumper, i + 1);
			goto again;
		}
	}
}

struct dumper *syslog_dumper_create(int priority)
{
	struct dumper *dumper = calloc(1, sizeof(struct dumper));
	struct syslog_data *data = calloc(1, sizeof(struct syslog_data));
	data->priority = priority;
	data->mem_dumper = mem_dumper_create();
	dumper->data = data;
	dumper->vprintf = &syslog_vprintf;
	return dumper;
}

void syslog_dumper_free(struct dumper *dumper)
{
	mem_dumper_free(((struct syslog_data *)dumper->data)->mem_dumper);
	free(dumper->data);
	free(dumper);
}

/* dumper which outputs to a memory buffer */

struct mem_data {
	char *buf;
	int size;
	int capacity;
};

static void mem_vprintf(struct dumper *dumper, const char *format, va_list ap)
{
	int n;
	char *tmp;
	struct mem_data *data = (struct mem_data *)dumper->data;

	while (1) {
		/* try to use the remaining space */
		int remaining = data->capacity - data->size;
		n = vsnprintf(data->buf + data->size, remaining, format, ap);

		/* enough space? */
		if (n > -1 && n < remaining) {
			data->size += n;
			return;
		}

		/* allocate more space and try again */
		tmp = realloc(data->buf, data->capacity * 2);
		if (tmp == NULL)
			return;
		data->buf = tmp;
		data->capacity *= 2;
	}
}

struct dumper *mem_dumper_create()
{
	struct dumper *dumper = calloc(1, sizeof(struct dumper));
	struct mem_data *data = calloc(1, sizeof(struct mem_data));
	if (!dumper || !data)
		return NULL;
	data->size = 0;
	data->capacity = 80;
	data->buf = malloc(data->capacity);
	if (!data->buf) {
		free(data);
		return NULL;
	}
	data->buf[0] = '\0';
	dumper->data = data;
	dumper->vprintf = &mem_vprintf;
	return dumper;
}

void mem_dumper_free(struct dumper *dumper)
{
	struct mem_data *data = (struct mem_data *)dumper->data;
	free(data->buf);
	free(data);
	free(dumper);
}

void mem_dumper_clear(struct dumper *dumper)
{
	struct mem_data *data = (struct mem_data *)dumper->data;
	data->buf[0] = '\0';
	data->size = 0;
}

void mem_dumper_consume(struct dumper *dumper, int n)
{
	struct mem_data *data = (struct mem_data *)dumper->data;
	if (n > data->size)
		n = data->size;
	memmove(data->buf, data->buf + n, data->size - n + 1);
	data->size -= n;
}

void mem_dumper_get(struct dumper *dumper, char **buf, int *size)
{
	struct mem_data *data = (struct mem_data *)dumper->data;
	*buf = data->buf;
	*size = data->size;
}
