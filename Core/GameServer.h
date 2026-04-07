#ifndef TEST_GAMESERVER_H
#define TEST_GAMESERVER_H
#endif //TEST_GAMESERVER_H

#include <mutex>
#include <vector>
#include <SFML/Network.hpp>

class GameServer {
public:
    GameServer(const unsigned short tcp_port, const unsigned short udp_port) :
        m_tcp_port(tcp_port), m_udp_port(udp_port) {}

    // Binds to a port and then loops around.  For every client that connects,
    // we start a new thread receiving their messages.
    void tcp_start();

    // UDP echo server. Used to let the clients know our IP address in case
    // they send a UDP broadcast message.
    void udp_start();


private:
    unsigned short m_tcp_port;
    unsigned short m_udp_port;
    std::vector<sf::TcpSocket*> m_clients;
    std::mutex m_clients_mutex;

    // Loop around, receive messages from client and send them to all
    // the other connected clients.
    void handle_client(sf::TcpSocket* client);

    // Sends `message` from `sender` to all the other connected clients
    void broadcast_message(const std::string& message, sf::TcpSocket* sender);
};