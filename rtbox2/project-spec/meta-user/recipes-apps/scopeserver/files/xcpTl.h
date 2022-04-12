/* xcpTl.h */

/* Copyright(c) Vector Informatik GmbH.All rights reserved.
   Licensed under the MIT license.See LICENSE file in the project root for details. */

#ifndef _XCPTL_H__
#define _XCPTL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define _LINUX

#ifdef _LINUX // Linux sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

    #define RECV_FLAGS 0 // Blocking receive (no MSG_DONTWAIT)
    #define SENDTO_FLAGS 0 // Blocking transmit (no MSG_DONTWAIT)
    #define SEND_RETRIES 10 // Retry when send CRM would block


#endif 


#ifdef _WIN // Windows sockets or XL-API

    #ifdef APP_ENABLE_XLAPI_V3
      #undef udpSendtoWouldBlock
      #define udpSendtoWouldBlock(r) (gOptionUseXLAPI ? (r==0) : (WSAGetLastError()==WSAEWOULDBLOCK))
    #endif

    #define RECV_FLAGS 0
    #define SENDTO_FLAGS 0
    #define SEND_RETRIES 10 // Retry when send CRM would block

#endif

#include "xcp_util.h"
#include "xcptl_cfg.h"

typedef struct {
    uint16_t dlc;               /* BYTE 1,2 lenght */
    uint16_t ctr;               /* BYTE 3,4 packet counter */
    unsigned char  data[XCPTL_CTO_SIZE];  /* BYTE[] data */
} tXcpCtoMessage;

typedef struct {
    uint16_t dlc;               /* BYTE 1,2 lenght */
    uint16_t ctr;               /* BYTE 3,4 packet counter */
    unsigned char  data[XCPTL_DTO_SIZE];  /* BYTE[] data */
} tXcpDtoMessage;


typedef struct {
    unsigned int xcp_size;             // Number of overall bytes in XCP DTO messages
    unsigned int xcp_uncommited;       // Number of uncommited XCP DTO messages
    unsigned char xcp[XCPTL_SOCKET_JUMBO_MTU_SIZE]; // Contains concatenated messages
} tXcpDtoBuffer;


typedef union {
    SOCKADDR_IN addr;
#ifdef APP_ENABLE_XLAPI_V3
    tUdpSockAddrXl addrXl;
#endif
} tUdpSockAddr;

typedef union {
    SOCKET sock; // Winsock or Linux
#ifdef APP_ENABLE_XLAPI_V3
    tUdpSockXl *sockXl; // XL-API
#endif
} tUdpSock;

typedef struct {
    
    tUdpSock Sock;
    bool connected;
    unsigned int SlaveMTU;
    tUdpSockAddr SlaveAddr;
    uint8_t SlaveUUID[8];
    tUdpSockAddr MasterAddr;
    int MasterAddrValid;


    // Multicast
#ifdef APP_ENABLE_MULTICAST
    tXcpThread MulticastThreadHandle;
    SOCKET MulticastSock;
    // XL-API
    #ifdef APP_ENABLE_XLAPI_V3
        tUdpSockAddr MulticastAddrXl;
    #endif
#endif 

    MUTEX Mutex_Send;

} tXcpTlData;

extern tXcpTlData gXcpTl;

extern int networkInit();
extern void networkShutdown();

extern int udpTlInit(uint8_t*slaveAddr, uint16_t slavePort);
extern void udpTlShutdown();

extern int udpTlHandleCommands();

#ifdef __cplusplus
}
#endif

#endif
