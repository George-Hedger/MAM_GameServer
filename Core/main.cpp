#include "GameServer.h"

int main()
{
    auto server = GameServer(4300, 4301);

    server.start();

    return 0;
}