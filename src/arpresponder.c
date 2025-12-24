#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <poll.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <linux/rtnetlink.h>

#include "common.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define MILLIS 250

typedef struct {
    /* === ipv6 header === */
    uint32_t ip6_un1_flow;   /* 4 bits version, 8 bits TC, 20 bits flow-ID */
    uint16_t ip6_un1_plen;   /* payload length */
    uint8_t  ip6_un1_nxt;    /* next header */
    uint8_t  ip6_un1_hlim;   /* hop limit */
    struct in6_addr ip6_src;      /* source address */
    struct in6_addr ip6_dst;      /* destination address */

    /* === icmpv6 header === */
    uint8_t     icmp6_type;   /* type field */
    uint8_t     icmp6_code;   /* code field */
    uint16_t    icmp6_cksum;  /* checksum field */

    /* === neighbor solicitation/advertisement fields === */
	uint8_t   icmp6_un_data8[4];  /* type-specific field */
    struct in6_addr abTargetAddr;
    uint8_t bType;
    uint8_t bLength;
    uint8_t abMacAddress[6];
} neighbor_packet_t;

typedef struct scheduled_packet_t {
    /* struct that holds the packet scheduled for emission */
    struct scheduled_packet_t *lpNext;
    int64_t qwScheduledTime;
    unsigned char abBuffer[MAX(sizeof(struct ether_arp), sizeof(neighbor_packet_t))];
    unsigned int dwSize;
    struct sockaddr_ll stDestSockAddr;
} scheduled_packet_t;

static const char *g_szInterface = NULL;

/* g_lpScheduled is a linked list, ordered from first scheduled packet to last */
static scheduled_packet_t *g_lpScheduled = NULL;
static scheduled_packet_t **g_lpScheduledTail = &g_lpScheduled;

/*
 * schedule a packet to be sent in MILLIS msec
 * lpBuffer: the data
 * dwSize: size of the data
 * lpDestAddr: struct sockaddr_ll containing destination mac, ifindex and protocol
 */
static void schedule_packet(void *lpBuffer, unsigned int dwSize, struct sockaddr_ll *lpDestAddr) {
    struct timespec stTimeSpec;
    scheduled_packet_t *lpPacket = malloc(sizeof(scheduled_packet_t));
    if (clock_gettime(CLOCK_MONOTONIC, &stTimeSpec) == -1) {
        stTimeSpec.tv_sec = 0;
        stTimeSpec.tv_nsec = 0;
    }

    memset(lpPacket, 0, sizeof(scheduled_packet_t));
    lpPacket->qwScheduledTime = stTimeSpec.tv_sec * 1000 + (stTimeSpec.tv_nsec / 1000000) + MILLIS;
    memcpy(lpPacket->abBuffer, lpBuffer, MIN(sizeof(lpPacket->abBuffer), dwSize));
    lpPacket->dwSize = MIN(sizeof(lpPacket->abBuffer), dwSize);
    memcpy(&lpPacket->stDestSockAddr, lpDestAddr, sizeof(struct sockaddr_ll));

    /* append to linked list */
    *g_lpScheduledTail = lpPacket;
    g_lpScheduledTail = &lpPacket->lpNext;
}

/*
 * get the number of milliseconds until the packet scheduled soonest is to be sent
 */
static int schedule_get_soonest(void) {
    struct timespec stTimeSpec;
    long long qwCurrentTime;

    /* nothing is scheduled -> wait until packet comes in or indefinitely */
    if (g_lpScheduled == NULL) {
        return -1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &stTimeSpec) == -1) {
        stTimeSpec.tv_sec = 0;
        stTimeSpec.tv_nsec = 0;
    }

    /* convert to millis */
    qwCurrentTime = stTimeSpec.tv_sec * 1000 + (stTimeSpec.tv_nsec / 1000000);
    if (g_lpScheduled->qwScheduledTime < qwCurrentTime) {
        /* soonest scheduled packet is already supposed to be sent */
        return 0;
    }
    /* return difference between time now and timestamp of soonest scheduled packet */
    return g_lpScheduled->qwScheduledTime - qwCurrentTime;
}

