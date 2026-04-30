#include "NetworkManager.h"

#include <algorithm>
#include <iostream>
#include <thread>
#include <SFML/Network.hpp>


NetworkManager::~NetworkManager()
{
    std::lock_guard lock(m_clients_mutex);
    for (auto&[fst, snd] : m_clients)
    {
        snd->disconnect();
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
                    << " ID: " << m_next_client_id << std::endl;

                std::thread(&NetworkManager::handle_client, this, client, m_next_client_id).detach();
                m_next_client_id++;
            }
            else
            {
                const auto msg = ErrorMessage(1, "Game in progress").serialize();
                tcp_message_client(msg, client);
            }
        }
    }
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
        int8_t receiver_port;

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

void NetworkManager::handle_client(sf::TcpSocket *client, const unsigned int id)
{
    while (client->getRemotePort() != 0)
    {
        char data[1024];
        switch (size_t size; client->receive(data, 1024, size))
        {
            case sf::Socket::Status::Disconnected:
                client->disconnect();
                break;
            case sf::Socket::Status::Done:
            {
                std::cout << "Received message from client: " << id<< std::endl;
                std::stringstream stream;

                stream.write(data, static_cast<std::streamsize>(size));
                add_message(data, size, id);
                break;
            }
            case sf::Socket::Status::NotReady:
            case sf::Socket::Status::Partial:
            case sf::Socket::Status::Error:
            default:
                std::cerr << "Error receiving message from client " << id << std::endl;
                break;
        }
    }

    std::cerr << "Client " << id << " disconnected!" << std::endl;

    {
        std::lock_guard lock(m_clients_mutex);
        m_clients.erase(id);
    }

    delete client;
    client = nullptr;
}

void NetworkManager::add_message(const char *data, const size_t &size, const unsigned int &id)
{
    {
        std::lock_guard lock(m_queue_mutex);
        m_queue.emplace(data, size, id);
    }

    m_queue_cond.notify_one();
}

void NetworkManager::tcp_message_all(const GameMessage *message, const unsigned int &ignoreId)
{
    const auto stream = message->serialize();

    std::lock_guard lock(m_clients_mutex);
    for (const auto&[fst, snd] : m_clients)
    {
        if (fst != ignoreId)
            tcp_message_client(stream, snd);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void NetworkManager::tcp_message_id(const GameMessage *message, const unsigned int &id)
{
    tcp_message_id(message->serialize(), id);
}

void NetworkManager::tcp_message_id(const std::stringstream &stream, const unsigned int &id)
{
    std::lock_guard lock(m_clients_mutex);
    if(const auto client = m_clients.at(id); client != nullptr)
        tcp_message_client(stream, client);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void NetworkManager::tcp_message_client(const std::stringstream &stream, sf::TcpSocket* client)
{
    std::cout << "Sending: " << stream.str() << std::endl;
    bool retry = false;
    do
    {
        switch (client->send(stream.str().c_str(), stream.str().size()))
        {
            case sf::Socket::Status::Done:
                std::cout << "Successfully sent message to client" << std::endl;
                break;
            case sf::Socket::Status::Disconnected:
                break;
            case sf::Socket::Status::Error:
                std::cerr << "Error sending message to client" << std::endl;
                break;
            case sf::Socket::Status::NotReady:
            case sf::Socket::Status::Partial:
            default:
                std::cerr << "Retrying: sending message to client" << std::endl;
                retry = true;
                break;
        }
    }
    while (retry);
}
