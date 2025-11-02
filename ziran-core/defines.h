#pragma once

#ifdef WIN32

#define DLLEXPORT __declspec(dllexport)

#include <WS2tcpip.h>
#include <WinSock2.h>

using SOCK = SOCKET;
using SOCKLEN = int;

inline int Get_Last_Socket_Error() {
    return WSAGetLastError();
}

inline int Network_Init() {
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    return WSAStartup(wVersionRequested, &wsaData);
}

inline int Network_Deinit() {
    return WSACleanup();
}

#else

#define DLLEXPORT 

#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstdlib>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>

using SOCK = int;
using SOCKLEN = socklen_t;

inline int closesocket(SOCK skt) {
    return close(skt);
}

inline int Get_Last_Socket_Error() {
    return errno;
}

inline int Network_Init() {
    // no need for networking initialization on other systems than Windows
    return 0;
}

inline int Network_Deinit() {
    return 0;
}

inline int ioctlsocket(SOCK fd, unsigned long request, u_long* val) {
    return ioctl(fd, request, val);
}

#endif

constexpr SOCK Invalid_Socket = static_cast<SOCK>(-1);
