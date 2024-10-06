#pragma once
#include <libwebsockets.h>
#include <list>
#include <string>

struct UserConnection {
  std::string room_id;
  std::string username;
  struct lws *socket;

  bool operator==(const UserConnection &other) const {
    return socket == other.socket;
  }
};

class WebSocketHandler {
public:
  static int startServer(int port);
  static std::list<struct UserConnection> ws_connections;
  static void removeConnection(struct lws *connection);
  static int callbackEcho(struct lws *connection,
                          enum lws_callback_reasons reason, void *user,
                          void *in, size_t len);
};