#include "GameServer.h"

#include <thread>

int main()
{
    GameServer server(4300, 4301);
    std::thread udp_thread(&GameServer::udp_start, &server);
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Not strictly necessary, just helps with the output
    server.tcp_start();
    udp_thread.join();
    return 0;
}