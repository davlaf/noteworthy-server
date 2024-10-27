#pragma once
#include "nlohmann/json.hpp"
#include <libwebsockets.h>
#include <list>
#include <string>
using json = nlohmann::json; // Define a shorthand for the json type
#define EVENT_PARAMS UserConnection &user, const json &event

class WebSocketHandler {
  public:
    static std::map<struct lws *, struct UserConnection> ws_connections;

    static int startServer(int port);
    static void removeConnection(struct lws *connection);
    static int callbackEcho(struct lws *connection,
                            enum lws_callback_reasons reason, void *user,
                            void *in, size_t len);
    static void handleEvent(UserConnection &user, const std::string &message);
};