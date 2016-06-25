/*******************************************************************  
* (c) Copyright 2011-2012 Discretix Technologies Ltd.              *
* This software is protected by copyright, international           *
* treaties and patents.                                            *
* Use of this Software as part of or with the Discretix CryptoCell *
* or Packet Engine products is governed by the products'           *
* commercial end user license agreement ("EULA").                  *
* It is possible that copies of this Software might be distributed *
* under some type of GNU General Public License ("GPL").           *
* Notwithstanding any such distribution under the terms of GPL,    *
* GPL does not govern the use of this Software as part of or with  *
* the Discretix CryptoCell or Packet Engine products, for which a  *
* EULA is required.                                                *
* If the product's EULA allows any copy or reproduction of this    *
* Software, then such copy or reproduction must include this       *
* Copyright Notice as well as any other notices provided           *
* thereunder.                                                      *
********************************************************************/

 
 
#ifndef CRYS_H
#define CRYS_H

/*
 * All the includes that are needed for code using this module to
 * compile correctly should be #included here.
 */

#include "dx_pal_types.h"
#include "crys_defs.h"
#include "crys_error.h"

#ifndef CRYS_NO_BYPASS_SUPPORT
#include "crys_bypass_api.h"
#endif

#ifndef CRYS_NO_AES_SUPPORT
#include "crys_aes.h"
#endif

#ifndef CRYS_NO_AESCCM_SUPPORT
#include "crys_aesccm.h"
#endif

#ifndef CRYS_NO_DES_SUPPORT
#include "crys_des.h"
#endif

#ifndef CRYS_NO_HASH_SUPPORT
#include "crys_hash.h"
#endif

#ifndef CRYS_NO_HMAC_SUPPORT
#include "crys_hmac.h"
#endif

#ifndef CRYS_NO_RC4_SUPPORT
#include "crys_rc4.h"
#endif

#ifndef CRYS_NO_CRYS_COMBINED_SUPPORT
#include "crys_combined.h"
#endif

#ifndef CRYS_NO_RND_SUPPORT
#include "crys_rnd.h"
#endif

#ifndef CRYS_NO_PKI_SUPPORT
#include "crys_rsa_build.h"
#include "crys_rsa_types.h"
#include "crys_rsa_schemes.h"
#include "crys_rsa_prim.h"
#include "crys_rsa_kg.h"
#endif

#ifndef CRYS_NO_ECPKI_SUPPORT
#include "crys_ecpki_types.h"
#include "crys_ecpki_build.h"
#include "crys_ecpki_kg.h"
#include "crys_ecpki_ecdsa.h"
#include "crys_ecpki_dh.h"
#endif

#ifndef CRYS_NO_KDF_SUPPORT
#include "crys_kdf.h"
#endif

#ifndef CRYS_NO_DH_SUPPORT
#include "crys_dh.h"
#endif

#ifndef CRYS_NO_RND_SUPPORT
#include "crys_rnd.h"
#endif


#ifdef __cplusplus
extern "C"
{
#endif


/************************ Defines ******************************/

/************************ Enums ********************************/

/************************ Typedefs  ****************************/

/************************ Structs  ******************************/

/************************ Public Variables **********************/

/************************ Public Functions **********************/

#ifdef __cplusplus
}
#endif

#endif