/*
 * function to be called on unblock of an incoming arp frame
 */
static int unblock_arp(int hSocket) {
    uint32_t dwSourceIp;
    uint32_t dwTargetIp;
    char szTargetIp[16];
    char szSourceIp[16];
    char szSourceMac[18];
    struct ether_arp stRequest, stResponse;
    int dwBytesRecvd;
    interface_info_t stInterfaceInfo;
    struct sockaddr_ll stSockAddr;
    scheduled_packet_t **lpCursor, *lpDelete;
    struct ether_arp *lpPacket;

    /* receive packet */
    dwBytesRecvd = recv(hSocket, &stRequest, sizeof(struct ether_arp), 0);
    if (dwBytesRecvd == -1 && errno != EINTR) {
        goto _error;
    }
    /* sanity check */
    if (
        dwBytesRecvd != sizeof(struct ether_arp) ||
        stRequest.arp_hrd != htons(ARPHRD_ETHER) ||
        stRequest.arp_pro != htons(ETH_P_IP) ||
        stRequest.arp_hln != ETHER_ADDR_LEN ||
        stRequest.arp_pln != sizeof(in_addr_t) ||
        (stRequest.arp_op != htons(ARPOP_REQUEST) && stRequest.arp_op != htons(ARPOP_REPLY)))
    {
        goto _success;
    }
    if (GetInterfaceInfo(&stInterfaceInfo, g_szInterface) != 0) {
        goto _error;
    }

    memcpy(&dwSourceIp, stRequest.arp_spa, sizeof(dwSourceIp));
    memcpy(&dwTargetIp, stRequest.arp_tpa, sizeof(dwTargetIp));

    /* check if packet is within our subnet -- if not, leave it be */
    if ((dwTargetIp & stInterfaceInfo.dwNetmask) != (stInterfaceInfo.dwIpAddr & stInterfaceInfo.dwNetmask)) {
        goto _success;
    }

    /* tell viewers what's going on */
    if (ntohs(stRequest.arp_op) == ARPOP_REQUEST) {
        inet_ntop(AF_INET, stRequest.arp_tpa, szTargetIp, sizeof(szTargetIp));
        inet_ntop(AF_INET, stRequest.arp_spa, szSourceIp, sizeof(szSourceIp));
        fprintf_flush(stderr, "Who has %s? Tell %s\n", szTargetIp, szSourceIp);
    } else if (ntohs(stRequest.arp_op) == ARPOP_REPLY) {
        inet_ntop(AF_INET, stRequest.arp_spa, szSourceIp, sizeof(szSourceIp));
        snprintf(szSourceMac, sizeof(szSourceMac), "%02x:%02x:%02x:%02x:%02x:%02x",
            stRequest.arp_sha[0], stRequest.arp_sha[1], stRequest.arp_sha[2], stRequest.arp_sha[3], stRequest.arp_sha[4], stRequest.arp_sha[5]
        );
        fprintf_flush(stderr, "%s is at %s\n", szSourceIp, szSourceMac);
    }

    if (ntohs(stRequest.arp_op) == ARPOP_REQUEST) {
        /* check if packet was intended for us -- if so, the kernel will process it */
        if (dwSourceIp == stInterfaceInfo.dwIpAddr || dwTargetIp == stInterfaceInfo.dwIpAddr) {
            goto _success;
        }
        /* special socket address type used for AF_PACKET */
        memset(&stSockAddr, 0, sizeof(struct sockaddr_ll));
        stSockAddr.sll_family = AF_PACKET;
        stSockAddr.sll_ifindex = stInterfaceInfo.dwIfIndex;
        stSockAddr.sll_halen = ETHER_ADDR_LEN;
        stSockAddr.sll_protocol = htons(ETH_P_ARP);
        memcpy(stSockAddr.sll_addr, stRequest.arp_sha, ETHER_ADDR_LEN);

        /* construct the ARP response */
        stResponse.arp_hrd = htons(ARPHRD_ETHER);
        stResponse.arp_pro = htons(ETH_P_IP);
        stResponse.arp_hln = ETHER_ADDR_LEN;
        stResponse.arp_pln = sizeof(in_addr_t);
        stResponse.arp_op = htons(ARPOP_REPLY);

        memcpy(stResponse.arp_tha, stRequest.arp_sha, ETHER_ADDR_LEN);
        memcpy(stResponse.arp_tpa, stRequest.arp_spa, sizeof(in_addr_t));
        memcpy(stResponse.arp_sha, stInterfaceInfo.abMacAddress, ETHER_ADDR_LEN);
        memcpy(stResponse.arp_spa, stRequest.arp_tpa, sizeof(in_addr_t));

        /*
         * schedule the packet for emission
         * we wait for a legitimate peer to respond to this message first
         * as we don't want to interfere with normal operation
         */
        schedule_packet(&stResponse, sizeof(struct ether_arp), &stSockAddr);
    } else if (stRequest.arp_op == ARPOP_REPLY) {
        /*
         * detected an arp reply from a peer
         * look through the scheduled packets and cancel everything
         * that would interfere with this
         */
        for (lpCursor = &g_lpScheduled; (*lpCursor) != NULL; lpCursor = &(*lpCursor)->lpNext) {
_next:
            if ((*lpCursor)->stDestSockAddr.sll_protocol != htons(ETH_P_ARP)) {
                /* not an arp packet */
                continue;
            }

            /* cast packet */
            lpPacket = (struct ether_arp *)((*lpCursor)->abBuffer);
            if (
                /* check source and destination ip address */
                memcmp(lpPacket->arp_tpa, stRequest.arp_spa, sizeof(in_addr_t)) == 0 &&
                memcmp(lpPacket->arp_spa, stRequest.arp_tpa, sizeof(in_addr_t)) == 0
            ) {
                /* found a match with a scheduled packet -- cancel it */
                lpDelete = *lpCursor;
                *lpCursor = (*lpCursor)->lpNext;
                free(lpDelete);
                if (*lpCursor == NULL) {
                    /* just deleted the last entry, adjust tail pointer */
                    g_lpScheduledTail = lpCursor;
                    break;
                }
                goto _next; /* cursor is already incremented */
            }
        }
    }

_success:
    return 0;

_error:
    return -1;
}

