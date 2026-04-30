#include "GameServer.h"

#define TCP_PORT 4300
#define UDP_PORT 4301

int main()
{
    auto server = GameServer(TCP_PORT, UDP_PORT);

    server.start();

    return 0;
}