#pragma once
#include <libwebsockets.h>
#include <string>

struct UserConnection {
    std::string room_id;
    std::string username;
    struct lws *socket;

    bool operator==(const UserConnection &other) const {
        return socket == other.socket;
    }
    void sendEvent(const std::string &message);
};