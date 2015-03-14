This is a an API to simplify creating code that can talk to either IPv4 or IPv6 hosts.  This seems like something that shouldn't be that hard, and it isn't, if you use this API.

This API leverages heavily on the existing functions that are built into glibc and most other standard libraries for resolving.  inet\_pton, gethostbyname, and gethostbyname2 are some of the more important prerequisites.