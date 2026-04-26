//
// Created by george on 11/04/2026.
//

#ifndef MAM_GAMESERVER_GAMEMESSAGE_H
#define MAM_GAMESERVER_GAMEMESSAGE_H

#include <sstream>
#include <utility>

#define TRY_DESERIALIZE(raw, message_type) if (const auto* message = MessageFactory::try_deserialize<message_type>(raw); message != nullptr)

enum MessageType : char
{
    REGISTER = 0,
    ERROR = 1,
    SUCCESS = 2
};

struct RawMessage
{
protected:
    friend class NetworkManager;
    RawMessage(const unsigned short &client_id, char* _payload, const size_t &size) :
        id(client_id),
        type(static_cast<MessageType>(_payload[0])),
        payload(std::string(_payload + 1, size))
    {}

public:
    const unsigned short id;
    const MessageType type;
    std::stringstream payload{};
};

struct GameMessage
{
protected:
    virtual void deserialize(std::stringstream& payload) = 0;

    virtual void int_serialize(std::stringstream& stream) const = 0;

public:

    [[nodiscard]] std::stringstream serialize() const
    {
        std::stringstream stream;
        stream << type;
        int_serialize(stream);
        return stream;
    }

    unsigned short id{};
    MessageType type{};
};

struct RegisterMessage : GameMessage
{
protected:
    friend class MessageFactory;
    void deserialize(std::stringstream& payload) override
    {
        chosen_player_name = payload.str();
    }

    void int_serialize(std::stringstream& stream) const override
    {
        stream << chosen_player_name;
    }

public:
    RegisterMessage()
    {
        type = REGISTER;
    }

    std::string chosen_player_name;
};

struct ErrorMessage : GameMessage
{
protected:
    friend class MessageFactory;
    void deserialize(std::stringstream& payload) override
    {
        payload >> code;
    }

    void int_serialize(std::stringstream& stream) const override
    {
        stream << code << error_message;
    }

public:
    ErrorMessage()
    {
        type = ERROR;
    }
    ErrorMessage(const unsigned short &_code, std::string _message) : error_message(std::move(_message)), code(_code)
    {
        type = ERROR;
    }

    std::string error_message;
    unsigned short code{};
};

struct SuccessMessage : GameMessage
{
protected:
    friend class MessageFactory;
    void deserialize(std::stringstream& payload) override {}

    void int_serialize(std::stringstream& stream) const override {}

public:
    SuccessMessage()
    {
        type = SUCCESS;
    }
};

class MessageFactory
{
public:
    template<typename T>
    static T* try_deserialize(RawMessage& raw)
    {
        if (check_type<T>(raw.type))
        {
            T* message = new T();
            message->id = raw.id;
            message->type = raw.type;
            message->deserialize(raw.payload);
        }

        return nullptr;
    }

private:
    template<typename T>
    static bool check_type(const MessageType& type)
    {
        switch (type)
        {
            case REGISTER:
                return std::is_same_v<T, RegisterMessage>;
            case ERROR:
                return std::is_same_v<T, ErrorMessage>;
            case SUCCESS:
                return std::is_same_v<T, SuccessMessage>;
                break;
        }

        return false;
    }
};



#endif //MAM_GAMESERVER_GAMEMESSAGE_H