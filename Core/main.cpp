#include <iostream>

#include "NetworkManager.h"

#include <thread>

int main()
{
    NetworkManager net(4300, 4301);

    net.set_accept_new_client(true);

    std::string message;

    while (true)
    {
        if (net.get_next_message(message))
        {
            std::cout << message << std::endl;
        }
    }

    return 0;
}