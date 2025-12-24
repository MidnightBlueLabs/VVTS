#include <net/if.h>
#include <net/if_arp.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <errno.h>

#include "common.h"

static int g_hRawSocket = -1;
static int g_hNetlinkSocket = -1;

#define for_each_nlmsg(n, buf, len)					\
	for (n = (struct nlmsghdr*)buf;					\
	     NLMSG_OK(n, (uint32_t)len) && n->nlmsg_type != NLMSG_DONE;	\
	     n = NLMSG_NEXT(n, len))

#define for_each_rattr(n, buf, len)					\
	for (n = (struct rtattr*)buf; RTA_OK(n, len); n = RTA_NEXT(n, len))

int GetInterfaceInfo(interface_info_t *lpInterfaceInfo, const char *szInterface) {
    struct ifreq stInterfaceRequest;
    int dwStatus, dwIpv6Status;
    int dwSize;
    struct sockaddr_nl stNetlinkSockAddr;
    unsigned char abNetlinkBuf[4096];
    struct nlmsghdr *lpNetlinkMsg;
    struct ifaddrmsg *lpIfAddr;
    struct iovec stIoVector;
    struct msghdr stNetlinkMsg;
    struct rtattr *lpRta;

    memset(&stInterfaceRequest, 0, sizeof(struct ifreq));
    snprintf(stInterfaceRequest.ifr_name, sizeof(stInterfaceRequest.ifr_name), "%s", szInterface);

    if (g_hRawSocket == -1 && (g_hRawSocket = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP))) == -1) {
        fprintf_flush(stderr, "[-] unable to create raw socket: %s\n", strerror(errno));
        dwStatus = g_hRawSocket;
        goto _error;
    }

    if (
        (dwStatus = ioctl(g_hRawSocket, SIOCGIFHWADDR, &stInterfaceRequest)) == -1 ||
        (stInterfaceRequest.ifr_hwaddr.sa_family != ARPHRD_ETHER && (dwStatus = -1))
    ) {
        fprintf_flush(stderr, "[-] unable to obtain mac address for interface %s\n", szInterface);
        goto _error;
    }
    memcpy(lpInterfaceInfo->abMacAddress, stInterfaceRequest.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);

    if (
        ioctl(g_hRawSocket, SIOCGIFADDR, &stInterfaceRequest) != -1 &&
        stInterfaceRequest.ifr_addr.sa_family == AF_INET
    ) {
        memcpy(&lpInterfaceInfo->dwIpAddr, &((struct sockaddr_in *)&stInterfaceRequest.ifr_addr)->sin_addr.s_addr, sizeof(in_addr_t));
    } else {
        lpInterfaceInfo->dwIpAddr = 0;
    }

    if (
        ioctl(g_hRawSocket, SIOCGIFNETMASK, &stInterfaceRequest) != -1 &&
        stInterfaceRequest.ifr_addr.sa_family == AF_INET
    ) {
        memcpy(&lpInterfaceInfo->dwNetmask, &((struct sockaddr_in *)&stInterfaceRequest.ifr_addr)->sin_addr.s_addr, sizeof(in_addr_t));
    } else {
        lpInterfaceInfo->dwNetmask = 0;
    }

    if ((dwStatus = ioctl(g_hRawSocket, SIOCGIFINDEX, &stInterfaceRequest)) == -1) {
        fprintf_flush(stderr, "[-] unable to obtain ifindex for interface %s\n", szInterface);
        goto _error;
    }
    lpInterfaceInfo->dwIfIndex = stInterfaceRequest.ifr_ifindex;
    dwStatus = 0;

    /*
     * please take the time to admire the sad state of affairs of
     * how one is supposed to retrieve the ipv6 address for a network interface on linux.
     */

    memset(&lpInterfaceInfo->abIpv6Addr, 0, sizeof(struct in6_addr));
    lpInterfaceInfo->dwIpv6PrefixLen = 0;

    if (g_hNetlinkSocket == -1 && (g_hNetlinkSocket = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) {
        fprintf_flush(stderr, "[-] unable to create netlink socket: %s\n", strerror(errno));
        dwIpv6Status = -1;
        goto _error;
    }

	memset(&stNetlinkSockAddr, 0, sizeof(stNetlinkSockAddr));
	stNetlinkSockAddr.nl_family = AF_NETLINK;

	memset(abNetlinkBuf, 0, sizeof(abNetlinkBuf));

	// assemble the message according to the netlink protocol
	lpNetlinkMsg = (struct nlmsghdr *)abNetlinkBuf;
	lpNetlinkMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	lpNetlinkMsg->nlmsg_type = RTM_GETADDR;
	lpNetlinkMsg->nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;

	lpIfAddr = (struct ifaddrmsg *)NLMSG_DATA(lpNetlinkMsg);
	lpIfAddr->ifa_family = AF_INET6;

	// prepare struct msghdr for sending.
	stIoVector.iov_base = lpNetlinkMsg;
    stIoVector.iov_len = lpNetlinkMsg->nlmsg_len;
	
    stNetlinkMsg.msg_name = &stNetlinkSockAddr,
    stNetlinkMsg.msg_namelen = sizeof(stNetlinkSockAddr),
    stNetlinkMsg.msg_iov = &stIoVector,
    stNetlinkMsg.msg_iovlen = 1;
    stNetlinkMsg.msg_control = NULL;
    stNetlinkMsg.msg_controllen = 0;
    stNetlinkMsg.msg_flags = 0;

	// send netlink message to kernel.
	dwIpv6Status = sendmsg(g_hNetlinkSocket, &stNetlinkMsg, 0);
    if (dwIpv6Status == -1) {
        goto _error;
    }

    do {
        stIoVector.iov_base = abNetlinkBuf;
        stIoVector.iov_len = sizeof(abNetlinkBuf);

        memset(&stNetlinkMsg, 0, sizeof(stNetlinkMsg));
        stNetlinkMsg.msg_name = &stNetlinkSockAddr;
        stNetlinkMsg.msg_namelen = sizeof(stNetlinkSockAddr);
        stNetlinkMsg.msg_iov = &stIoVector;
        stNetlinkMsg.msg_iovlen = 1;

        dwIpv6Status = recvmsg(g_hNetlinkSocket, &stNetlinkMsg, 0);

        if (dwIpv6Status == -1) {
            goto _error;
        }

        for_each_nlmsg(lpNetlinkMsg, abNetlinkBuf, dwIpv6Status) {
            if (lpNetlinkMsg->nlmsg_type == NLMSG_ERROR) {
                dwIpv6Status = -1;
                goto _error;
            }

            if (lpNetlinkMsg->nlmsg_type == RTM_NEWADDR) {
                lpIfAddr = (struct ifaddrmsg *)NLMSG_DATA(lpNetlinkMsg);
                if (lpIfAddr->ifa_index != lpInterfaceInfo->dwIfIndex) {
                    continue;
                }

                dwSize = IFA_PAYLOAD(lpNetlinkMsg);
                for_each_rattr(lpRta, IFA_RTA(lpIfAddr), dwSize) {
                    if (lpRta->rta_type == IFA_ADDRESS) {
                        if (
                            ((unsigned char *)RTA_DATA(lpRta))[0] == 0xfe &&
                            ((unsigned char *)RTA_DATA(lpRta))[1] == 0x80
                        ) {
                            continue;
                        }
                        memcpy(&lpInterfaceInfo->abIpv6Addr, RTA_DATA(lpRta), sizeof(struct in6_addr));
                        lpInterfaceInfo->dwIpv6PrefixLen = lpIfAddr->ifa_prefixlen;
                        dwIpv6Status = 0;
                        goto _success;
                    }
                }
            }
        }
    } while (lpNetlinkMsg->nlmsg_type != NLMSG_DONE && lpNetlinkMsg->nlmsg_type != NLMSG_ERROR);

_success:
_error:
    return dwStatus;
}
