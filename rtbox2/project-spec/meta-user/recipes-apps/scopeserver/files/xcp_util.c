/*----------------------------------------------------------------------------
| File:
|   util.c
|
| Description:
|   Some helper functions
|
|   Code released into public domain, no attribution required
 ----------------------------------------------------------------------------*/

#include "xcpTl.h"



/**************************************************************************/
// Keyboard
/**************************************************************************/

/*
#ifdef _LINUX

int _getch() {
    static int ch = -1, fd = 0;
    struct termios neu, alt;
    fd = fileno(stdin);
    tcgetattr(fd, &alt);
    neu = alt;
    neu.c_lflag = (unsigned int)neu.c_lflag & ~(unsigned int)(ICANON | ECHO);
    tcsetattr(fd, TCSANOW, &neu);
    ch = getchar();
    tcsetattr(fd, TCSANOW, &alt);
    return ch;
}

int _kbhit() {
    struct termios term, oterm;
    int fd = 0;
    int c = 0;
    tcgetattr(fd, &oterm);
    memcpy(&term, &oterm, sizeof(term));
    term.c_lflag = term.c_lflag & (!ICANON);
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 1;
    tcsetattr(fd, TCSANOW, &term);
    c = getchar();
    tcsetattr(fd, TCSANOW, &oterm);
    if (c != -1)
        ungetc(c, stdin);
    return ((c != -1) ? 1 : 0);
}

#endif
*/

/**************************************************************************/
// Mutex
/**************************************************************************/

#ifdef _LINUX

void mutexInit(MUTEX* m, int recursive, uint32_t spinCount) {

    if (recursive) {
        pthread_mutexattr_t ma;
        pthread_mutexattr_init(&ma);
        pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(m, &ma);
    }
    else {
        pthread_mutex_init(m, NULL);
    }
}

void mutexDestroy(MUTEX* m) {

    pthread_mutex_destroy(m);
}

#else 

void mutexInit(MUTEX* m, int recursive, uint32_t spinCount) {
    // Window critical sections are always recursive
    InitializeCriticalSectionAndSpinCount(m,spinCount);
}

void mutexDestroy(MUTEX* m) {

    DeleteCriticalSection(m);
}

#endif


/**************************************************************************/
// Sockets
/**************************************************************************/

#ifdef _LINUX

int socketOpen(SOCKET* sp, int nonBlocking, int reuseaddr) {

    // Create a socket
    *sp = socket(AF_INET6, SOCK_DGRAM, 0);
    if (*sp < 0) {
        printf("ERROR: cannot open socket!\n");
        return 0;
    }

    if (reuseaddr) {
        int yes = 1;
        setsockopt(*sp, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    }

    return 1;
}

int socketBind(SOCKET sock, uint16_t port) {

    // Bind the socket to any address and the specified port
    SOCKADDR_IN a;
    a.sin6_family = AF_INET6;
    a.sin6_addr = in6addr_any;
    a.sin6_port = htons(port);
    if (bind(sock, (SOCKADDR*)&a, sizeof(a)) < 0) {
        printf("ERROR %u: cannot bind on UDP port %u!\n", socketGetLastError(), port);
        return 0;
    }

    return 1;
}


int socketClose(SOCKET *sp) {
    if (*sp != INVALID_SOCKET) {
        // shutdown(*sp, SHUT_RDWR);
        close(*sp);
        *sp = INVALID_SOCKET;
    }
    return 1;
}

#endif

/*
int socketJoin(SOCKET sock, uint8_t* addr) {

    struct ip_mreq group;
    group.imr_multiaddr.s_addr = *(uint32_t*)addr;
    group.imr_interface.s_addr = htonl(INADDR_ANY);
    if (0 > setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&group, sizeof(group))) {
        printf("ERROR %u: Failed to set multicast socket option IP_ADD_MEMBERSHIP!\n", socketGetLastError());
        return 0;
    }
    return 1;
}

*/

int socketRecvFrom(SOCKET sock, uint8_t* buffer, uint16_t bufferSize, uint8_t *addr, uint16_t *port) {

    SOCKADDR_IN src;
    socklen_t srclen = sizeof(src);
    int n = (int)recvfrom(sock, buffer, bufferSize, 0, (SOCKADDR*)&src, &srclen);
    if (n == 0) return 0;
    if (n < 0) {
        if (socketGetLastError() == SOCKET_ERROR_WBLOCK) return 0;
        if (socketGetLastError() == SOCKET_ERROR_CLOSED) {
            printf("Socket closed\n");
            return -1; // Socket closed
        }
        printf("ERROR %u: recvfrom failed (result=%d)!\n", socketGetLastError(), n);
    }
    if (port) *port = htons(src.sin6_port);
    if (addr) memcpy(addr, &src.sin6_addr.s6_addr, sizeof(src.sin6_addr.s6_addr));
    return n;
}

int socketRecv(SOCKET sock, uint8_t* buffer, uint16_t bufferSize) {

    return socketRecvFrom(sock, buffer, bufferSize, NULL, NULL);
}



