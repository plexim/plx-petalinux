/*----------------------------------------------------------------------------
| File:
|   xcpTl.c
|
| Description:
|   XCP on UDP transport layer
|   Linux (Raspberry Pi) and Windows version
|   Supports Winsock, Linux Sockets and Vector XL-API V3
|
| Copyright (c) Vector Informatik GmbH. All rights reserved.
| Licensed under the MIT license. See LICENSE file in the project root for details.
|
 ----------------------------------------------------------------------------*/

#include "xcpTl.h"
#include "xcpLite.h"

static void xcpTlInitDefaults();


// XCP on UDP Transport Layer data
tXcpTlData gXcpTl;


#ifdef APP_ENABLE_MULTICAST
static int udpTlHandleXcpMulticast(int n, tXcpCtoMessage* p);
#endif


// Transmit a UDP datagramm (contains multiple XCP DTO messages or a single CRM message)
// Must be thread safe, because it is called from CMD and from DAQ thread
// Returns -1 on would block, 1 if ok, 0 on error
static int sendDatagram(const unsigned char* data, unsigned int size ) {

    int r;
        
#if defined ( XCP_ENABLE_TESTMODE )
    if (XcpDebugLevel >= 3) {
        XcpPrint("TX: size=%u ",size);
        for (unsigned int i = 0; i < size; i++) XcpPrint("%0X ", data[i]);
        XcpPrint("\n");
    }
#endif

    // Respond to active master
    if (!gXcpTl.MasterAddrValid) {
        XcpPrint("ERROR: invalid master address!\n");
        return 0;
    }
#ifdef APP_ENABLE_XLAPI_V3
    if (gOptionUseXLAPI) {
        r = (int)udpSendTo(gXcpTl.Sock.sockXl, data, size, 0, &gXcpTl.MasterAddr.addrXl, (socklen_t)sizeof(gXcpTl.MasterAddr.addrXl));
    }
    else 
#endif    
    {
        mutexLock(&gXcpTl.Mutex_Send);
        r = (int)sendto(gXcpTl.Sock.sock, data, size, SENDTO_FLAGS, (SOCKADDR*)&gXcpTl.MasterAddr.addr, (uint16_t)sizeof(gXcpTl.MasterAddr.addr));
        mutexUnlock(&gXcpTl.Mutex_Send);
    }
    if (r != size) {
        if (socketGetLastError()==SOCKET_ERROR_WBLOCK) {
            return -1; // Would block
        }
        else {
            XcpPrint("ERROR: sento failed (result=%d, errno=%d)!\n", r, socketGetLastError());
        }
        return 0; // Error
    }

    return 1; // Ok
}


//------------------------------------------------------------------------------

// Transmit XCP response or event packet
// Returns 0 error, 1 ok, -1 would block
int udpTlSendCrmPacket(void* aBuffer) {

    int result;
    unsigned int retries = SEND_RETRIES;

    // Build XCP CTO message (ctr+dlc+packet)
    tXcpCtoMessage* p = (tXcpCtoMessage*)aBuffer;
    uint16_t size = p->dlc;
    result = sendDatagram((unsigned char*)p, size + XCPTL_TRANSPORT_LAYER_HEADER_SIZE);
    if (result == -1) // retry on would block (-1)
    {
       tXcpCtoMessage p_buf;
       memcpy(&p_buf, p, size + XCPTL_TRANSPORT_LAYER_HEADER_SIZE);
       do {
          result = sendDatagram((unsigned char*)&p_buf, size + XCPTL_TRANSPORT_LAYER_HEADER_SIZE);
          if (result != -1) break; // break on success or error, retry on would block (-1)
          usleep(1);
       } while (--retries > 0);
    }
    return result;
}

// Transmit XCP response or event packet
// Returns 0 error, 1 ok, -1 would block
int udpTlSendDtoPacket(void* aBuffer) {

    tXcpDtoBuffer* p = (tXcpDtoBuffer*)aBuffer;
    return sendDatagram(&p->xcp[0], p->xcp_size);
}
    
