/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/

#ifndef CRYPT_GPL_ALGORITHM 
/****************************************************************************
    Module Name:
    HMAC

    Abstract:
    FIPS 198: The Keyed-Hash Message Authentication Code (HMAC)
    
    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
    Eddy        2008/11/24      Create HMAC-SHA1, HMAC-SHA256
***************************************************************************/
#endif /* CRYPT_GPL_ALGORITHM */

#include "crypt_hmac.h"

#ifdef CRYPT_GPL_ALGORITHM

#if defined(__cplusplus)
extern "C"
{
#endif

/* initialise the HMAC context to zero */
void hmac_sha_begin(hmac_ctx cx[1])
{
    memset(cx, 0, sizeof(hmac_ctx));
}

/* input the HMAC key (can be called multiple times)    */
int hmac_sha_key(const unsigned char key[], unsigned int key_len, hmac_ctx cx[1])
{
    if(cx->klen == HMAC_IN_DATA)                /* error if further key input   */
        return HMAC_BAD_MODE;                   /* is attempted in data mode    */

    if(cx->klen + key_len > HASH_INPUT_SIZE)    /* if the key has to be hashed  */
    {
        if(cx->klen <= HASH_INPUT_SIZE)         /* if the hash has not yet been */
        {                                       /* started, initialise it and   */
            sha_begin(cx->ctx);                /* hash stored key characters   */
            sha_hash(cx->key, cx->klen, cx->ctx);
        }

        sha_hash(key, key_len, cx->ctx);       /* hash long key data into hash */
    }
    else                                        /* otherwise store key data     */
        memcpy(cx->key + cx->klen, key, key_len);

    cx->klen += key_len;                        /* update the key length count  */
    return HMAC_OK;
}

/* input the HMAC data (can be called multiple times) - */
/* note that this call terminates the key input phase   */
void hmac_sha_data(const unsigned char data[], unsigned int data_len, hmac_ctx cx[1])
{   unsigned int i;

    if(cx->klen != HMAC_IN_DATA)                /* if not yet in data phase */
    {
        if(cx->klen > HASH_INPUT_SIZE)          /* if key is being hashed   */
        {                                       /* complete the hash and    */
            sha_end(cx->key, cx->ctx);         /* store the result as the  */
            cx->klen = HASH_OUTPUT_SIZE;        /* key and set new length   */
        }

        /* pad the key if necessary */
        memset(cx->key + cx->klen, 0, HASH_INPUT_SIZE - cx->klen);

        /* xor ipad into key value  */
        for(i = 0; i < (HASH_INPUT_SIZE >> 2); ++i)
            ((unsigned int*)cx->key)[i] ^= 0x36363636;

        /* and start hash operation */
        sha_begin(cx->ctx);
        sha_hash(cx->key, HASH_INPUT_SIZE, cx->ctx);

        /* mark as now in data mode */
        cx->klen = HMAC_IN_DATA;
    }

    /* hash the data (if any)       */
    if(data_len)
        sha_hash(data, data_len, cx->ctx);
}

/* compute and output the MAC value */
void hmac_sha_end(unsigned char mac[], unsigned int mac_len, hmac_ctx cx[1])
{   unsigned char dig[HASH_OUTPUT_SIZE];
    unsigned int i;

    /* if no data has been entered perform a null data phase        */
    if(cx->klen != HMAC_IN_DATA)
        hmac_sha_data((const unsigned char*)0, 0, cx);

    sha_end(dig, cx->ctx);         /* complete the inner hash      */

    /* set outer key value using opad and removing ipad */
    for(i = 0; i < (HASH_INPUT_SIZE >> 2); ++i)
        ((unsigned int*)cx->key)[i] ^= 0x36363636 ^ 0x5c5c5c5c;

    /* perform the outer hash operation */
    sha_begin(cx->ctx);
    sha_hash(cx->key, HASH_INPUT_SIZE, cx->ctx);
    sha_hash(dig, HASH_OUTPUT_SIZE, cx->ctx);
    sha_end(dig, cx->ctx);

    /* output the hash value            */
    for(i = 0; i < mac_len; ++i)
        mac[i] = dig[i];
}

/* 'do it all in one go' subroutine     */
void hmac_sha(const unsigned char key[], unsigned int key_len,
          const unsigned char data[], unsigned int data_len,
          unsigned char mac[], unsigned int mac_len)
{   hmac_ctx    cx[1];

    hmac_sha_begin(cx);
    hmac_sha_key(key, key_len, cx);
    hmac_sha_data(data, data_len, cx);
    hmac_sha_end(mac, mac_len, cx);
}

#if defined(__cplusplus)
}
#endif

#else /* CRYPT_GPL_ALGORITHM */