/*
 * check whether peer A and B are within each other's subnet
 * lpAddrA: address of peer A
 * lpAddrB: address of peer B
 * dwPrefixLen: number of bits of the address determining the subnet
 */
static int is_within_subnet(struct in6_addr *lpAddrA, struct in6_addr *lpAddrB, int dwPrefixLen) {
    int i;

    for (i = 0; i < ((dwPrefixLen + 7) >> 3) && i < 16; i++) {
        if (i < (dwPrefixLen >> 3)) {
            if (((uint8_t *)lpAddrA)[i] != ((uint8_t *)lpAddrB)[i]) {
                return 0;
            }
        }
        if (i == (dwPrefixLen >> 3)) {
            if (
                (((uint8_t *)lpAddrA)[i] & (0xff00 >> (dwPrefixLen >> 7))) !=
                (((uint8_t *)lpAddrB)[i] & (0xff00 >> (dwPrefixLen >> 7)))
            ) {
                return 0;
            }
        }
    }

    return 1;
}

/*
 * icmpv6 checksum functions
 */
#define CKSUM_CARRY(x)   (x = (x >> 16) + (x & 0xffff), (~(x + (x >> 16)) & 0xffff))
static int chksum(const uint16_t *addr, int len) {
    int sum = 0;
    union {
        uint16_t s;
        uint8_t b[2];
    } pad;

    sum = 0;

    while (len > 1) {
        sum += *addr++;
        len -= 2;
    }

    if (len == 1) {
        pad.b[0] = *(uint8_t *)addr;
        pad.b[1] = 0;
        sum += pad.s;
    }

    return (sum);
}

