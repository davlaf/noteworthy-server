#pragma once
#include "UserRoles.hpp"
#include <iostream>
#ifndef NOTEWORTHY_QT
#include <libwebsockets.h>
#endif
#include <string>
#include <vector>

class UserConnection {
  public:
    std::string room_id;
    std::string username;
    UserRole role;
#ifndef NOTEWORTHY_QT
    struct lws *socket;

    UserConnection(std::string room_id, std::string username,
                   struct lws *socket, UserRole role)
        : room_id(room_id), username(username), socket(socket), role(role) {};
#endif

    UserConnection(std::string room_id, std::string username, UserRole role)
        : room_id(room_id), username(username), role(role) {};

    UserConnection() = default;

#ifndef NOTEWORTHY_QT
    bool operator==(const UserConnection &other) const {
        return socket == other.socket;
    }
    void sendEvent(const std::string &message) const {
        // Prepare the message buffer
        size_t message_buffer_length =
            message.length() + 1; // Include null terminator
        std::vector<unsigned char> buf(LWS_PRE + message_buffer_length);

        // Copy the message into the buffer
        memcpy(&buf[LWS_PRE], message.c_str(), message_buffer_length);

        // Send the message back to the client
        size_t message_length = message_buffer_length;
        int result =
            lws_write(socket, &buf[LWS_PRE], message_length, LWS_WRITE_TEXT);

        if (result < 0) {
            // Handle error (log it, clean up, etc.)
            std::cerr << "Failed to send message, code: " << result
                      << std::endl;
        }
    }
#endif
};