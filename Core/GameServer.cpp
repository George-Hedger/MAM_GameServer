#include "GameServer.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <thread>

void GameServer::tcp_start()
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
            //Lock scope
            {
                std::lock_guard lock(m_clients_mutex);
                m_clients.push_back(client);
            }

            std::cout << "New client connected: "
                << client->getRemoteAddress().value()
                << std::endl;

            std::thread(&GameServer::handle_client, this, client).detach();
        }
    }
    // No need to call close of the listener.
    // The connection is closed automatically when the listener object is out of scope.
}

void GameServer::udp_start()
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
            break;
        }

        std::cout << "Received " << received << " bytes from " << sender.value() << ", remote port: " << receiver_port
            << std::endl << "Message reads: " << std::endl << buffer.data() << std::endl;

        // SENDING
        if (socket.send(buffer.begin(), received, sender.value(), receiver_port)
            != sf::Socket::Status::Done)
        {
            std::cerr << "Error sending data" << std::endl;
            break;
        }
    }

    socket.unbind();
    std::cout << "UDP Server stopped" << std::endl;
}

void GameServer::handle_client(sf::TcpSocket *client)
{
    while (true)
    {
        // RECEIVING
        char payload[1024];
        std::memset(payload, 0, 1024);
        size_t received;
        sf::Socket::Status status = client->receive(payload, 1024, received);
        if (status != sf::Socket::Status::Done)
        {
            std::cerr << "Error receiving message from client" << std::endl;
            break;
        } else {
            // Actually, there is no need to print the message if the message is not a string
            std::string message(payload);
            std::cout << "Received message: " << message << std::endl;
            broadcast_message(message, client);
        }
    }

    // Everything that follows only makes sense if we have a graceful way to exiting the loop.
    // Remove the client from the list when done
    {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), client),
                m_clients.end());
    }
    delete client;
}

void GameServer::broadcast_message(const std::string &message, sf::TcpSocket *sender)
{
    // You might want to validate the message before you send it.
    // A few reasons for that:
    // 1. Make sure the message makes sense in the game.
    // 2. Make sure the sender is not cheating.
    // 3. First need to synchronise the players inputs (usually done in Lockstep).
    // 4. Compensate for latency and perform rollbacks (usually done in Ded Reckoning).
    // 5. Delay the sending of messages to make the game fairer wrt high ping players.
    // This is where you can write the authoritative part of the server.
    std::lock_guard<std::mutex> lock(m_clients_mutex);
    for (auto& client : m_clients)
    {
        if (client != sender)
        {
            // SENDING
            sf::Socket::Status status = client->send(message.c_str(), message.size() + 1) ;
            if (status != sf::Socket::Status::Done)
            {
                std::cerr << "Error sending message to client" << std::endl;
            }
        }
    }
}
