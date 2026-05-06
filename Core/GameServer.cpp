//
// Created by george on 26/04/2026.
//

#include "GameServer.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include "mazegen.hpp"

#include "Entity.h"

void GameServer::start()
{
    net.start();
    login_loop();
}

void GameServer::login_loop()
{
    net.set_accept_new_client(true);

    while (player_names.size() < m_max_players)
    {
        RawMessage raw = net.await_next_message();

        TRY_DESERIALIZE(raw, RegisterMessage)
        {
            std::cout << "Player: " << raw.id << " requests name: " << message->chosen_player_name << std::endl;

            if (!validate_name(message->chosen_player_name, raw.id))
            {
                std::cout << "Invalid name request" << std::endl;

                net.tcp_message_id(new ErrorMessage{100, "Invalid Name"}, raw.id);
            }
            else
            {
                std::cout << "Name Accepted!" << std::endl;

                net.tcp_message_id(new SuccessMessage{0}, raw.id);
                player_names.emplace(raw.id, message->chosen_player_name);

                net.tcp_message_all(new InfoMessage{static_cast<int8_t>(player_names.size()), "CurrentPlayers"},  raw.id);
            }
        }
        else TRY_DESERIALIZE(raw, InfoMessage)
        {
            if (message->details == "MaxPlayerCount")
            {
                net.tcp_message_id(new InfoMessage{m_max_players, "MaxPlayerCount"}, raw.id);
            }
            else if (message->details == "CurrentPlayers")
            {
                net.tcp_message_id(new InfoMessage{static_cast<int8_t>(player_names.size()), "CurrentPlayers"}, raw.id);
            }
        }
        else
        {
            std::cerr << "No use for message" << std::endl;
            std::cerr << raw.stream.str() << std::endl;
        }
    }

    net.set_accept_new_client(false);

    generate_world();
}

bool GameServer::validate_name(const std::string &name, const int8_t &id) const
{
    return std::all_of(player_names.begin(), player_names.end(), [&name, &id](const std::pair<int8_t, std::string> &pair)
    {
        if (pair.second == name || pair.first == id)
            return false;

        return true;
    });
}

void GameServer::generate_world()
{
    std::cout << "Generating world..." << std::endl;

    net.tcp_message_all(new RegisterMessage{"Loading Entities..."});
    net.tcp_message_all(new InfoMessage{map_x, "MapX"});
    net.tcp_message_all(new InfoMessage{map_y, "MapY"});

    int success = 0;

    while (success < m_max_players)
    {
        RawMessage raw = net.await_next_message();

        TRY_DESERIALIZE(raw, SuccessMessage)
        {
            if (message->code == 1)
            {
                success++;
            }
        }
        else TRY_DESERIALIZE(raw, InfoMessage)
        {
            if (message->details == "MaxPlayerCount")
            {
                net.tcp_message_id(new InfoMessage{m_max_players, "MaxPlayerCount"}, raw.id);
            }
            else if (message->details == "CurrentPlayers")
            {
                net.tcp_message_id(new InfoMessage{static_cast<int8_t>(player_names.size()), "CurrentPlayers"}, raw.id);
            }
        }
        else
        {
            std::cerr << "No use for message" << std::endl;
            std::cerr << raw.stream.str() << std::endl;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //Create players
    for (const auto& [id, name] : player_names)
    {
        entities.emplace(id, new Entity());
        turn_order.emplace_back(id);
        net.tcp_message_all(new NewEntityMessage(id, DEFAULT));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    net.tcp_message_all(new RegisterMessage{"Loading Map..."});

    //Create map
    mazegen::Config cfg;
    cfg.ROOM_BASE_NUMBER = 25;
    cfg.ROOM_SIZE_MIN = 3;
    cfg.ROOM_SIZE_MAX = 7;
    cfg.EXTRA_CONNECTION_CHANCE = 0.2;
    cfg.WIGGLE_CHANCE = 0.5;
    cfg.DEADEND_CHANCE = 0.5;
    cfg.RECONNECT_DEADENDS_CHANCE = 0.5;
    cfg.CONSTRAIN_HALL_ONLY = true;

    auto gen = mazegen::Generator();
    gen.set_seed(random());
    gen.generate(map_x, map_y, cfg);

    if (!gen.get_warnings().empty()) {
        std::cout << gen.get_warnings() << std::endl;
    }

    for (int y = 0; y < map_y; y++) {
        for (int x = 0; x < map_x; x++) {
            int region = gen.region_at(x, y);
            if (region == mazegen::NOTHING_ID) {
                std::cout << "██";
                map[y][x] = -2;
            } else if (mazegen::is_door(region)){
                // print doors
                std::cout << "▒▒";
                map[y][x] = -1;
            } else {
                // for rooms and halls we just print last 2 digits of their ids
                std::cout << std::setw(2) << region % 100;
                map[y][x] = -1;
            }
        }
        std::cout << std::endl;
    }

    //Place entities in free spots
    auto i = entities.begin();
    for (int y = 0; y < map_y; y++)
    {
        for (int x = 0; x < map_x; x++)
        {
            if (move_entity(i->first, x, y))
            {
                i = std::next(i);
                if (i == entities.end())
                {
                    break;
                }
            }
        }

        if (i == entities.end())
        {
            break;
        }
    }

    for (int8_t y = 0; y < map_y; y++)
    {
        net.tcp_message_all(new LoadMapMessage(y, map_x, map[y]));
    }

    success = 0;

    while (success < m_max_players)
    {
        RawMessage raw = net.await_next_message();

        TRY_DESERIALIZE(raw, SuccessMessage)
        {
            if (message->code == 2)
            {
                success++;
            }
        }
        else
        {
            std::cerr << "No use for message" << std::endl;
            std::cerr << raw.stream.str() << std::endl;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    net.tcp_message_all(new SuccessMessage(3));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    game_loop();
}


void GameServer::game_loop()
{
    bool playing = true;

    while (playing)
    {
        //Turn Order
        for (auto turnID : turn_order)
        {
            bool turnOver = false;
            while (!turnOver)
            {
                net.tcp_message_all(new InfoMessage{turnID, "BeginTurn"});

                RawMessage raw = net.await_next_message();

                if (raw.id == turnID)
                {
                    TRY_DESERIALIZE(raw, TileUpdateMessage)
                    {
                        if (move_entity(raw.id, message->x, message->y))
                        {
                            net.tcp_message_all(new TileUpdateMessage{message->x, message->y, turnID});
                            turnOver = true;
                        }
                        else
                        {
                            net.tcp_message_id(new ErrorMessage{1, "Invalid movement"}, raw.id);
                        }
                    }
                    else
                    {
                        std::cerr << "No use for message" << std::endl;
                        std::cerr << raw.stream.str() << std::endl;
                    }
                }
                else
                {
                    net.tcp_message_id(new ErrorMessage{0, "Not your turn"}, raw.id);
                }
            }
        }
    }
}

bool GameServer::move_entity(const int8_t &id, const int8_t &x, const int8_t &y)
{
    if (map[y][x] != -1)
        return false;

    auto& e = entities[id];

    if (e->coordX != -1 && e->coordY != -1)
        map[e->coordY][e->coordX] = -1;

    e->coordX = x;
    e->coordY = y;

    map[y][x] = id;

    return true;
}
