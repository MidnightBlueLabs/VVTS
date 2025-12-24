#ifndef __HAVE_COMMON_H
#define __HAVE_COMMON_H

#include <net/ethernet.h>
#include <netinet/in.h>

#define fprintf_flush(stream, ...) do { \
    fprintf(stream, __VA_ARGS__); \
    fflush(stream); \
} while(0);

typedef struct {
    uint8_t abMacAddress[ETHER_ADDR_LEN];
    in_addr_t dwIpAddr;
    in_addr_t dwNetmask;
    struct in6_addr abIpv6Addr;
    int dwIpv6PrefixLen;
    int dwIfIndex;
} interface_info_t;

int GetInterfaceInfo(interface_info_t *lpInterfaceInfo, const char *szInterface);

#endif /* __HAVE_COMMON_H */