static int udpTlHandleXcpCommands(int n, tXcpCtoMessage * p, tUdpSockAddr * src) {

    int connected;

    if (n >= XCPTL_TRANSPORT_LAYER_HEADER_SIZE+1) { // Valid socket data received, at least transport layer header and 1 byte

        // gXcpTl.LastCrmCtr = p->ctr;
        connected = gXcpTl.connected;

#ifdef XCP_ENABLE_TESTMODE
        if (XcpDebugLevel >= 2 || (!connected && XcpDebugLevel >= 1)) {
            XcpPrint("RX: CTR %04X LEN %04X DATA = ", p->ctr,p->dlc);
            for (int i = 0; i < p->dlc; i++) XcpPrint("%0X ", p->data[i]);
            XcpPrint("\n");
        }
#endif
        const tXcpCto* pCmd = (const tXcpCto*)&p->data[0];
        /* Connected */
        if (connected) {
            
            // Check src addr 
            assert(gXcpTl.MasterAddrValid);

            if (src != NULL) {

                // Check unicast ip address, not allowed to change 
                if (memcmp(&gXcpTl.MasterAddr.addr.sin6_addr, &src->addr.sin6_addr, sizeof(src->addr.sin6_addr)) != 0) { // Message from different master received
                    char tmp[64];
                    inet_ntop(AF_INET6, &src->addr.sin6_addr, tmp, sizeof(tmp));
                    XcpPrint("WARNING: message from unknown new master %s, disconnecting!\n", tmp);
                    XcpDisconnect();
                    gXcpTl.MasterAddrValid = 0;
                    gXcpTl.connected = false;
                    return 1; // Disconnect
                }

                // Check unicast master udp port, not allowed to change 
                if (gXcpTl.MasterAddr.addr.sin6_port != src->addr.sin6_port) {
                    XcpPrint("WARNING: master port changed from %u to %u, disconnecting!\n", htons(gXcpTl.MasterAddr.addr.sin6_port), htons(src->addr.sin6_port));
                    XcpDisconnect();
                    gXcpTl.MasterAddrValid = 0;
                    gXcpTl.connected = false;
                    return 1; // Disconnect
                }
                if (p->dlc == 1 && pCmd->b[0] == CC_DISCONNECT) {
                    gXcpTl.connected = false;
                    XcpPrint("Disconnect\n");
                }
            }

            XcpCommand((const vuint32*)&p->data[0]); // Handle command
        }

        /* Not connected yet */
        else {
            /* Check for CONNECT command ? */
            if (p->dlc == 2 && pCmd->b[0] == CC_CONNECT) { 
                gXcpTl.MasterAddr = *src; // Save master address, so XcpCommand can send the CONNECT response
                gXcpTl.MasterAddrValid = 1;
                XcpCommand((const vuint32*)&p->data[0]); // Handle CONNECT command
                gXcpTl.connected = true;
            }
#ifdef XCP_ENABLE_TESTMODE
            else if (XcpDebugLevel >= 1) {
                XcpPrint("WARNING: no valid CONNECT command\n");
            }
#endif
        }
       
        // Actions after successfull connect
        if (!connected) {
            if (gXcpTl.connected) { // Is in connected state

#ifdef XCP_ENABLE_TESTMODE
                {
                    char tmp[64];
                    inet_ntop(AF_INET6, &gXcpTl.MasterAddr.addr.sin6_addr, tmp, sizeof(tmp));
                    XcpPrint("XCP master connected: addr=%s, port=%u\n", tmp, htons(gXcpTl.MasterAddr.addr.sin6_port));
                }
#endif

                // Inititialize the DAQ message queue
                //udpTlInitTransmitQueue(); 

            }  
            else { // Is not in connected state
                gXcpTl.MasterAddrValid = 0; // Any client can connect
            } 
        } // !connected before

    }
    else if (n>0) {
        XcpPrint("WARNING: invalid transport layer header received!\n");
        return 0; // Error
    }
    return 1; // Ok
}


