/*----------------------------------------------------------------------------
| File:
|   xcptl_cfg.h
|
| Description:
|   User configuration file for XCP transport layer parameters
 ----------------------------------------------------------------------------*/

#ifndef _XCPTL_CFG_H_
#define _XCPTL_CFG_H_

#define XCPTL_TRANSPORT_LAYER_HEADER_SIZE 4

 // Transport layer version 
 #define XCP_TRANSPORT_LAYER_VERSION 0x0104 

 // UDP socket MTU
#define XCPTL_SOCKET_MTU_SIZE 1400
#define XCPTL_SOCKET_JUMBO_MTU_SIZE 7500

// DTO size (does not need jumbo frames)
#define XCPTL_DTO_SIZE (XCPTL_SOCKET_MTU_SIZE-XCPTL_TRANSPORT_LAYER_HEADER_SIZE)

// CTO size
#define XCPTL_CTO_SIZE 250

 // DTO queue entry count 
#define XCPTL_DTO_QUEUE_SIZE 8   // DAQ transmit queue size in UDP packets, should at least be able to hold all data produced until the next call to udpTlHandleTransmitQueue

#define XcpDebugLevel 2
#define XcpPrint printf

#define APP_DEFAULT_SLAVE_UUID {0xdc,0xa6,0x32,0xFF,0xFE,0x7e,0x66,0xdc} // Slave clock UUID 

extern uint32_t* gOptionSlaveAddr;
extern uint16_t gOptionSlavePort;

// Multicast (GET_DAQ_CLOCK_MULTICAST)
// Use multicast time synchronisation to improve synchronisation of multiple XCP slaves
// This is standard in XCP V1.3, but it needs to create an additional thread and socket for multicast reception
// Adjust CANape setting in device/protocol/event/TIME_CORRELATION_GETDAQCLOCK from "multicast" to "extended response" if this is not desired 
//#define APP_ENABLE_MULTICAST 
#ifdef APP_ENABLE_MULTICAST
    // XCP default cluster id (multicast addr 239,255,0,1, group 127,0,1 (mac 01-00-5E-7F-00-01)
    // #define XCP_MULTICAST_CLUSTER_ID 1
    // #define XCP_MULTICAST_PORT 5557
    #define XCP_DEFAULT_MULTICAST_MAC {0x01,0x00,0x5E,0x7F,0x00,0x01}
#endif

#endif



