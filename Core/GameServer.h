//
// Created by george on 26/04/2026.
//

#ifndef MAM_GAMESERVER_GAMESERVER_H
#define MAM_GAMESERVER_GAMESERVER_H
#include "NetworkManager.h"

class GameServer {

public:
    GameServer(const unsigned short tcp_port, const unsigned short udp_port) : net(tcp_port, udp_port){};

    void start();

private:
    NetworkManager net;

    static constexpr int8_t m_max_players{1};

    static constexpr int8_t map_x{32};
    static constexpr int8_t map_y{64};

    void login_loop();
    bool validate_name(const std::string& name, const int8_t &id) const;

    void generate_world();

    int8_t map[map_y][map_x]{-2};
    std::unordered_map<int8_t, class Entity*> entities;

    std::unordered_map<int8_t, std::string> player_names;

    void game_loop();
};



#endif //MAM_GAMESERVER_GAMESERVER_H
