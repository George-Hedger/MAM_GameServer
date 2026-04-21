#include "NetworkManager.h"

#include <algorithm>
#include <iostream>
#include <thread>

NetworkManager::NetworkManager(const unsigned short tcp_port, const unsigned short udp_port) : m_tcp_port(tcp_port), m_udp_port(udp_port)
{
    //std::thread(&NetworkManager::udp_start, this).detach();
    std::thread(&NetworkManager::tcp_start, this).detach();
}

void NetworkManager::set_accept_new_client(const bool accept_new_client)
{
    std::lock_guard lock(m_accept_new_client_mutex);
    m_accept_new_client = accept_new_client;
}

bool NetworkManager::get_next_message(std::string &message)
{
    if (!m_queue_has_message.load())
        return false;
    else
    {
        std::lock_guard lock(m_message_queue_mutex);

        message = m_message_queue.front();
        m_message_queue.pop();

        if (m_message_queue.empty())
            m_queue_has_message.store(false);

        return true;
    }
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

void NetworkManager::handle_client(sf::TcpSocket *client, const int id)
{
    while (true)
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
            add_message(message);
        }
    }

    // Everything that follows only makes sense if we have a graceful way to exiting the loop.
    // Remove the client from the list when done
    {
        std::lock_guard lock(m_clients_mutex);
        m_clients.erase(id);
    }

    delete client;
}

void NetworkManager::add_message(const std::string &message)
{
    std::lock_guard lock(m_message_queue_mutex);
    m_message_queue.push(message);
    m_queue_has_message.store(true);
}

void NetworkManager::tcp_message_all(const std::string &message)
{
    std::lock_guard lock(m_clients_mutex);
    for (const auto&[fst, snd] : m_clients)
    {
        tcp_message_client(message, snd);
    }
}

void NetworkManager::tcp_message_id(const std::string &message, const int &id)
{
    std::lock_guard lock(m_clients_mutex);
    if(const auto client = m_clients.at(id); client != nullptr)
        tcp_message_client(message, client);
}

void NetworkManager::tcp_message_client(const std::string &message, sf::TcpSocket* client)
{
    // SENDING
    if (client->send(message.c_str(), message.size() + 1)
        != sf::Socket::Status::Done)
    {
        std::cerr << "Error sending message to client" << std::endl;
    }
}
