//
// Created by george on 11/04/2026.
//

#ifndef MAM_GAMESERVER_GAMEMESSAGE_H
#define MAM_GAMESERVER_GAMEMESSAGE_H
#endif //MAM_GAMESERVER_GAMEMESSAGE_H

enum MessageType : unsigned short
{
    MOVE = 0,
    ACTION = 1,
};

struct GameMessage
{
    const int client_id;
    const MessageType type;
    const char payload[1024];
};