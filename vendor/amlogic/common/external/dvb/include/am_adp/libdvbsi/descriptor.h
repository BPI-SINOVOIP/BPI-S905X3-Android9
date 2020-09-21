/*****************************************************************************
 * descriptor.h
 * (c)2001-2002 VideoLAN
 * $Id: descriptor.h 88 2004-02-24 14:31:18Z sam $
 *
 * Authors: Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *****************************************************************************/

/*!
 * \file <descriptor.h>
 * \author Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
 * \brief Common descriptor tools.
 *
 * Descriptor structure and its Manipulation tools.
 */

#ifndef _DVBPSI_DESCRIPTOR_H_
#define _DVBPSI_DESCRIPTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ANDROID
#include <unistd.h>
#endif
#include <stdbool.h>

#define DVBPSI_BCD2VALUE(b) ((((b)&0xf0)>>4)*10 + ((b)&0x0f))


/*****************************************************************************
 * dvbpsi_descriptor_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_descriptor_s
 * \brief Descriptor structure.
 *
 * This structure is used to store a descriptor.
 * (ISO/IEC 13818-1 section 2.6).
 */
/*!
 * \typedef struct dvbpsi_descriptor_s dvbpsi_descriptor_t
 * \brief dvbpsi_descriptor_t type definition.
 */
typedef struct dvbpsi_descriptor_s
{
  uint8_t                       i_tag;          /*!< descriptor_tag */
  uint8_t                       i_length;       /*!< descriptor_length */

  uint8_t *                     p_data;         /*!< content */

  struct dvbpsi_descriptor_s *  p_next;         /*!< next element of
                                                     the list */

  void *                        p_decoded;      /*!< decoded descriptor */

} dvbpsi_descriptor_t;


/*****************************************************************************
 * dvbpsi_NewDescriptor
 *****************************************************************************/
/*!
 * \fn dvbpsi_descriptor_t* dvbpsi_NewDescriptor(uint8_t i_tag,
                                                 uint8_t i_length,
                                                 uint8_t* p_data)
 * \brief Creation of a new dvbpsi_descriptor_t structure.
 * \param i_tag descriptor's tag
 * \param i_length descriptor's length
 * \param p_data descriptor's data
 * \return a pointer to the descriptor.
 */

#ifdef TABLE_AREA
dvbpsi_descriptor_t* dvbpsi_NewDescriptorEx(uint8_t i_tag, uint8_t i_length,
                                          uint8_t* p_data, void *user);
#define dvbpsi_NewDescriptor(_t_, _l_, _p_)\
dvbpsi_NewDescriptorEx((_t_), (_l_), (_p_), psi_decode_descriptor_userdata)
#else
dvbpsi_descriptor_t* dvbpsi_NewDescriptor(uint8_t i_tag, uint8_t i_length,
                                          uint8_t* p_data);
#endif

/*****************************************************************************
 * dvbpsi_DeleteDescriptors
 *****************************************************************************/
/*!
 * \fn void dvbpsi_DeleteDescriptors(dvbpsi_descriptor_t* p_descriptor)
 * \brief Destruction of a dvbpsi_descriptor_t structure.
 * \param p_descriptor pointer to the first descriptor structure
 * \return nothing.
 */
void dvbpsi_DeleteDescriptors(dvbpsi_descriptor_t* p_descriptor);

/*****************************************************************************
 * dvbpsi_AddDescriptor
 *****************************************************************************/
/*!
 * \fn  dvbpsi_descriptor_t *dvbpsi_AddDescriptor(dvbpsi_descriptor_t *p_list,
                                          dvbpsi_descriptor_t *p_descriptor);
 * \brief Add a descriptor to the end of descriptor list.
 * \param p_list the first descriptor in the descriptor list.
 * \param p_descriptor the descriptor to add to the list
 * \return a pointer to the first element in the descriptor list.
 */
dvbpsi_descriptor_t *dvbpsi_AddDescriptor(dvbpsi_descriptor_t *p_list,
                                          dvbpsi_descriptor_t *p_descriptor);

/*****************************************************************************
 * dvbpsi_CanDecodeAsDescriptor
 *****************************************************************************/
/*!
 * \fn bool dvbpsi_CanDecodeAsDescriptor(dvbpsi_descriptor_t *p_descriptor, const uint8_t i_tag);
 * \brief Checks if descriptor tag matches.
 * \param p_descriptor pointer to descriptor allocated with @see dvbpsi_NewDescriptor
 * \param i_tag descriptor tag to evaluate against
 * \return true if descriptor can be decoded, false if not.
 */
bool dvbpsi_CanDecodeAsDescriptor(dvbpsi_descriptor_t *p_descriptor, const uint8_t i_tag);

/*****************************************************************************
 * dvbpsi_IsDescriptorDecoded
 *****************************************************************************/
/*!
 * \fn bool dvbpsi_IsDescriptorDecoded(dvbpsi_descriptor_t *p_descriptor);
 * \brief Checks if descriptor was already decoded.
 * \param p_descriptor pointer to descriptor allocated with @see dvbpsi_NewDescriptor
 * \return true if descriptor can be decoded, false if already decoded.
 */
bool dvbpsi_IsDescriptorDecoded(dvbpsi_descriptor_t *p_descriptor);


/*****************************************************************************
 * dvbpsi_DuplicateDecodedDescriptor
 *****************************************************************************/
/*!
 * \fn void *dvbpsi_DuplicateDecodedDescriptor(void *p_decoded, ssize_t i_size);
 * \brief Duplicate a decoded descriptor. The caller is responsible for releasing the associated memory.
 * \param p_decoded pointer to decoded descriptor obtained with dvbpsi_Decode* function
 * \param i_size the sizeof decoded descriptor
 * \return pointer to duplicated descriptor, NULL on error.
 */
void *dvbpsi_DuplicateDecodedDescriptor(void *p_decoded, ssize_t i_size);

typedef void (*DVBpsi_Decode_Descriptor) (dvbpsi_descriptor_t *descr, void *user);

/*****************************************************************************
 * dvbpsi_Set_DecodeDescriptor_Callback
 *****************************************************************************/
/*!
 * \fn void dvbpsi_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb)
 * \brief set decode des callback.
 * \param cb pointer to DVBpsi_Decode_Descriptor type fun
 * \return 0:success, -1:error.
 */
int dvbpsi_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb);

#define DEF_SET_DECODE_DESCRIPTOR_CALLBACK(_table_)\
static void *psi_decode_descriptor_userdata = NULL;\
int dvbpsi_##_table_##_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user)\
{\
    dvbpsi_Set_DecodeDescriptor_Callback(cb);\
	psi_decode_descriptor_userdata = user;\
	/*AM_DEBUG(1,"set ##_table_## decode descriptor cb/user\n");*/\
	return 0;\
}

#define SET_DECODE_DESCRIPTOR_CALLBACK(_table_, _cb_, _user_)\
dvbpsi_##_table_##_Set_DecodeDescriptor_Callback(_cb_, (void*)(long)(_user_));

#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of descriptor.h"
#endif

