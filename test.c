#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resolv6.h"

#define TEST_PORT 64500
const char junk[] = "TESTING_RESOLV6_API";
const char *testcases[] = {"192.168.100.1", "fe80::201:2ff:fe6b:7520", "localhost", "127.0.0.1", "::1", "google.com"};
#define NUM_TESTCASES (sizeof(testcases) / sizeof(testcases[0]))

int main(void)
{
    int sock;
    unsigned i;
    struct sockaddr_in6 sin6;
    struct in6_addr in6;

    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(TEST_PORT);

    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
	perror("socket");
        return EXIT_FAILURE;
    }

    for (i = 0; i < NUM_TESTCASES; i++)
    {
        int ret = ResolveAddress(testcases[i], RESOLVE_ANY, 0, &in6);
        fprintf(stderr, "Resolving test case %d(%s): %s\n", i, testcases[i], ret ? "successful" : "unsuccessful");
        if (ret)
        {
            memcpy(sin6.sin6_addr.s6_addr, in6.s6_addr, sizeof(in6));
            (void)sendto(sock, junk, sizeof(junk) - 1, 0, (struct sockaddr *)(void *)&sin6, sizeof(sin6)); 
        }
    }
    
    return EXIT_SUCCESS;
}