/*
 * function to be called on unblock of an incoming ipv6 frame
 * which may turn out to be icmpv6
 */
static int unblock_ipv6(int hSocket) {
    char szTargetIp[INET6_ADDRSTRLEN];
    char szSourceIp[INET6_ADDRSTRLEN];
    char szSourceMac[18];
    neighbor_packet_t stRequest, stResponse;
    int dwBytesRecvd;
    interface_info_t stInterfaceInfo;
    struct sockaddr_ll stSockAddrRecv, stSockAddrSend;
    socklen_t dwSockAddrSize;
    uint32_t dwChecksum;
    int i;

    dwSockAddrSize = sizeof(stSockAddrRecv);
    dwBytesRecvd = recvfrom(hSocket, &stRequest, sizeof(neighbor_packet_t), 0, (struct sockaddr *)&stSockAddrRecv, &dwSockAddrSize);
    if (dwBytesRecvd != sizeof(neighbor_packet_t)) {
        goto _success;
    }

    // printf("unblock_ipv6\n");

    if (
        /* sanity check */
        stSockAddrRecv.sll_family != AF_PACKET ||
        (ntohl(stRequest.ip6_un1_flow) & 0xf0000000) != 0x60000000 ||
        ntohs(stRequest.ip6_un1_plen) != 32 ||
        stRequest.ip6_un1_nxt != IPPROTO_ICMPV6 ||
        (stRequest.icmp6_type != ND_NEIGHBOR_SOLICIT && stRequest.icmp6_type != ND_NEIGHBOR_ADVERT) ||
        // (stRequest.icmp6_type == ND_NEIGHBOR_SOLICIT && stRequest.bType != 1) ||
        // (stRequest.icmp6_type == ND_NEIGHBOR_ADVERT && stRequest.bType != 2) ||
        (GetInterfaceInfo(&stInterfaceInfo, g_szInterface) != 0) ||
        !is_within_subnet(&stRequest.abTargetAddr, &stInterfaceInfo.abIpv6Addr, stInterfaceInfo.dwIpv6PrefixLen)
    ) {
        goto _success;
    }

    snprintf(szSourceMac, sizeof(szSourceMac), "%02x:%02x:%02x:%02x:%02x:%02x",
        stSockAddrRecv.sll_addr[0], stSockAddrRecv.sll_addr[1], stSockAddrRecv.sll_addr[2],
        stSockAddrRecv.sll_addr[3], stSockAddrRecv.sll_addr[4], stSockAddrRecv.sll_addr[5]
    );

    /* tell the viewers what's going on */
    if (stRequest.icmp6_type == ND_NEIGHBOR_SOLICIT) {
        inet_ntop(AF_INET6, &stRequest.abTargetAddr, szTargetIp, INET6_ADDRSTRLEN);
        fprintf_flush(stderr, "Neighbor Solicitation for %s from %s\n", szTargetIp, szSourceMac);
    } else {
        inet_ntop(AF_INET6, &stRequest.abTargetAddr, szTargetIp, INET6_ADDRSTRLEN);
        fprintf_flush(stderr, "Neighbor Advertisement %s is at %s\n", szTargetIp, szSourceMac);
    }

    /* neighbor solicitation (analogous to arp request) */
    if (stRequest.icmp6_type == ND_NEIGHBOR_SOLICIT) {
        /* check if target message is intended for us -- if so, the kernel will process it */
        if (
            memcmp(&stRequest.abTargetAddr, &stInterfaceInfo.abIpv6Addr, sizeof(struct in6_addr)) == 0 ||
            memcmp(&stRequest.ip6_src, &stInterfaceInfo.abIpv6Addr, sizeof(struct in6_addr)) == 0
        ) {
            goto _success;
        }
        /*
         * check if the source is :: -- most likely a bootstrap duplicate check
         * we can't reply to it anyway without implementing multicast
         */
        memset(&stResponse.ip6_src, 0, sizeof(struct in6_addr));
        if (memcmp(&stResponse.ip6_src, &stRequest.ip6_src, sizeof(struct in6_addr)) == 0) {
            goto _success;
        }

        /* construct destinatin address struct */
        memset(&stSockAddrSend, 0, sizeof(struct sockaddr_ll));
        stSockAddrSend.sll_family = AF_PACKET;
        stSockAddrSend.sll_ifindex = stInterfaceInfo.dwIfIndex;
        stSockAddrSend.sll_halen = ETHER_ADDR_LEN;
        stSockAddrSend.sll_protocol = htons(ETH_P_IPV6);
        memcpy(stSockAddrSend.sll_addr, stSockAddrRecv.sll_addr, ETHER_ADDR_LEN);

        /* construct ipv6 header */
        memset(&stResponse, 0, sizeof(neighbor_packet_t));
        stResponse.ip6_un1_flow = htonl(0x60000000);
        stResponse.ip6_un1_plen = htons(32);
        stResponse.ip6_un1_nxt = IPPROTO_ICMPV6;
        stResponse.ip6_un1_hlim = 255;
        memcpy(&stResponse.ip6_src, &stInterfaceInfo.abIpv6Addr, sizeof(struct in6_addr));
        memcpy(&stResponse.ip6_dst, &stRequest.ip6_src, sizeof(struct in6_addr));

        /* construct icmpv6 header */
        stResponse.icmp6_type = ND_NEIGHBOR_ADVERT;
        stResponse.icmp6_code = 0;
        stResponse.icmp6_cksum = 0;
        stResponse.icmp6_un_data8[0] = 0xe0;
        memcpy(&stResponse.abTargetAddr, &stRequest.abTargetAddr, sizeof(struct in6_addr));
        stResponse.bType = 2;
        stResponse.bLength = 1;
        memcpy(stResponse.abMacAddress, stInterfaceInfo.abMacAddress, ETHER_ADDR_LEN);

        /* generate icmpv6 checksum */
        dwChecksum = chksum((uint16_t *)&stResponse.ip6_src, sizeof(struct in6_addr));
        dwChecksum += chksum((uint16_t *)&stResponse.ip6_dst, sizeof(struct in6_addr));
        dwChecksum += htons(IPPROTO_ICMPV6);
        dwChecksum += htons(32);
        dwChecksum += chksum((uint16_t *)&stResponse.icmp6_type, 32);
        stResponse.icmp6_cksum = CKSUM_CARRY(dwChecksum);

        /*
         * schedule the packet for emission
         * we wait for a legitimate peer to respond to this message first
         * as we don't want to interfere with normal operation
         */
        schedule_packet(&stResponse, sizeof(neighbor_packet_t), &stSockAddrSend);
    } else { /* ND_NEIGHBOR_ADVERT */
        scheduled_packet_t **lpCursor, *lpDelete;
        neighbor_packet_t *lpPacket;

        /*
         * detected a neighbor advertisement from a peer
         * look through the scheduled packets and cancel everything
         * that would interfere with this
         */
        for (lpCursor = &g_lpScheduled; (*lpCursor) != NULL; lpCursor = &(*lpCursor)->lpNext) {
_next:
            if ((*lpCursor)->stDestSockAddr.sll_protocol != ETH_P_IPV6) {
                /* not an ipv6 packet */
                continue;
            }

            /* cast packet */
            lpPacket = (neighbor_packet_t *)((*lpCursor)->abBuffer);
            if (
                /* check source and destination ip address */
                memcmp(&lpPacket->abTargetAddr, &stRequest.abTargetAddr, sizeof(struct in6_addr)) == 0 &&
                memcmp(&lpPacket->ip6_dst, &stRequest.ip6_src, sizeof(struct in6_addr)) == 0
            ) {
                /* found a match with a scheduled packet -- cancel it */
                lpDelete = *lpCursor;
                *lpCursor = (*lpCursor)->lpNext;
                free(lpDelete);
                if (*lpCursor == NULL) {
                    /* just deleted the last entry, adjust tail pointer */
                    g_lpScheduledTail = lpCursor;
                    break;
                }
                goto _next; /* cursor is already incremented */
            }
        }
    }

_success:
    return 0;

_error:
    return -1;
}

