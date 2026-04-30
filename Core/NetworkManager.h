#ifndef MAM_GAMESERVER_NETWORKMANAGER_H
#define MAM_GAMESERVER_NETWORKMANAGER_H

#include <mutex>
#include <unordered_map>
#include <queue>
#include <condition_variable>
#include "GameMessage.h"

namespace sf
{
    class TcpSocket;
}

class NetworkManager {

public:
    NetworkManager(const unsigned short tcp_port, const unsigned short udp_port) : m_tcp_port(tcp_port), m_udp_port(udp_port){}
    ~NetworkManager();

    void start();

    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    void set_accept_new_client(bool accept_new_client);
    bool get_accept_new_client();

    RawMessage await_next_message();

    static constexpr int8_t m_max_players{1};

private:
    const unsigned short m_tcp_port;
    const unsigned short m_udp_port;

    std::unordered_map<unsigned int, sf::TcpSocket*> m_clients;
    std::mutex m_clients_mutex;

    unsigned int m_next_client_id{0};

    std::queue<RawMessage> m_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cond;

    bool m_accept_new_client{true};
    std::mutex m_accept_new_client_mutex;

    // Binds to a port and then loops around.  For every client that connects,
    // we start a new thread receiving their messages.
    void tcp_start();

    /*
    // UDP echo server. Used to let the clients know our IP address in case
    // they send a UDP broadcast message.
    void udp_start();
    */

    // Loop around, receive messages from client and send them to all
    // the other connected clients.
    void handle_client(sf::TcpSocket* client, unsigned int id);

    //Add `message` to vector
    void add_message(const char *data, const size_t &size, const unsigned int &id);

public:
    // Sends `message` to all connected clients
    void tcp_message_all(const GameMessage* message, const unsigned int &ignoreId = 1000);

    void tcp_message_id(const GameMessage* message, const unsigned int& id);
    void tcp_message_id(const std::stringstream &stream, const unsigned int& id);

private:
    // Sends `message` to `client`
    static void tcp_message_client(const std::stringstream &stream, sf::TcpSocket* client);
};

#endif //MAM_GAMESERVER_NETWORKMANAGER_H