// Handle incoming XCP commands
// returns 0 on error
int udpTlHandleCommands() {

    uint8_t buffer[XCPTL_TRANSPORT_LAYER_HEADER_SIZE + XCPTL_CTO_SIZE];
    tUdpSockAddr src;
    socklen_t srclen;
    int n;

    // Receive a UDP datagramm
    // No no partial messages assumed
#ifdef APP_ENABLE_XLAPI_V3
    if (gOptionUseXLAPI) {
        unsigned int flags;
        srclen = sizeof(src.addrXl);
        n = udpRecvFrom(gXcpTl.Sock.sockXl, (char*)&buffer, sizeof(buffer), &src.addrXl, &flags);
        if (n <= 0) {
            if (n == 0) return 1; // Ok, no command pending
            if (socketGetLastError() == SOCKET_ERROR_WBLOCK) return 1; // Ok, no command pending
            XcpPrint("ERROR %u: recvfrom failed (result=%d)!\n", socketGetLastError(), n);
           return 0; // Error
        }
#ifdef APP_ENABLE_MULTICAST
        if (flags & RECV_FLAGS_MULTICAST) {
            return udpTlHandleXcpMulticast(n, (tXcpCtoMessage*)buffer);
        }
#endif
    }
    else 
#endif
    {
        srclen = sizeof(src.addr);
        n = (int)recvfrom(gXcpTl.Sock.sock, (char*)&buffer, (uint16_t)sizeof(buffer), 0, (SOCKADDR*)&src.addr, &srclen); // recv blocking
        if (n <= 0) {
            if (n == 0) return 1; // Ok, no command pending
            if (socketGetLastError() == SOCKET_ERROR_WBLOCK) {
                return 1; // Ok, no command pending
            }
                XcpPrint("ERROR %u: recvfrom failed (result=%d)!\n", socketGetLastError(), n);
            return 0; // Error           
        }
    }

    return udpTlHandleXcpCommands(n, (tXcpCtoMessage*)buffer, &src);
}


//-------------------------------------------------------------------------------------------------------
// XCP Multicast

#ifdef APP_ENABLE_MULTICAST

static int udpTlHandleXcpMulticast(int n, tXcpCtoMessage* p) {

    // Valid socket data received, at least transport layer header and 1 byte
    if (gXcpTl.MasterAddrValid && XcpIsConnected() && n >= XCPTL_TRANSPORT_LAYER_HEADER_SIZE + 1) {
        XcpCommand((const vuint32*)&p->data[0]); // Handle command
    }
    return 1; // Ok
}

#ifdef _WIN
DWORD WINAPI udpTlMulticastThread(LPVOID lpParameter)
#else
extern void* udpTlMulticastThread(void* par)
#endif
{
    uint8_t buffer[256];
    int n;
    char tmp[32];
    uint16_t cid = XcpGetClusterId();
    uint8_t cip[4] = { 239,255,(uint8_t)(cid >> 8),(uint8_t)(cid) };

    XcpPrint("Start XCP multicast thread\n");
    if (!socketOpen(&gXcpTl.MulticastSock, FALSE /*nonblocking*/, TRUE /*reusable*/)) return 0;
    if (!socketBind(gXcpTl.MulticastSock, 5557)) return 0;
    if (!socketJoin(gXcpTl.MulticastSock, cip)) return 0;
    inet_ntop(AF_INET, cip, tmp, sizeof(tmp));
    XcpPrint("  Listening on %s port=%u\n\n", tmp, 5557);
    for (;;) {
        n = socketRecv(gXcpTl.MulticastSock, (char*)&buffer, (uint16_t)sizeof(buffer));
        if (n < 0) break; // Terminate on error (socket close is used to terminate thread)
        udpTlHandleXcpMulticast(n, (tXcpCtoMessage*)buffer);
    }
    XcpPrint("Terminate XCP multicast thread\n");
    socketClose(&gXcpTl.MulticastSock);
    return 0;
}

#endif





//-------------------------------------------------------------------------------------------------------

#ifdef _LINUX // Linux

#include <linux/if_packet.h>

