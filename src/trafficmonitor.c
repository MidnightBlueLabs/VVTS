#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>

#include "common.h"

int g_hSocket = -1;

static int write_complete(int hSocket, void *lpBuf, unsigned int dwSize) {
    int dwBytesWritten;
    unsigned int dwOffset = 0;

    while (dwOffset != dwSize) {
        dwBytesWritten = write (hSocket, lpBuf, dwSize);
        if (dwBytesWritten < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            break;
        } else if (dwBytesWritten == 0) {
            break;
        }

        dwOffset += dwBytesWritten;
    }

    return dwOffset;
}

int main(int argc, char *argv[]) {
    char *szInterface = NULL;
    uint32_t dwSourceIp;
    uint32_t dwTargetIp;
    char szTargetIp[16];
    char szSourceIp[16];
    char szSourceMac[18];
    int dwBytesRecvd;
    struct sockaddr_ll stSockAddr;
    interface_info_t stInterfaceInfo;
    unsigned char *lpPacket = NULL;
    uint16_t wPacketSize;

    if (argc < 2) {
        fprintf_flush(stderr, "[i] usage: %s <interface>\n", argv[0]);
        return 0;
    }
    szInterface = argv[1];

    if ((g_hSocket = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL))) == -1) {
        fprintf_flush(stderr, "[-] unable to create raw socket: %s\n", strerror(errno));
        goto _error;
    }

    if (GetInterfaceInfo(&stInterfaceInfo, szInterface) != 0) {
        goto _error;
    }

    memset(&stSockAddr, 0, sizeof(struct sockaddr_ll));
    stSockAddr.sll_family = AF_PACKET;
    stSockAddr.sll_ifindex = stInterfaceInfo.dwIfIndex;
    stSockAddr.sll_protocol = htons(ETH_P_ALL);
    if (bind(g_hSocket, (struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_ll)) == -1) {
        fprintf_flush(stderr, "[-] unable to bind raw socket: %s\n", strerror(errno));
        goto _error;
    }

    if ((lpPacket = malloc(1600)) == NULL) {
        goto _error;
    }

    fprintf_flush(stderr, "[i] trafficmonitor running\n");
    fcntl(1, F_SETFL, fcntl(1, F_GETFL) & ~O_NONBLOCK);

    for(;;) {
        dwBytesRecvd = recv(g_hSocket, lpPacket, 1600, 0);
        if (dwBytesRecvd <= 0) {
            break;
        }

        wPacketSize = htons(dwBytesRecvd);
        write_complete(1, &wPacketSize, sizeof(wPacketSize));
        write_complete(1, lpPacket, dwBytesRecvd);
    }

_error:
    if (lpPacket != NULL) { free(lpPacket); }
    if (g_hSocket != -1) { close(g_hSocket); }
    return -1;
}
