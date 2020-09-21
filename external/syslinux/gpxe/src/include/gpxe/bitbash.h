#ifndef _GPXE_BITBASH_H
#define _GPXE_BITBASH_H

/** @file
 *
 * Bit-bashing interfaces
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

struct bit_basher;

/** Bit-bashing operations */
struct bit_basher_operations {
	/**
	 * Set/clear output bit
	 *
	 * @v basher		Bit-bashing interface
	 * @v bit_id		Bit number
	 * @v data		Value to write
	 * 
	 * @c data will be 0 if a logic 0 should be written (i.e. the
	 * bit should be cleared), or -1UL if a logic 1 should be
	 * written (i.e. the bit should be set).  This is done so that
	 * the method may simply binary-AND @c data with the
	 * appropriate bit mask.
	 */
	void ( * write ) ( struct bit_basher *basher, unsigned int bit_id,
			   unsigned long data );
	/**
	 * Read input bit
	 *
	 * @v basher		Bit-bashing interface
	 * @v bit_id		Bit number
	 * @ret zero		Input is a logic 0
	 * @ret non-zero	Input is a logic 1
	 */
	int ( * read ) ( struct bit_basher *basher, unsigned int bit_id );
};

/** A bit-bashing interface */
struct bit_basher {
	/** Bit-bashing operations */
	struct bit_basher_operations *op;
};

extern void write_bit ( struct bit_basher *basher, unsigned int bit_id,
			unsigned long data );
extern int read_bit ( struct bit_basher *basher, unsigned int bit_id );

#endif /* _GPXE_BITBASH_H */