static int GetMAC(char* ifname, uint8_t* mac) {
    struct ifaddrs* ifap, * ifaptr;

    if (getifaddrs(&ifap) == 0) {
        for (ifaptr=ifap; ifaptr!=NULL; ifaptr=ifaptr->ifa_next) {
            if (!strcmp(ifaptr->ifa_name, ifname) && ifaptr->ifa_addr->sa_family == AF_PACKET) {
                struct sockaddr_ll* s = (struct sockaddr_ll*)ifaptr->ifa_addr;
                memcpy(mac, s->sll_addr, 6);
                break;
            }
        }
        freeifaddrs(ifap);
        return ifaptr != NULL;
    }
    return 0;
}

uint32_t GetLocalIPAddr( uint8_t *mac )
{
    struct ifaddrs* ifaddr;
    char strbuf[100];
    uint32_t addr1 = 0;
    uint8_t mac1[6] = {0,0,0,0,0,0};
    struct ifaddrs* ifa1 = NULL;

    if (-1 != getifaddrs(&ifaddr)) {
        for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if ((NULL != ifa->ifa_addr) && (AF_INET == ifa->ifa_addr->sa_family)) {
                struct sockaddr_in* sa = (struct sockaddr_in*)(ifa->ifa_addr);
                if (0x100007f != sa->sin_addr.s_addr) { /* not loop back adapter (127.0.0.1) */
                    inet_ntop(AF_INET, &sa->sin_addr.s_addr, strbuf, sizeof(strbuf));
                    XcpPrint("  Network interface %s: ip=%s\n", ifa->ifa_name, strbuf );
                    if (addr1 == 0) {
                        addr1 = sa->sin_addr.s_addr;
                        ifa1 = ifa;
                    }
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    if (addr1 != 0) {
        GetMAC(ifa1->ifa_name, mac1);
        if (mac) memcpy(mac, mac1, 6);
        inet_ntop(AF_INET, &addr1, strbuf, sizeof(strbuf));
        XcpPrint("  Use adapter %s with ip=%s, mac=%02X-%02X-%02X-%02X-%02X-%02X for A2L info and clock UUID\n", ifa1->ifa_name, strbuf, mac1[0], mac1[1], mac1[2], mac1[3], mac1[4], mac1[5]);
    }
    return addr1;
}


int networkInit() {

    XcpPrint("Init Network\n");
    xcpTlInitDefaults();
    return 1;
}


extern void networkShutdown() {

}


int udpTlInit(uint8_t notUsedSlaveAddr1[4], uint16_t slavePort)
{
    // gXcpTl.LastCroCtr = 0;
    // gXcpTl.DtoCtr = 0;
    // gXcpTl.CrmCtr = 0;
    gXcpTl.MasterAddrValid = 0;

    if (!socketOpen(&gXcpTl.Sock.sock, FALSE, FALSE)) return 0;
    if (!socketBind(gXcpTl.Sock.sock, slavePort)) return 0;
    XcpPrint("  Listening on UDP port %u\n\n", slavePort);
    // Create multicast thread
#ifdef APP_ENABLE_MULTICAST
    create_thread(&gXcpTl.MulticastThreadHandle, udpTlMulticastThread);
    sleepMs(50);
#endif

    XcpPrint("\n");
    return 1;
}

/*
// Wait for outgoing data or timeout after timeout_us
void udpTlWaitForTransmitData(unsigned int timeout_us) {
    if (gXcpTl.dto_queue_len <= 1) {
        usleep(timeout_us);
    }
    return; 
}
*/

void udpTlShutdown() {

#ifdef APP_ENABLE_MULTICAST
    socketClose(&gXcpTl.MulticastSock);
    sleepMs(500);
    cancel_thread(gXcpTl.MulticastThreadHandle);
#endif
    mutexDestroy(&gXcpTl.Mutex_Send);
    socketClose(&gXcpTl.Sock.sock);
}


#endif

#ifdef _WIN 

#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

uint32_t GetLocalIPAddr( uint8_t *mac ) {

    uint32_t addr1 = 0;
    uint8_t mac1[6] = { 0,0,0,0,0,0 };

    uint32_t addr;
    uint32_t index1 = 0;
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    PIP_ADAPTER_INFO pAdapter1 = NULL;
    DWORD dwRetVal = 0;

    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) return 0;

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (pAdapterInfo == NULL) return 0;
    }
    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            if (pAdapter->Type == MIB_IF_TYPE_ETHERNET) {
                inet_pton(AF_INET, pAdapter->IpAddressList.IpAddress.String, &addr);
                if (addr) {
                    XcpPrint("  Ethernet adapter %d:", pAdapter->Index);
                    //XcpPrint(" %s", pAdapter->AdapterName);
                    XcpPrint(" %s", pAdapter->Description);
                    XcpPrint(" %02X-%02X-%02X-%02X-%02X-%02X", pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2], pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
                    XcpPrint(" %s", pAdapter->IpAddressList.IpAddress.String);
                    //XcpPrint(" %s", pAdapter->IpAddressList.IpMask.String);
                    //XcpPrint(" Gateway: %s", pAdapter->GatewayList.IpAddress.String);
                    //if (pAdapter->DhcpEnabled) XcpPrint(" DHCP");
                    XcpPrint("\n");
                    if (addr1 == 0 || ((uint8_t*)&addr)[0]==172) { // prefer 172.x.x.x
                        addr1 = addr; 
                        index1 = pAdapter->Index; 
                        memcpy(mac1, pAdapter->Address, 6);
                        pAdapter1 = pAdapter;
                    }
                }
            }
            pAdapter = pAdapter->Next;
        }
    }
    else {
        return 0;
    }
    if (addr1) {
        XcpPrint("  Use adapter %d ip=%s, mac=%02X-%02X-%02X-%02X-%02X-%02X for A2L info and clock UUID\n", index1, pAdapter1->IpAddressList.IpAddress.String, mac1[0], mac1[1], mac1[2], mac1[3], mac1[4], mac1[5]);
        if (mac) memcpy(mac, mac1, 6);
    }
    if (pAdapterInfo) free(pAdapterInfo);
    return addr1;
}