/* 
 * main
 */
int main(int argc, char *argv[]) {
    struct sockaddr_ll stSockAddr;
    int hSocket = -1;
    int hSocketIpv6 = -1;
    struct pollfd astPollFds[2];
    interface_info_t stInterfaceInfo;
    struct timespec stTimeSpec;
    long long qwCurrentTime;
    scheduled_packet_t **lpCursor, *lpDelete;
    neighbor_packet_t *lpPacket;
    struct in6_addr abZeroAddr;

    if (argc < 2) {
        fprintf_flush(stderr, "[i] usage: %s <interface>\n", argv[0]);
        return 0;
    }
    g_szInterface = argv[1];

    if (GetInterfaceInfo(&stInterfaceInfo, g_szInterface) != 0) {
        goto _error;
    }

    if (stInterfaceInfo.dwIpAddr != 0) {
        /* create raw socket for arp packets */
        if ((hSocket = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP))) == -1) {
            fprintf_flush(stderr, "[-] unable to create raw socket (ETH_P_ARP): %s\n", strerror(errno));
            goto _error;
        }

        /* bind socket to interface */
        memset(&stSockAddr, 0, sizeof(struct sockaddr_ll));
        stSockAddr.sll_family = AF_PACKET;
        stSockAddr.sll_ifindex = stInterfaceInfo.dwIfIndex;
        stSockAddr.sll_protocol = htons(ETH_P_ARP);
        if (bind(hSocket, (struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_ll)) == -1) {
            fprintf_flush(stderr, "[-] unable to bind raw socket: %s\n", strerror(errno));
            goto _error;
        }
    }
    memset(&abZeroAddr, 0, sizeof(struct in6_addr));
    if (memcmp(&abZeroAddr, &stInterfaceInfo.abIpv6Addr, sizeof(struct in6_addr)) != 0) {
        /* create raw socket for icmpv6 packets */
        if ((hSocketIpv6 = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6))) == -1) {
            fprintf_flush(stderr, "[-] unable to create raw socket (ETH_P_IPV6): %s\n", strerror(errno));
            goto _error;
        }

        /* bind ipv6 socket to interface */
        memset(&stSockAddr, 0, sizeof(struct sockaddr_ll));
        stSockAddr.sll_family = AF_PACKET;
        stSockAddr.sll_ifindex = stInterfaceInfo.dwIfIndex;
        stSockAddr.sll_protocol = htons(ETH_P_IPV6);
        if (bind(hSocketIpv6, (struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_ll)) == -1) {
            fprintf_flush(stderr, "[-] unable to bind raw socket: %s\n", strerror(errno));
            goto _error;
        }
    }

    if (hSocket == -1 && hSocketIpv6 == -1) {
        fprintf_flush(stderr, "[-] neither ipv4 nor ipv6 address set for interface %s, nothing to do\n", g_szInterface);
        goto _error;
    }

    fprintf_flush(stderr, "[i] arpresponder running\n");

    /* main loop */
    for(;;) {
        memset(&astPollFds, 0, sizeof(astPollFds));
        astPollFds[0].events = POLLIN;
        astPollFds[0].fd = hSocket;
        astPollFds[1].events = POLLIN;
        astPollFds[1].fd = hSocketIpv6;

        /* if ipv6 socket is not open, only concern ourselves with arp */
        if (poll(&astPollFds[(hSocket == -1)], 2 - (hSocketIpv6 == -1) - (hSocket == -1), schedule_get_soonest()) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EINTR) {
                continue;
            }
        }

        /* check if arp socket unblocked */
        if (astPollFds[0].revents & (POLLIN | POLLERR | POLLNVAL | POLLHUP)) {
            if (unblock_arp(hSocket) == -1) {
                goto _error;
            }
        }

        /* check if ipv6 socket unblocked */
        if (astPollFds[1].revents & (POLLIN | POLLERR | POLLNVAL | POLLHUP)) {
            if (unblock_ipv6(hSocketIpv6) == -1) {
                goto _error;
            }
        }

        /* send scheduled packets */
        if (clock_gettime(CLOCK_MONOTONIC, &stTimeSpec) == -1) {
            stTimeSpec.tv_sec = 0;
            stTimeSpec.tv_nsec = 0;
        }

        /* convert to millis */
        qwCurrentTime = stTimeSpec.tv_sec * 1000 + (stTimeSpec.tv_nsec / 1000000);
        /* loop through scheduled packets */
        for (lpCursor = &g_lpScheduled; (*lpCursor) != NULL; lpCursor = &(*lpCursor)->lpNext) {
_next:
            if ((*lpCursor)->qwScheduledTime <= qwCurrentTime) {
                /* packet scheduled to be sent now -> send via appropriate socket */
                if (sendto(
                    ((*lpCursor)->stDestSockAddr.sll_protocol == htons(ETH_P_ARP)) ? hSocket : hSocketIpv6,
                    (*lpCursor)->abBuffer, (*lpCursor)->dwSize, 0,
                    (struct sockaddr *) &(*lpCursor)->stDestSockAddr, sizeof(struct sockaddr_ll)
                ) == -1) {
                    fprintf_flush(stderr, "[-] unable to send raw packet: %s\n", strerror(errno));
                } else {
                    if ((*lpCursor)->stDestSockAddr.sll_protocol == htons(ETH_P_ARP)) {
                        /* tell our viewers that a soofed arp reply just went out */
                        char szSourceIp[16];
                        char szSourceMac[18];
                        struct ether_arp *lpPacket = (struct ether_arp *)((*lpCursor)->abBuffer);
                        inet_ntop(AF_INET, lpPacket->arp_spa, szSourceIp, sizeof(szSourceIp));
                        snprintf(szSourceMac, sizeof(szSourceMac), "%02x:%02x:%02x:%02x:%02x:%02x",
                            lpPacket->arp_sha[0], lpPacket->arp_sha[1], lpPacket->arp_sha[2],
                            lpPacket->arp_sha[3], lpPacket->arp_sha[4], lpPacket->arp_sha[5]
                        );
                        fprintf_flush(stderr, "%s is at %s (spoofed)\n", szSourceIp, szSourceMac);
                    } else {
                        /* tell our viewers that a spoofed neighbor advertisement just went out */
                        char szSourceIp[INET6_ADDRSTRLEN];
                        char szSourceMac[18];
                        neighbor_packet_t *lpPacket = (neighbor_packet_t *)((*lpCursor)->abBuffer);
                        inet_ntop(AF_INET6, &lpPacket->abTargetAddr, szSourceIp, INET6_ADDRSTRLEN);
                        snprintf(szSourceMac, sizeof(szSourceMac), "%02x:%02x:%02x:%02x:%02x:%02x",
                            lpPacket->abMacAddress[0], lpPacket->abMacAddress[1], lpPacket->abMacAddress[2],
                            lpPacket->abMacAddress[3], lpPacket->abMacAddress[4], lpPacket->abMacAddress[5]
                        );
                        fprintf_flush(stderr, "Neighbor Advertisement %s is at %s (spoofed)\n", szSourceIp, szSourceMac);
                    }
                }

                /* delete linked list entry */
                lpDelete = *lpCursor;
                *lpCursor = (*lpCursor)->lpNext;
                free(lpDelete);
                if (*lpCursor == NULL) {
                    /* just deleted the last entry, adjust tail pointer */
                    g_lpScheduledTail = lpCursor;
                    break;
                }
                goto _next; /* cursor is already incremented */
            } else {
                break;
            }
        }
    }

_error:
    if (hSocket != -1) { close(hSocket); }
    if (hSocketIpv6 != -1) { close(hSocketIpv6); }
    return -1;
}
