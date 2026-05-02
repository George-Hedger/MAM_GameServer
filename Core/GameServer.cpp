//
// Created by george on 26/04/2026.
//

#include "GameServer.h"

#include <algorithm>
#include <iostream>
#include <random>
#include <thread>

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

    net.tcp_message_all(new RegisterMessage{"Loading Map..."});
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

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> rand_X(0, map_x - 1);
    std::uniform_int_distribution<> rand_Y(0, map_y - 1);

    for (auto &y : map)
    {
        for (auto &x : y)
        {
            x = -1;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (auto [id, name] : player_names)
    {

    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

    }
}