int networkInit() {

    int err;

    XcpPrint("Init Network\n");

#ifdef APP_ENABLE_XLAPI_V3
    if (!gOptionUseXLAPI)
#endif
    {
        WORD wsaVersionRequested;
        WSADATA wsaData;

        // Init Winsock2
        wsaVersionRequested = MAKEWORD(2, 2);
        err = WSAStartup(wsaVersionRequested, &wsaData);
        if (err != 0) {
            XcpPrint("ERROR: WSAStartup failed with ERROR: %d!\n", err);
            return 0;
        }
        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) { // Confirm that the WinSock DLL supports 2.2
            XcpPrint("ERROR: could not find a usable version of Winsock.dll!\n");
            WSACleanup();
            return 0;
        }
    }

    xcpTlInitDefaults();
    return 1;
}


extern void networkShutdown() {

    WSACleanup();
}


int udpTlInit(uint8_t *slaveAddr, uint16_t slavePort, uint16_t slaveMTU) {

    XcpPrint("\nInit XCP on UDP transport layer\n  (MTU=%u, DTO_QUEUE_SIZE=%u)\n", slaveMTU, XCPTL_DTO_QUEUE_SIZE);

    gXcpTl.SlaveMTU = slaveMTU;
    if (gXcpTl.SlaveMTU > XCPTL_SOCKET_JUMBO_MTU_SIZE) gXcpTl.SlaveMTU = XCPTL_SOCKET_JUMBO_MTU_SIZE;
    // gXcpTl.LastCroCtr = 0;
    // gXcpTl.DtoCtr = 0;
    // gXcpTl.CrmCtr = 0;
    gXcpTl.MasterAddrValid = 0;

    mutexInit(&gXcpTl.Mutex_Send,FALSE,0);

#ifdef APP_ENABLE_XLAPI_V3
    if (gOptionUseXLAPI) {     
        // XCP multicast IP-addr and port
        uint16_t cid = XcpGetClusterId();
        uint8_t cip[4] = { 239,255,(uint8_t)(cid >> 8),(uint8_t)(cid) };
#ifdef APP_ENABLE_MULTICAST
        memset((void*)&gXcpTl.MulticastAddrXl.addrXl, 0, sizeof(gXcpTl.MulticastAddrXl.addrXl));
        memcpy(gXcpTl.MulticastAddrXl.addrXl.sin_addr, cip, 4);
        gXcpTl.MulticastAddrXl.addrXl.sin_port = HTONS(5557);
        return udpInit(&gXcpTl.Sock.sockXl, &gXcpTl.SlaveAddr.addrXl, &gXcpTl.MulticastAddrXl.addrXl);
#else
        return udpInit(&gXcpTl.Sock.sockXl, &gXcpTl.SlaveAddr.addrXl, NULL);
#endif
    }
    else 
#endif
    {            
        if (!socketOpen(&gXcpTl.Sock.sock, FALSE, FALSE)) return 0;
        if (!socketBind(gXcpTl.Sock.sock,slavePort)) return 0;
        XcpPrint("  Listening on UDP port %u\n\n", slavePort);
#ifdef APP_ENABLE_MULTICAST
        create_thread(&gXcpTl.MulticastThreadHandle, udpTlMulticastThread);
#endif
        return 1;
    }
}


