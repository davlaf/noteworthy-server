#pragma once
#include "User.hpp"
#include "nlohmann/json.hpp"
#include <libwebsockets.h>
#include <list>
#include <string>
using json = nlohmann::json; // Define a shorthand for the json type

class WebSocketHandler {
public:
    static std::map<struct lws*, struct UserConnection> ws_connections;

    static int startServer(int port);
    static int callbackEcho(struct lws* connection,
        enum lws_callback_reasons reason, void* user,
        void* in, size_t len);
    static void handleEvent(User& user, const std::string& message);
};