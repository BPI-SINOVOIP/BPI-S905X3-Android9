#!/usr/bin/env python
#Python 2.7.6
from Crypto.Cipher import AES, DES3
import binascii
#============================================
#CONSTANTS
#============================================
SCK = binascii.unhexlify('77656c636f6d65746f6d797061727479')
MASK_KEY = binascii.unhexlify('F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF') # unused
EK2 = binascii.unhexlify('202122232425262728292A2B2C2D2E2F')
EK1 = binascii.unhexlify('10ffa00000023a9e61c5fdffff5f00f2')
NONCE = binascii.unhexlify('a0a1a2a3a4a5a6a7a8a9aaabacadaeaf')
CW8 = binascii.unhexlify('BCFBB26913BABE8B')
CW8_2 = binascii.unhexlify('68E1DA5B24AD861F')
CW16 = binascii.unhexlify('2020039f5f0bd2a9d4f9989b9ae30aa8')
MODULE_ID = '\xa5'

USE_FIXED_SEEDV = False
FIXED_SEEDV = binascii.unhexlify('d2d0b3541dd726d1398eb77cb123d25f')
VENDOR_ID = '\x2a\x42'

#============================================
#Utility functions
#============================================
#pretty prints a string of bytes with a given separating character
def h(x,s=" "):
    return s.join(["%02X" % ord(c) for c in x])
#performs xor on two strings of the same length
def XOR(a,b):
    if len(a) != len(b):
        print 'XOR received strings of unequal length'

    result = ''.join([chr(ord(a[i]) ^ ord(b[i])) for i in range(len(a))])
    return result
#the following are needed for padding a CW out to 16 bytes, or no padding at all.
def pad8(cw):
    return cw + '\x00'*8
def padNone(cw):
    return cw
#============================================
#Crypto Functions
#============================================
def AESEncrypt(key,data):
    return AES.new(key,1).encrypt(data)

def AESDecrypt(key,data):
    return AES.new(key,1).decrypt(data)

def DES3Encrypt(key,data):
    return DES3.new(key,1).encrypt(data)

def DES3Decrypt(key,data):
    return DES3.new(key,1).decrypt(data)

#============================================
#These functions pad the vendor ID or module ID depending on the algorithm
#============================================
#for AES, just a straght zero-pad
def padVendorIDAES(vid):
    return '\x00' * 14 + vid

#for TDES, the vendor ID must be placed in each half, and we want to somehow make each half different
def padVendorIDDES3(vid):
    return '\x01' + '\x00' * 5 + vid + '\x02' + '\x00' *5 + vid

def padModuleIDAES(mid):
    return '\x00' * 15 + mid
#for TDES, the vendor ID must be placed in each half, and we want to somehow make each half different
def padModuleIDDES3(mid):
    return '\x01' + '\x00' * 6 + mid + '\x02' + '\x00' *6 + mid
#============================================
#The following functions implement the actual operations
#============================================
#There are two types of key derivations: with and without module ID. Within those groups
#the flow is the the same, it's just the algos and the VID/ModuleID padding that change.
#this function performs the basic operations
def BasicKeyDerivation(algo, SCK, vendorID):
    SCKv = algo(SCK,vendorID)
    if USE_FIXED_SEEDV:
        Seedv = FIXED_SEEDV
    else:
        Seedv = algo(MASK_KEY,vendorID)
    K3 = XOR(Seedv, algo(SCKv,Seedv))
    return {'SCKv':SCKv, 'Seedv':Seedv, 'K3':K3}

def BasicKeyDerivationWithModuleID(algo, SCK, vendorID,moduleID):
    SCKv = algo(SCK,vendorID)
    if USE_FIXED_SEEDV:
        Seedv = FIXED_SEEDV
    else:
        Seedv = algo(MASK_KEY,vendorID)
    Modv = XOR(Seedv, algo(SCKv,Seedv))
    K3 = algo(Modv, moduleID)
    return {'SCKv':SCKv, 'Seedv':Seedv, 'Modv':Modv, 'K3':K3}

#performs the CW KLAD path for the ETSI TS 103 162 standard
def BasicKLAD(algoD, algoE, K3, CW):
    K2 = algoD(K3,EK2)
    K1 = algoD(K2,EK1)
    ECW = algoE(K1,CW)
    return {'K3':K3, 'K2':K2,'K1':K1,'ECW':ECW,'CW':CW}