void udpTlWaitForTransmitData(unsigned int timeout_us) {

    if (gXcpTl.dto_queue_len <= 1) {
        assert(timeout_us >= 1000);
        Sleep(timeout_us/1000);
    }
    return;

}


void udpTlShutdown() {

#ifdef APP_ENABLE_XLAPI_V3
    if (gOptionUseXLAPI) {
        udpShutdown(gXcpTl.Sock.sockXl);
    }
    else 
#endif
    {
#ifdef APP_ENABLE_MULTICAST
        socketClose(&gXcpTl.MulticastSock);
        sleepMs(500);
        cancel_thread(gXcpTl.MulticastThreadHandle);
#endif
        socketClose(&gXcpTl.Sock.sock);
    }
}

#endif

//-------------------------------------------------------------------------------------------------------

static void xcpTlInitDefaults() {

    uint8_t uuid[8] = APP_DEFAULT_SLAVE_UUID;
    uint32_t a;
    uint8_t m[6];

#ifdef APP_ENABLE_XLAPI_V3
    if (gOptionUseXLAPI) {

        // MAC, Unicast IP-addr and port
        uint8_t mac[8] = APP_DEFAULT_SLAVE_MAC;
        memset((void*)&gXcpTl.SlaveAddr.addrXl, 0, sizeof(gXcpTl.SlaveAddr.addrXl));
        memcpy(gXcpTl.SlaveAddr.addrXl.sin_mac, mac, 6);
        memcpy(gXcpTl.SlaveAddr.addrXl.sin_addr, gOptionSlaveAddr, 4);
        gXcpTl.SlaveAddr.addrXl.sin_port = HTONS(gOptionSlavePort);
        memcpy(&gXcpTl.SlaveUUID, uuid, 8);
    }
    else
#endif
    {
        // Create a UUID for slave clock
        // Determine slave ip for A2L generation
        memset((void*)&gXcpTl.SlaveAddr.addr, 0, sizeof(gXcpTl.SlaveAddr.addr));
        a = GetLocalIPAddr(m);
        if (a == 0) {
            //memcpy(&gXcpTl.SlaveAddr.addr.sin_addr, gOptionSlaveAddr, 4);
            gXcpTl.SlaveAddr.addr.sin6_port = htons(gOptionSlavePort);
            memcpy(&gXcpTl.SlaveUUID, uuid, 8);
        }
        else {
            //memcpy(&gXcpTl.SlaveAddr.addr.sin_addr, &a, 4);
            gXcpTl.SlaveAddr.addr.sin6_port = htons(gOptionSlavePort);
            memcpy(&gXcpTl.SlaveUUID[0], &m[0], 3);
            gXcpTl.SlaveUUID[3] = 0xFF; gXcpTl.SlaveUUID[4] = 0xFE;
            memcpy(&gXcpTl.SlaveUUID[5], &m[3], 3);
        }
    }
}
