/*
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License
 * Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
 * used only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission
 * from Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __AM_DSC_TYPE__H__
#define __AM_DSC_TYPE__H__
typedef enum {
	AM_DSC_KEY_TYPE_EVEN = 0,        /**< DVB-CSA even key */
	AM_DSC_KEY_TYPE_ODD = 1,         /**< DVB-CSA odd key */
	AM_DSC_KEY_TYPE_AES_EVEN = 2,    /**< AES even key */
	AM_DSC_KEY_TYPE_AES_ODD = 3,     /**< AES odd key */
	AM_DSC_KEY_TYPE_AES_IV_EVEN = 4, /**< AES-CBC iv even key */
	AM_DSC_KEY_TYPE_AES_IV_ODD = 5,  /**< AES-CBC iv odd key */
	AM_DSC_KEY_TYPE_DES_EVEN = 6,    /**< DES even key */
	AM_DSC_KEY_TYPE_DES_ODD = 7,     /**< DES odd key */
	AM_DSC_KEY_TYPE_SM4_EVEN = 8,	 /**< SM4 even key */
	AM_DSC_KEY_TYPE_SM4_ODD = 9,	 /**< SM4 odd key */
	AM_DSC_KEY_TYPE_SM4_EVEN_IV = 10,/**< SM4-CBC iv even key */
	AM_DSC_KEY_TYPE_SM4_ODD_IV = 11, /**< SM4-CBC iv odd key */
	AM_DSC_KEY_FROM_KL = (1<<7)      /**< Key from keyladder flag */
} AM_DSC_KeyType_t;
#endif
