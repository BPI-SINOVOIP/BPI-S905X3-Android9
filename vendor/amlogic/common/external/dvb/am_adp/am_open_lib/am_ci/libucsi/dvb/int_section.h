/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 * Copyright (C) 2005 Patrick Boettcher (pb@linuxtv.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#ifndef _UCSI_DVB_INT_SECTION_H
#define _UCSI_DVB_INT_SECTION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>

/**
 * dvb_int_section structure - IP/MAC notification section.
 */
struct dvb_int_section {
	struct section_ext head;

  EBIT2(uint32_t platform_id				:24;  ,
	uint32_t processing_order			: 8;  );
  EBIT2(uint16_t reserved2				: 4;  ,
	uint16_t platform_descriptors_length		:12;  );
	/* struct descriptor platform_descriptors[] */
	/* struct dvb_int_target target_loop[] */
} __ucsi_packed;

/**
 * An entry in the target_loop field of a dvb_int_section.
 */
struct dvb_int_target {
  EBIT2(uint16_t reserved3			: 4;  ,
	uint16_t target_descriptors_length	:12;  );
	/* struct descriptor target_descriptors[] */
	/* struct dvb_int_operational_loop operational_loop */
} __ucsi_packed;

/**
 * The operational_loop field in a dvb_int_target.
 */
struct dvb_int_operational_loop {
  EBIT2(uint16_t reserved4				: 4;  ,
	uint16_t operational_descriptors_length		:12;  );
	/* struct descriptor operational_descriptors[] */
} __ucsi_packed;

/**
 * Process a dvb_int_section.
 *
 * @param section Generic section_ext pointer.
 * @return dvb_int_section pointer, or NULL on error.
 */
extern struct dvb_int_section * dvb_int_section_codec(struct section_ext *section);

/**
 * Accessor for the action_type field of an INT.
 *
 * @param intp INT pointer.
 * @return The action_type.
 */
static inline uint8_t dvb_int_section_action_type(struct dvb_int_section *intp)
{
	return intp->head.table_id_ext >> 8;
}

/**
 * Accessor for the platform_id_hash field of an INT.
 *
 * @param intp INT pointer.
 * @return The platform_id_hash.
 */
static inline uint8_t dvb_int_section_platform_id_hash(struct dvb_int_section *intp)
{
	return intp->head.table_id_ext & 0xff;
}

/**
 * Iterator for platform_descriptors field in a dvb_int_section.
 *
 * @param intp dvb_int_section pointer.
 * @param pos Variable holding a pointer to the current descriptor.
 */
#define dvb_int_section_platform_descriptors_for_each(intp, pos) \
	for ((pos) = dvb_int_section_platform_descriptors_first(intp); \
	     (pos); \
	     (pos) = dvb_int_section_platform_descriptors_next(intp, pos))

/**
 * Iterator for the target_loop field in a dvb_int_section.
 *
 * @param intp dvb_int_section pointer.
 * @param pos Variable holding a pointer to the current dvb_int_target.
 */
#define dvb_int_section_target_loop_for_each(intp,pos) \
	for ((pos) = dvb_int_section_target_loop_first(intp); \
	     (pos); \
	     (pos) = dvb_int_section_target_loop_next(intp, pos))

/**
 * Iterator for the target_descriptors field in a dvb_int_target.
 *
 * @param target dvb_int_target pointer.
 * @param pos Variable holding a pointer to the current descriptor.
 */
#define dvb_int_target_target_descriptors_for_each(target, pos) \
	for ((pos) = dvb_int_target_target_descriptors_first(target); \
	     (pos); \
	     (pos) = dvb_int_target_target_descriptors_next(target, pos))

/**
 * Accessor for the operational_loop field of a dvb_int_target.
 *
 * @param target dvb_int_target pointer.
 * @return Pointer to a dvb_int_operational_loop.
 */
static inline struct dvb_int_operational_loop *
	dvb_int_target_operational_loop(struct dvb_int_target *target)
{
	return (struct dvb_int_operational_loop *)
		((uint8_t *) target + sizeof(struct dvb_int_target) + target->target_descriptors_length);
}

/**
 * Iterator for the operational_descriptors field in a dvb_int_operational_loop.
 *
 * @param oploop dvb_int_operational_loop pointer.
 * @param pos Variable holding a pointer to the current descriptor.
 */
#define dvb_int_operational_loop_operational_descriptors_for_each(oploop, pos) \
	for ((pos) = dvb_int_operational_loop_operational_descriptors_first(oploop); \
	     (pos); \
	     (pos) = dvb_int_operational_loop_operational_descriptors_next(oploop, pos))





/******************************** PRIVATE CODE ********************************/
static inline struct descriptor *
	dvb_int_section_platform_descriptors_first(struct dvb_int_section *in)
{
	if (in->platform_descriptors_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t *) in + sizeof(struct dvb_int_section));
}

static inline struct descriptor *
	dvb_int_section_platform_descriptors_next(struct dvb_int_section *in,
		struct descriptor* pos)
{
	return next_descriptor((uint8_t*) in + sizeof(struct dvb_int_section),
		in->platform_descriptors_length,
		pos);
}

static inline struct dvb_int_target *
	dvb_int_section_target_loop_first(struct dvb_int_section *in)
{
	if (sizeof(struct dvb_int_section) + in->platform_descriptors_length >= (uint32_t) section_ext_length((struct section_ext *) in))
		return NULL;

	return (struct dvb_int_target *)
		((uint8_t *) in + sizeof(struct dvb_int_section) + in->platform_descriptors_length);
}

static inline struct dvb_int_target *
	dvb_int_section_target_loop_next(struct dvb_int_section *in,
			struct dvb_int_target *pos)
{
	struct dvb_int_operational_loop *ol = dvb_int_target_operational_loop(pos);
	struct dvb_int_target *next =
		(struct dvb_int_target *) ( (uint8_t *) pos +
			sizeof(struct dvb_int_target) + pos->target_descriptors_length +
			sizeof(struct dvb_int_operational_loop) + ol->operational_descriptors_length);
	struct dvb_int_target *end =
		(struct dvb_int_target *) ((uint8_t *) in + section_ext_length((struct section_ext *) in) );

	if (next >= end)
		return 0;
	return next;
}

static inline struct descriptor *
	dvb_int_target_target_descriptors_first(struct dvb_int_target *tl)
{
	if (tl->target_descriptors_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t *) tl + sizeof(struct dvb_int_target));
}

static inline struct descriptor *
	dvb_int_target_target_descriptors_next(struct dvb_int_target *tl,
		struct descriptor* pos)
{
	return next_descriptor((uint8_t*) tl + sizeof(struct dvb_int_target),
		tl->target_descriptors_length,
		pos);
}

static inline struct descriptor *
	dvb_int_operational_loop_operational_descriptors_first(struct dvb_int_operational_loop *ol)
{
	if (ol->operational_descriptors_length == 0)
		return NULL;

	return (struct descriptor *)
		((uint8_t *) ol + sizeof(struct dvb_int_operational_loop));
}

static inline struct descriptor *
	dvb_int_operational_loop_operational_descriptors_next(struct dvb_int_operational_loop *ol,
		struct descriptor* pos)
{
	return next_descriptor((uint8_t*) ol + sizeof(struct dvb_int_operational_loop),
		ol->operational_descriptors_length,
		pos);
}

#ifdef __cplusplus
}
#endif

#endif
