#ifndef TEST_GAMESERVER_H
#define TEST_GAMESERVER_H
#endif //TEST_GAMESERVER_H

#include <mutex>
#include <unordered_map>
#include <queue>
#include <atomic>
#include <SFML/Network.hpp>

class NetworkManager {
public:
    NetworkManager(unsigned short tcp_port, unsigned short udp_port);

    void set_accept_new_client(bool accept_new_client);

    bool get_next_message(std::string& message);

    // Sends `message` to all connected clients
    void tcp_message_all(const std::string& message);

    void tcp_message_id(const std::string& message, const int& id);

private:
    unsigned short m_tcp_port;
    unsigned short m_udp_port;

    std::unordered_map<unsigned short, sf::TcpSocket*> m_clients;
    int m_next_client_id{1};
    std::mutex m_clients_mutex;

    std::atomic<bool> m_queue_has_message{false};
    std::queue<std::string> m_message_queue;
    std::mutex m_message_queue_mutex;

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
    void handle_client(sf::TcpSocket* client, int id);

    //Add `message` to vector
    void add_message(const std::string &message);

    // Sends `message` to `client`
    static void tcp_message_client(const std::string& message, sf::TcpSocket* client);
};