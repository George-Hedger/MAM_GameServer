#include "NetworkManager.h"

#include <algorithm>
#include <iostream>
#include <thread>
#include <SFML/Network.hpp>


NetworkManager::~NetworkManager()
{
    std::lock_guard lock(m_clients_mutex);
    for (const auto&[fst, snd] : m_clients)
    {
        delete &snd;
    }
}

void NetworkManager::start()
{
    //std::thread(&NetworkManager::udp_start, this).detach();
    std::thread(&NetworkManager::tcp_start, this).detach();
}

void NetworkManager::set_accept_new_client(const bool accept_new_client)
{
    std::lock_guard lock(m_accept_new_client_mutex);
    m_accept_new_client = accept_new_client;
}

bool NetworkManager::get_accept_new_client()
{
    std::lock_guard lock(m_accept_new_client_mutex);
    return m_accept_new_client;
}

RawMessage NetworkManager::await_next_message()
{
    std::unique_lock lock(m_queue_mutex);     // Lock the mutex
    m_queue_cond.wait(lock, [this]() { return !m_queue.empty(); }); // Wait until queue is not empty

    RawMessage value = std::move(m_queue.front());
    m_queue.pop();

    return value;
}

void NetworkManager::tcp_start()
{
    // BINDING
    sf::TcpListener listener;
    if (listener.listen(m_tcp_port) != sf::Socket::Status::Done)
    {
        std::cerr << "Error binding listener to port" << std::endl;
        return;
    }

    std::cout << "TCP Server is listening to port "
       << m_tcp_port
       << ", waiting for connections..."
       << std::endl;

    while (true)
    {
        // ACCEPTING
        if (auto* client = new sf::TcpSocket; listener.accept(*client) != sf::Socket::Status::Done)
        {
            std::cerr << "Error accepting new TCP client" << std::endl;
            return;
        }
        else
        {
            bool accept_new_client;
            {
                std::lock_guard lock(m_accept_new_client_mutex);
                accept_new_client = m_accept_new_client;
            }

            if (accept_new_client)
            {
                //Lock scope
                {
                    std::lock_guard lock(m_clients_mutex);
                    m_clients.emplace(std::pair{m_next_client_id, client});
                }

                std::cout << "New client connected: "
                    << client->getRemoteAddress().value()
                    << std::endl;

                std::thread(&NetworkManager::handle_client, this, client, m_next_client_id).detach();
                m_next_client_id++;

                if (m_max_players == m_next_client_id)
                {
                    set_accept_new_client(false);
                }
            }
            else
            {
                //TODO add "game in progress" message
            }
        }
    }
    // No need to call close of the listener.
    // The connection is closed automatically when the listener object is out of scope.
}

/*
void NetworkManager::udp_start()
{
    // BINDING
    sf::UdpSocket socket;

    if (socket.bind(m_udp_port) != sf::Socket::Status::Done)
    {
        std::cerr << "Error binding socket to port " << m_udp_port << std::endl;
        return;
    }

    std::cout << "UDP Server started on port " << m_udp_port << std::endl;

    while (true)
    {
        std::array<char, 1024> buffer = {};
        std::size_t received;
        unsigned short receiver_port;

        // RECEIVING
        std::optional<sf::IpAddress> sender;

        if (socket.receive(buffer.data(), buffer.size(), received, sender, receiver_port)
            != sf::Socket::Status::Done)
        {
            std::cerr << "Error receiving UDP data" << std::endl;
        }

        std::cout << "Received " << received << " bytes from " << sender.value() << ", remote port: " << receiver_port
            << std::endl << "Message reads: " << std::endl << buffer.data() << std::endl;

        // SENDING
        if (socket.send(buffer.begin(), received, sender.value(), receiver_port)
            != sf::Socket::Status::Done)
        {
            std::cerr << "Error sending data" << std::endl;
        }
    }

    socket.unbind();
    std::cout << "UDP Server stopped" << std::endl;
}
*/

void NetworkManager::handle_client(sf::TcpSocket *client, const unsigned short id)
{
    while (client != nullptr)
    {
        // RECEIVING
        char payload[1024] = {};
        if (size_t received; client->receive(payload, 1024, received)
            != sf::Socket::Status::Done)
        {
            std::cerr << "Error receiving message from client" << std::endl;
        }
        else
        {
            std::string message(payload);
            std::cout << "Received message: " << message << std::endl;

            add_message(id, payload, received);
        }
    }

    // Everything that follows only makes sense if we have a graceful way to exiting the loop.
    // Remove the client from the list when done
    {
        std::lock_guard lock(m_clients_mutex);
        m_clients.erase(id);
    }
}

void NetworkManager::add_message(const unsigned short &id, char* payload, const size_t &size)
{
    {
        std::lock_guard lock(m_queue_mutex);
        m_queue.push(RawMessage(id, payload, size));
    }
    m_queue_cond.notify_one();
}

void NetworkManager::tcp_message_all(const GameMessage *message)
{
    const auto payload = message->serialize();

    std::lock_guard lock(m_clients_mutex);
    for (const auto&[fst, snd] : m_clients)
    {
        tcp_message_client(payload, snd);
    }
}

void NetworkManager::tcp_message_id(const GameMessage *message, const int &id)
{
    std::lock_guard lock(m_clients_mutex);
    if(const auto client = m_clients.at(id); client != nullptr)
        tcp_message_client(message->serialize(), client);
}

void NetworkManager::tcp_message_client(const std::stringstream &payload, sf::TcpSocket* client)
{
    // SENDING
    if (client->send(payload.str().data(), payload.str().size())
        != sf::Socket::Status::Done)
    {
        std::cerr << "Error sending message to client" << std::endl;
    }
}
