/*
 * Private IPP definitions for CUPS.
 *
 * Copyright 2007-2014 by Apple Inc.
 * Copyright 1997-2006 by Easy Software Products.
 *
 * These coded instructions, statements, and computer programs are the
 * property of Apple Inc. and are protected by Federal copyright
 * law.  Distribution and use rights are outlined in the file "LICENSE.txt"
 * which should have been included with this file.  If this file is
 * missing or damaged, see the license at "http://www.cups.org/".
 *
 * This file is subject to the Apple OS-Developed Software exception.
 */

#ifndef _CUPS_IPP_PRIVATE_H_
#  define _CUPS_IPP_PRIVATE_H_

/*
 * Include necessary headers...
 */

#  include <cups/ipp.h>


/*
 * C++ magic...
 */

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */


/*
 * Constants...
 */

#  define IPP_BUF_SIZE	(IPP_MAX_LENGTH + 2)
					/* Size of buffer */


/*
 * Structures...
 */

typedef struct				/**** Attribute mapping data ****/
{
  int		multivalue;		/* Option has multiple values? */
  const char	*name;			/* Option/attribute name */
  ipp_tag_t	value_tag;		/* Value tag for this attribute */
  ipp_tag_t	group_tag;		/* Group tag for this attribute */
  ipp_tag_t	alt_group_tag;		/* Alternate group tag for this
					 * attribute */
  const ipp_op_t *operations;		/* Allowed operations for this attr */
} _ipp_option_t;


/*
 * Prototypes for private functions...
 */

#ifdef DEBUG
extern const char	*_ippCheckOptions(void);
#endif /* DEBUG */
extern _ipp_option_t	*_ippFindOption(const char *name);

/*
 * C++ magic...
 */

#  ifdef __cplusplus
}
#  endif /* __cplusplus */
#endif /* !_CUPS_IPP_H_ */
