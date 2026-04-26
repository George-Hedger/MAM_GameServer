//
// Created by george on 26/04/2026.
//

#ifndef MAM_GAMESERVER_GAMESERVER_H
#define MAM_GAMESERVER_GAMESERVER_H
#include "NetworkManager.h"


class GameServer {

public:
    GameServer(unsigned short tcp_port, unsigned short udp_port) : net(tcp_port, udp_port){};

    void start();

private:
    NetworkManager net;

    void login_loop();
    bool validate_name(const std::string& name, const unsigned short &id) const;

    void generate_world();

    std::unordered_map<unsigned short, std::string> player_names;
};



#endif //MAM_GAMESERVER_GAMESERVER_H
