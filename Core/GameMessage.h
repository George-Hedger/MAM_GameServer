//
// Created by george on 11/04/2026.
//

#ifndef MAM_GAMESERVER_GAMEMESSAGE_H
#define MAM_GAMESERVER_GAMEMESSAGE_H

#include <iostream>
#include <sstream>
#include <utility>

#define TRY_DESERIALIZE(raw, message_type) if (const auto* message = MessageFactory::try_deserialize<message_type>(raw); message != nullptr)

enum MessageType : int8_t
{
    REGISTER = 0,
    ERROR = 1,
    SUCCESS = 2,
    INFO = 3,
    TILE_UPDATE = 4
};

struct RawMessage
{
    RawMessage(const char* data, const size_t & size, const unsigned int &_id) : id(_id)
    {
        type = static_cast<MessageType>(*data);

        stream.write(data + 1, static_cast<std::streamsize>(size) - 1);
    }

    MessageType type;
    std::stringstream stream{};
    const unsigned int id;
};

struct GameMessage
{
    explicit GameMessage(const MessageType _type) : type(_type){}

    [[nodiscard]] std::stringstream serialize() const
    {
        std::stringstream stream;
        stream << type;
        _serialize(stream);
        return stream;
    }

    MessageType type;

protected:
    friend class MessageFactory;

    virtual void _deserialize(std::stringstream &stream) = 0;

    virtual void _serialize(std::stringstream &stream) const = 0;

    void deserialize(std::stringstream &stream)
    {
        _deserialize(stream);
    }
};

//
//
//

struct RegisterMessage : GameMessage
{
    RegisterMessage() : GameMessage(REGISTER){}

    RegisterMessage(std::string _chosen_player_name) : GameMessage(REGISTER),
        chosen_player_name(std::move(_chosen_player_name)){}

    std::string chosen_player_name;

protected:
    void _deserialize(std::stringstream &stream) override
    {
        uint8_t len;
        stream >> len;
        char data[len];
        stream.read(data, len);
        chosen_player_name = std::string(data, len);
    }

    void _serialize(std::stringstream &stream) const override
    {
        stream << static_cast<uint8_t>(chosen_player_name.length()) << chosen_player_name;
    }
};

struct ErrorMessage : GameMessage
{
    ErrorMessage() : GameMessage(ERROR){}

    ErrorMessage(const int8_t &_code, std::string _message) : GameMessage(ERROR),
        code(_code), error_message(std::move(_message)){}

    int8_t code{};
    std::string error_message;

protected:
    void _deserialize(std::stringstream &stream) override
    {
        uint8_t len;
        stream >> code >> len;
        char data[len];
        stream.read(data, len);
        error_message = std::string(data, len);
    }

    void _serialize(std::stringstream &stream) const override
    {
        stream << code << static_cast<uint8_t>(error_message.length()) << error_message;;
    }
};

struct SuccessMessage : GameMessage
{
    SuccessMessage() : GameMessage(SUCCESS) {}

    SuccessMessage(const int8_t &_code) : GameMessage(SUCCESS),
        code(_code){}

    int8_t code{};

protected:
    void _deserialize(std::stringstream &stream) override
    {
        stream >> code;
    }

    void _serialize(std::stringstream &stream) const override
    {
        stream << code;
    }
};

struct InfoMessage : GameMessage
{
    InfoMessage() : GameMessage(INFO){}

    InfoMessage(const int8_t &_payload, std::string _details) : GameMessage(INFO),
        payload(_payload), details(std::move(_details)){}

    int8_t payload{};
    std::string details;

protected:
    void _deserialize(std::stringstream &stream) override
    {
        uint8_t len;
        stream >> payload >> len;
        char data[len];
        stream.read(data, len);
        details = std::string(data, len);
    }

    void _serialize(std::stringstream &stream) const override
    {
        stream << payload << static_cast<uint8_t>(details.length()) << details;;
    }
};

struct TileUpdateMessage : GameMessage
{
    TileUpdateMessage() : GameMessage(TILE_UPDATE) {}

    TileUpdateMessage(const int8_t &_x, const int8_t &_y, const int8_t &_occupied_id = -1) : GameMessage(TILE_UPDATE),
        x(_x), y(_y), occupied_id(_occupied_id) {}

    int8_t x{};
    int8_t y{};
    int8_t occupied_id{};

protected:
    void _deserialize(std::stringstream &stream) override
    {
        stream >> x >> y >> occupied_id;
    }

    void _serialize(std::stringstream &stream) const override
    {
        stream << x << y << occupied_id;
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
            message->deserialize(raw.stream);

            std::cout << "New message of type " << int(raw.type) << std::endl;

            return message;
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
            case INFO:
                return std::is_same_v<T, InfoMessage>;
            case TILE_UPDATE:
                return std::is_same_v<T, TileUpdateMessage>;
                break;
        }

        std::cout << "Could not find message of type" << int(type) << std::endl;

        return false;
    }
};



#endif //MAM_GAMESERVER_GAMEMESSAGE_H