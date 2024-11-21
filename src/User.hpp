#pragma once
#include <iostream>

#include "UserRoles.hpp"
#ifndef NOTEWORTHY_QT
#include <libwebsockets.h>
#endif
#include <string>
#include <vector>

#include "EventTypeEnums.hpp"
#include "SendableObject.hpp"
#include "nlohmann/json.hpp"

class User : public SendableObject {
public:
    std::string username;
    bool is_connected = false;
    bool is_kicked = false;
#ifndef NOTEWORTHY_QT
    struct lws* socket;

    User(std::string room_id, std::string username, struct lws* socket)
        : SendableObject(room_id)
        , username(username)
        , socket(socket) {};
#endif

    User(std::string room_id, std::string username)
        : username(username) {};

    User() = default;

    enum UserEventType {
        CREATE,
        DELETE,
        CONNECT,
        DISCONNECT,
        KICK,
    };

    virtual EventObjectType getObjectType() override
    {
        return USER;
    };

    virtual void addMetaInformation(nlohmann::json& json) override
    {
        SendableObject::addMetaInformation(json);
        json["username"] = username;
    }

    virtual void retrieveMetaInformation(const nlohmann::json& json) override
    {
        SendableObject::retrieveMetaInformation(json);
        json.at("username").get_to(username);
    }

    virtual void toJson(nlohmann::json& json) override
    {
        addMetaInformation(json);
        json["username"] = username;
        json["is_connected"] = is_connected;
        json["is_kicked"] = is_kicked;
    };

    virtual void fromJson(const nlohmann::json& json) override
    {
        retrieveMetaInformation(json);
        json.at("username").get_to(username);
        json.at("is_connected").get_to(is_connected);
        json.at("is_kicked").get_to(is_connected);
    };

    void createConnectEvent(nlohmann::json& json)
    {
        json["event_type"] = CONNECT;
    }

    void createDisconnectEvent(nlohmann::json& json)
    {
        json["event_type"] = DISCONNECT;
    }

    void createKickEvent(nlohmann::json& json)
    {
        json["event_type"] = KICK;
    }

    virtual void applyEvent(const nlohmann::json& json) override
    {
        auto event_type = static_cast<UserEventType>(json["event_type"]);
        switch (event_type) {
        case CREATE:
        case DELETE:
            throw std::logic_error("should've been handled by ws handler");
            break;
        case CONNECT: {
            is_connected = true;
            break;
        }
        case DISCONNECT: {
            is_connected = false;
            break;
        }
        case KICK: {
            is_kicked = false;
            break;
        }
        };
    }

#ifndef NOTEWORTHY_QT
    bool operator==(const User& other) const
    {
        return socket == other.socket;
    }
    void sendEvent(const std::string& message) const
    {
        // Prepare the message buffer
        size_t message_buffer_length = message.length() + 1; // Include null terminator
        std::vector<unsigned char> buf(LWS_PRE + message_buffer_length);

        // Copy the message into the buffer
        memcpy(&buf[LWS_PRE], message.c_str(), message_buffer_length);

        // Send the message back to the client
        size_t message_length = message_buffer_length;
        int result = lws_write(socket, &buf[LWS_PRE], message_length, LWS_WRITE_TEXT);

        if (result < 0) {
            // Handle error (log it, clean up, etc.)
            std::cerr << "Failed to send message, code: " << result << std::endl;
        }
    }
#endif
};