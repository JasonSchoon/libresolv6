#ifndef __RESOLV6_H_
#define __RESOLV6_H_

#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    RESOLVE_ANY,
    RESOLVE_IPV4_ONLY,
    RESOLVE_IPV6_ONLY
};    

extern int ResolveAddress(const char *DomainName, unsigned AllowedAddress, int SkipResolver, struct in6_addr *resultIP);

#ifdef __cplusplus
}
#endif

#endif
