//
// Created by george on 26/04/2026.
//

#include "GameServer.h"

#include <algorithm>
#include <iostream>

void GameServer::start()
{
    net.start();
    login_loop();
}

void GameServer::login_loop()
{
    net.set_accept_new_client(true);

    while (net.get_accept_new_client() || player_names.size() != NetworkManager::m_max_players)
    {
        RawMessage raw = net.await_next_message();

        TRY_DESERIALIZE(raw, RegisterMessage)
        {
            std::cout << "Player: " << message->id << " requests name: " << message->chosen_player_name << std::endl;

            if (!validate_name(message->chosen_player_name, message->id))
            {
                std::cout << "Invalid name request" << std::endl;

                net.tcp_message_id(new ErrorMessage{100, "Invalid Name"}, message->id);
            }
            else
            {
                std::cout << "Name Accepted!" << std::endl;

                net.tcp_message_id(new SuccessMessage{}, message->id);
                player_names.emplace(message->id, message->chosen_player_name);
            }
        }
        else
        {
            std::cout << "Invalid message type" << std::endl;
        }
    }

    generate_world();
}

bool GameServer::validate_name(const std::string &name, const unsigned short &id) const
{
    return std::all_of(player_names.begin(), player_names.end(), [&name, &id](const std::pair<unsigned short, std::string> &pair)
    {
        if (pair.second == name || pair.first == id)
            return false;

        return true;
    });
}

void GameServer::generate_world()
{
    std::cout << "Generating world" << std::endl;
}
