/* xcpLite.h */

/* Copyright(c) Vector Informatik GmbH.All rights reserved.
   Licensed under the MIT license.See LICENSE file in the project root for details. */

#ifndef __XCPLITE_H_ 
#define __XCPLITE_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Include configuration headers                                           */
/***************************************************************************/

#include "xcptl_cfg.h"  // Transport layer configuration
#include "xcp_cfg.h" // Protocol layer configuration


/***************************************************************************/
/* Commands                                                                */
/***************************************************************************/

/*-------------------------------------------------------------------------*/
/* Standard Commands */

#define CC_CONNECT                        0xFF
#define CC_DISCONNECT                     0xFE


/****************************************************************************/
/* XCP Packet Type Definition                                               */
/****************************************************************************/

typedef union { 
  /* There might be a loss of up to 3 bytes. */
  vuint8  b[ ((XCPTL_CTO_SIZE + 3) & 0xFFC)      ];
  vuint16 w[ ((XCPTL_CTO_SIZE + 3) & 0xFFC) / 2  ];
  vuint32 dw[ ((XCPTL_CTO_SIZE + 3) & 0xFFC) / 4 ];
} tXcpCto;

extern void XcpDisconnect();

/* XCP command processor */
extern void XcpCommand( const vuint32* pCommand );

#ifdef __cplusplus
}
#endif

#endif

