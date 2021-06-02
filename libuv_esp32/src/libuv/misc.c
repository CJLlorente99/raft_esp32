#include "uv.h"

int
uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr){
    addr->sin_addr.s_addr = inet_addr(ip);
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    return 0;
}