#performs the Challenge-response path for the ETSI TS 103 162 standard
def BasicCR(algoD, K3):
    K2 = algoD(K3,EK2)
    A = algoD(K2,K2)
    dnonce = algoD(A,NONCE)
    return {'K3':K3, 'K2':K2, 'A': A, 'dnonce':dnonce}
#============================================
#Functions for pretty printing the vectors
#============================================
def prettyPrintKLADKeys(kdKeys,derivation):
    print 'Key Derivation Name: %s' % derivation['name']
    print 'SCK : %s' % h(SCK)
    print 'Mask Key : %s' % h(MASK_KEY)
    print 'VID : %s' % h(VENDOR_ID)
    #print 'Padded VID : %s' % h(derivation['vidPad'](VENDOR_ID))
    #print 'SCKv : %s' % h(kdKeys['SCKv'])
    #print 'Seedv : %s' % h(kdKeys['Seedv'])
    #check to see if there's a Modv key
    if 'Modv' in kdKeys:
        print 'MID : %s' % h(MODULE_ID)
        #print 'Padded MID : %s' % h(derivation['midPad'](MODULE_ID))
        #print 'Modv : %s' % h(kdKeys['Modv'])
    print 'K3 : %s' % h(kdKeys['K3'])

def prettyPrintCRKeys(crKeys, klad):
    print 'C/R Algo : %s' % klad['name']
    #print 'K3 : %s' % h(crKeys['K3'])
    print 'EK2 : %s' % h(EK2)
    #print 'K2 : %s' % h(crKeys['K2'])
    print 'A : %s' % h(crKeys['A'])
    print 'nonce : %s' % h(NONCE)
    print 'dnonce : %s' % h(crKeys['dnonce'])

#============================================
#These items describe the various key derivation/klad combinations we'll perform
#============================================
AESKLAD = {'algoD': AESDecrypt, 'algoE': AESEncrypt, 'cw8Padder': pad8, 'name':'AES'}
DES3KLAD = {'algoD': DES3Decrypt, 'algoE': DES3Encrypt, 'cw8Padder': padNone, 'name': '3DES'}
keyDerivations = (
    {'algo':DES3Decrypt, 'vidPad': padVendorIDDES3, 
            'name': 'Profile 1: Triple DES (decrypt) Profile'},
    {'algo':DES3Decrypt, 'vidPad': padVendorIDDES3, 'midPad':padModuleIDDES3,
            'name': 'Profile 1a: Triple DES (decrypt) Profile with Module Key Derivation'},
    {'algo':AESEncrypt, 'vidPad': padVendorIDAES, 'name': 'Profile 2: AES (encrypt) Profile'},
    {'algo':AESEncrypt, 'vidPad': padVendorIDAES, 'midPad':padModuleIDAES,
            'name': 'Profile 2a: AES (encrypt) with Module Key Derivation'},
    {'algo':AESDecrypt, 'vidPad': padVendorIDAES, 'midPad':padModuleIDAES,
            'name': 'Profile 2b: AES (decrypt) with Module Key Derivation'},
    )

#the main loop will iterate over the defined key derivation blocks
#for each key derivation, we use the resulting root key for all possible key ladder operations
#I.e., AES/TDES on both the CW and the C/R path.
for keyDerivation in keyDerivations:
    #we have slightly different handling for a key derivation depending on whether or not it supports Module ID
    if 'midPad' in keyDerivation:
        kdKeys = BasicKeyDerivationWithModuleID(keyDerivation['algo'], SCK,
                keyDerivation['vidPad'](VENDOR_ID), keyDerivation['midPad'](MODULE_ID))
    else:
        kdKeys = BasicKeyDerivation(keyDerivation['algo'], SCK,
                keyDerivation['vidPad'](VENDOR_ID))
    print ("==================================================================================================================")
    prettyPrintKLADKeys(kdKeys,keyDerivation)
    #iterate over KLAD algos
    for klad in (AESKLAD, ):
        #calculate Challenge/response keys.
        crKeys = BasicCR(klad['algoD'], kdKeys['K3'])
        prettyPrintCRKeys(crKeys,klad)
	print ("*******************************************************")
    for klad in (DES3KLAD, ):
        #calculate Challenge/response keys.
        crKeys = BasicCR(klad['algoD'], kdKeys['K3'])
        prettyPrintCRKeys(crKeys,klad)
    print ("==================================================================================================================")