#ifdef HMAC_SHA1_SUPPORT
/*
========================================================================
Routine Description:
    HMAC using SHA1 hash function

Arguments:
    key             Secret key
    key_len         The length of the key in bytes   
    message         Message context
    message_len     The length of message in bytes
    macLen          Request the length of message authentication code

Return Value:
    mac             Message authentication code

Note:
    None
========================================================================
*/
VOID RT_HMAC_SHA1 (
    IN  const UINT8 Key[], 
    IN  UINT KeyLen, 
    IN  const UINT8 Message[], 
    IN  UINT MessageLen, 
    OUT UINT8 MAC[],
    IN  UINT MACLen)    
{
    SHA1_CTX_STRUC sha_ctx1;
    SHA1_CTX_STRUC sha_ctx2;
    UINT8 K0[SHA1_BLOCK_SIZE];
    UINT8 Digest[SHA1_DIGEST_SIZE];    
    UINT index;

    NdisZeroMemory(&sha_ctx1, sizeof(SHA1_CTX_STRUC));
    NdisZeroMemory(&sha_ctx2, sizeof(SHA1_CTX_STRUC));    
    /*
     * If the length of K = B(Block size): K0 = K.
     * If the length of K > B: hash K to obtain an L byte string, 
     * then append (B-L) zeros to create a B-byte string K0 (i.e., K0 = H(K) || 00...00).
     * If the length of K < B: append zeros to the end of K to create a B-byte string K0
     */
    NdisZeroMemory(K0, SHA1_BLOCK_SIZE);
    if (KeyLen <= SHA1_BLOCK_SIZE)
        NdisMoveMemory(K0, Key, KeyLen);
    else
        RT_SHA1(Key, KeyLen, K0);
    /* End of if */

    /* Exclusive-Or K0 with ipad */
    /* ipad: Inner pad; the byte x��36�� repeated B times. */
    for (index = 0; index < SHA1_BLOCK_SIZE; index++)
        K0[index] ^= 0x36;
        /* End of for */

    RT_SHA1_Init(&sha_ctx1);
    /* H(K0^ipad) */
    RT_SHA1_Append(&sha_ctx1, K0, sizeof(K0));
    /* H((K0^ipad)||text) */
    RT_SHA1_Append(&sha_ctx1, Message, MessageLen);
    RT_SHA1_End(&sha_ctx1, Digest);

    /* Exclusive-Or K0 with opad and remove ipad */
    /* opad: Outer pad; the byte x��5c�� repeated B times. */
    for (index = 0; index < SHA1_BLOCK_SIZE; index++)
        K0[index] ^= 0x36^0x5c;
        /* End of for */

    RT_SHA1_Init(&sha_ctx2);
    /* H(K0^opad) */
    RT_SHA1_Append(&sha_ctx2, K0, sizeof(K0));
    /* H( (K0^opad) || H((K0^ipad)||text) ) */
    RT_SHA1_Append(&sha_ctx2, Digest, SHA1_DIGEST_SIZE);
    RT_SHA1_End(&sha_ctx2, Digest);

    if (MACLen > SHA1_DIGEST_SIZE)
        NdisMoveMemory(MAC, Digest, SHA1_DIGEST_SIZE);
    else
        NdisMoveMemory(MAC, Digest, MACLen);    
} /* End of RT_HMAC_SHA1 */
#endif /* HMAC_SHA1_SUPPORT */


#ifdef HMAC_SHA256_SUPPORT
/*
========================================================================
Routine Description:
    HMAC using SHA256 hash function

Arguments:
    key             Secret key
    key_len         The length of the key in bytes   
    message         Message context
    message_len     The length of message in bytes
    macLen          Request the length of message authentication code

Return Value:
    mac             Message authentication code

Note:
    None
========================================================================
*/
VOID RT_HMAC_SHA256 (
    IN  const UINT8 Key[], 
    IN  UINT KeyLen, 
    IN  const UINT8 Message[], 
    IN  UINT MessageLen, 
    OUT UINT8 MAC[],
    IN  UINT MACLen)
{
    SHA256_CTX_STRUC sha_ctx1;
    SHA256_CTX_STRUC sha_ctx2;
    UINT8 K0[SHA256_BLOCK_SIZE];
    UINT8 Digest[SHA256_DIGEST_SIZE];
    UINT index;

    NdisZeroMemory(&sha_ctx1, sizeof(SHA256_CTX_STRUC));
    NdisZeroMemory(&sha_ctx2, sizeof(SHA256_CTX_STRUC));
    /*
     * If the length of K = B(Block size): K0 = K.
     * If the length of K > B: hash K to obtain an L byte string, 
     * then append (B-L) zeros to create a B-byte string K0 (i.e., K0 = H(K) || 00...00).
     * If the length of K < B: append zeros to the end of K to create a B-byte string K0
     */
    NdisZeroMemory(K0, SHA256_BLOCK_SIZE);
    if (KeyLen <= SHA256_BLOCK_SIZE) {
        NdisMoveMemory(K0, Key, KeyLen);
    } else {
        RT_SHA256(Key, KeyLen, K0);
    }

    /* Exclusive-Or K0 with ipad */
    /* ipad: Inner pad; the byte x��36�� repeated B times. */
    for (index = 0; index < SHA256_BLOCK_SIZE; index++)
        K0[index] ^= 0x36;
        /* End of for */
        
    RT_SHA256_Init(&sha_ctx1);
    /* H(K0^ipad) */
    RT_SHA256_Append(&sha_ctx1, K0, sizeof(K0));
    /* H((K0^ipad)||text) */
    RT_SHA256_Append(&sha_ctx1, Message, MessageLen);  
    RT_SHA256_End(&sha_ctx1, Digest);

    /* Exclusive-Or K0 with opad and remove ipad */
    /* opad: Outer pad; the byte x��5c�� repeated B times. */
    for (index = 0; index < SHA256_BLOCK_SIZE; index++)
        K0[index] ^= 0x36^0x5c;
        /* End of for */
        
    RT_SHA256_Init(&sha_ctx2);
    /* H(K0^opad) */
    RT_SHA256_Append(&sha_ctx2, K0, sizeof(K0));
    /* H( (K0^opad) || H((K0^ipad)||text) ) */
    RT_SHA256_Append(&sha_ctx2, Digest, SHA256_DIGEST_SIZE);
    RT_SHA256_End(&sha_ctx2, Digest);

    if (MACLen > SHA256_DIGEST_SIZE)
        NdisMoveMemory(MAC, Digest,SHA256_DIGEST_SIZE);
    else
        NdisMoveMemory(MAC, Digest, MACLen);
    
} /* End of RT_HMAC_SHA256 */
#endif /* HMAC_SHA256_SUPPORT */


