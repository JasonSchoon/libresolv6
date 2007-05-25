/* Borrowed from the libresolv6 project */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#ifdef RESOLVE_DEBUG
/* Max length for FQDN + NULL, as opposed to maximum hostname (MAXHOSTNAMELEN=64) */
#define MAX_HOSTLEN 256
#include <errno.h>
#endif
#include <arpa/inet.h>
#include "resolv6.h"

static int IsAddressIPv4(char *addressString)
{
    char *start, *token;
    unsigned octet, goodOctets = 0;
  
    // Check for something that looks like x.x.x.x, where x is a number
    start = addressString;
    for (octet = 0; octet < 4; octet++)
    {
        if (isdigit(start[0]))
        {
            if (octet < 3)
            {
                token = strchr(start, '.');
                if (token)
                {
                    start = token+1;
                    goodOctets++;
                }
            }
            else
                goodOctets++;
        }
    }

    return (goodOctets == 4);
}

static int ip6_atoi(struct in6_addr *ip, const char *addrString)
{
    if (addrString == NULL || (strlen((char *) addrString) < 2))
    {
        memcpy(ip->s6_addr, &in6addr_any, sizeof(struct in6_addr));
        return 1;
    }
    else if (IsAddressIPv4((char *)addrString))
    {
        char buf[INET6_ADDRSTRLEN];
        strncpy(buf, (char *)addrString, sizeof(buf) - 1);
        // TODO - Cleanup
        {
            char tempAddress[INET6_ADDRSTRLEN];
    
            if (IsAddressIPv4(buf))
            {
                // Prepend the IPv4 prefix
                strncpy(tempAddress, buf, INET6_ADDRSTRLEN);
                (void)snprintf(buf, 64, "%s%s", "::ffff:", tempAddress);
            }
        }
        return (inet_pton(AF_INET6, (char *)buf, ip->s6_addr) > 0) ? 1 : 0;
    }
    else
        return (inet_pton(AF_INET6, (char *) addrString, ip->s6_addr) > 0) ? 1 : 0;
}

void MapIPv4ToIPv6(uint32_t v4address, struct in6_addr *v6address)
{
    memset(v6address, 0, sizeof(struct in6_addr));
    v4address = htonl(v4address);
    ((uint16_t *)v6address->s6_addr)[5] = 0xFFFF;
    memcpy(&v6address->s6_addr[12], &v4address, sizeof(v4address));
}

static int _ResolveAddressImmediate(const char *DomainName, unsigned AllowedAddress, struct in6_addr *resultIP)
{
    uint32_t IPv4Address;
    struct in6_addr IPv6Address;
    char IPbuffer[INET6_ADDRSTRLEN];
    
    // Look for a literal IPv4 address
    if (inet_pton(AF_INET, DomainName, &IPv4Address) > 0) 
    {
        if (AllowedAddress == RESOLVE_IPV6_ONLY)
        {
            #ifdef RESOLVE_DEBUG
            fprintf(stderr, "IPv4 literal address found in IPv6 only mode.\n");
            #endif
            return 0;
        }
        if (inet_ntop(AF_INET, &IPv4Address, IPbuffer, sizeof(IPbuffer)))
        {
            #ifdef RESOLVE_DEBUG
            fprintf(stderr, "IPv4 literal address found (%s).\n", IPbuffer);
            #endif
        }
      
        MapIPv4ToIPv6(htonl(IPv4Address), resultIP);
        return 1;
    }

    // Look for a literal IPv6 address
    memset(IPv6Address.s6_addr, 0, sizeof(struct in6_addr));
    if (ip6_atoi(&IPv6Address, DomainName))
    {
        if (AllowedAddress== RESOLVE_IPV4_ONLY)
        {
            #ifdef RESOLVE_DEBUG
            fprintf(stderr, "IPv6 literal address found in IPv4 only mode.\n");
            #endif
            return 0;
        }
      
        memcpy(resultIP->s6_addr, IPv6Address.s6_addr, sizeof(struct in6_addr));
        return 1;
    }

    #ifdef RESOLVE_DEBUG
    if (strlen(DomainName) >= MAX_HOSTLEN) 
    {
        fprintf(stderr, "Domain name is too long.\n");
    }
    #endif

    return 0;
}

static int _ResolveAddress(const char *DomainName, unsigned AllowedAddress, struct in6_addr *resultIP)
{
    #ifdef RESOLVE_DEBUG
    char *err = NULL;
    #endif
    int ResolveError;
    struct hostent *host;

    if (AllowedAddress == RESOLVE_ANY) 
        host = gethostbyname(DomainName);
    else
        host = gethostbyname2(DomainName, AllowedAddress == RESOLVE_IPV4_ONLY ? AF_INET : AF_INET6);
    if (host)
    {
        if (host->h_addrtype == AF_INET)
        {
            MapIPv4ToIPv6(htonl(*((uint32_t *)*host->h_addr_list)), resultIP);
            return 1;
        }
        else if (host->h_addrtype == AF_INET6)
        {
            memcpy(resultIP->s6_addr, ((struct in6_addr *)*host->h_addr_list), sizeof(struct in6_addr));
            return 1;            
        }
    }
    else
    {
        ResolveError = h_errno;

        #ifdef RESOLVE_DEBUG
        switch (ResolveError)
        {
            default:
            case NETDB_INTERNAL:
                err = strerror(errno);
                break;
            case NETDB_SUCCESS:
                break;
            case HOST_NOT_FOUND:
                err = "Authoritative Answer Host not found.";
                break;
            case TRY_AGAIN:
                err = "Non-Authoritative Host not found, or SERVERFAIL.";
                break;
            case NO_RECOVERY:
                err = "Non recoverable errors, FORMERR, REFUSED, NOTIMP.";
                break;
            case NO_DATA:
                err = "Valid name, no data record of requested type."; 
                break;
        }
                        
        fprintf(stderr, "lookup failed: %s\n", err);
        #endif

        if (ResolveError == HOST_NOT_FOUND)
        {
            #ifdef RESOLVE_DEBUG
            fprintf(stderr, "performing simple lookup...\n");
            #endif
            if (_ResolveAddressImmediate(DomainName, AllowedAddress, resultIP))
                return 1;
        }
    }

    return 0;
}

// Assumes that resultIP points to a valid struct in6_addr buffer
int ResolveAddress(const char *DomainName, unsigned AllowedAddress, int SkipResolver, struct in6_addr *resultIP)
{
    if (!DomainName || !DomainName[0])
        return 0;
    
    #ifdef RESOLVE_DEBUG
    fprintf(stderr, "Searching for %s\n", DomainName);
    #endif

    /* Do a fixup here for 'localhost', even though they shouldn't specify SkipResolver if they want it */
    if (SkipResolver && strcmp(DomainName, "localhost")) 
        return _ResolveAddressImmediate(DomainName, AllowedAddress, resultIP);
    else
        return _ResolveAddress(DomainName, AllowedAddress, resultIP);
}