#ifdef HMAC_MD5_SUPPORT
/*
========================================================================
Routine Description:
    HMAC using MD5 hash function

Arguments:
    key             Secret key
    key_len         The length of the key in bytes   
    message         Message context
    message_len     The length of message in bytes
    macLen          Request the length of message authentication code

Return Value:
    mac             Message authentication code

Note:
    None
========================================================================
*/
VOID RT_HMAC_MD5(
    IN  const UINT8 Key[], 
    IN  UINT KeyLen, 
    IN  const UINT8 Message[], 
    IN  UINT MessageLen, 
    OUT UINT8 MAC[],
    IN  UINT MACLen)    
{
    MD5_CTX_STRUC md5_ctx1;
    MD5_CTX_STRUC md5_ctx2;
    UINT8 K0[MD5_BLOCK_SIZE];
    UINT8 Digest[MD5_DIGEST_SIZE];    
    UINT index;

    NdisZeroMemory(&md5_ctx1, sizeof(MD5_CTX_STRUC));
    NdisZeroMemory(&md5_ctx2, sizeof(MD5_CTX_STRUC));
    /*
     * If the length of K = B(Block size): K0 = K.
     * If the length of K > B: hash K to obtain an L byte string, 
     * then append (B-L) zeros to create a B-byte string K0 (i.e., K0 = H(K) || 00...00).
     * If the length of K < B: append zeros to the end of K to create a B-byte string K0
     */
    NdisZeroMemory(K0, MD5_BLOCK_SIZE);
    if (KeyLen <= MD5_BLOCK_SIZE) {
        NdisMoveMemory(K0, Key, KeyLen);
    } else {
        RT_MD5(Key, KeyLen, K0);
    }

    /* Exclusive-Or K0 with ipad */
    /* ipad: Inner pad; the byte x��36�� repeated B times. */
    for (index = 0; index < MD5_BLOCK_SIZE; index++)
        K0[index] ^= 0x36;
        /* End of for */
        
    RT_MD5_Init(&md5_ctx1);
    /* H(K0^ipad) */
    RT_MD5_Append(&md5_ctx1, K0, sizeof(K0));
    /* H((K0^ipad)||text) */
    RT_MD5_Append(&md5_ctx1, Message, MessageLen);  
    RT_MD5_End(&md5_ctx1, Digest);

    /* Exclusive-Or K0 with opad and remove ipad */
    /* opad: Outer pad; the byte x��5c�� repeated B times. */
    for (index = 0; index < MD5_BLOCK_SIZE; index++)
        K0[index] ^= 0x36^0x5c;
        /* End of for */
        
    RT_MD5_Init(&md5_ctx2);
    /* H(K0^opad) */
    RT_MD5_Append(&md5_ctx2, K0, sizeof(K0));
    /* H( (K0^opad) || H((K0^ipad)||text) ) */
    RT_MD5_Append(&md5_ctx2, Digest, MD5_DIGEST_SIZE);
    RT_MD5_End(&md5_ctx2, Digest);

    if (MACLen > MD5_DIGEST_SIZE)
        NdisMoveMemory(MAC, Digest, MD5_DIGEST_SIZE);
    else
        NdisMoveMemory(MAC, Digest, MACLen);    
} /* End of RT_HMAC_SHA256 */
#endif /* HMAC_MD5_SUPPORT */

#endif /* CRYPT_GPL_ALGORITHM */

/* End of crypt_hmac.c */

