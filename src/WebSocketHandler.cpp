#include "WebSocketHandler.hpp"
#include <iostream>

std::list<UserConnection> WebSocketHandler::ws_connections;

int WebSocketHandler::startServer(int port) {

  lws_set_log_level(0, NULL);

  struct lws_context_creation_info context_info;
  memset(&context_info, 0, sizeof(context_info));

  // Define the protocols
  static struct lws_protocols protocols[] = {
      {"http", lws_callback_http_dummy, 0, 0},
      {"echo-protocol", callbackEcho, 0, 128},
      {NULL, NULL, 0, 0} // terminator
  };

  // Setup context information
  context_info.port = port;
  context_info.protocols = protocols;

  // Create context

  lws_context *context = lws_create_context(&context_info);

  if (!context) {
    std::cerr << "Failed to create websocket context!" << std::endl;
    return -1;
  }

  std::cout << "WebSocket server started on port " << context_info.port
            << std::endl;

  // Event loop
  while (lws_service(context, 1000) >= 0) {
    // Keep running
  }

  // Cleanup
  lws_context_destroy(context);
  return 0;
}

void WebSocketHandler::removeConnection(struct lws *connection) {
  ws_connections.remove({.socket = connection});
}

int WebSocketHandler::callbackEcho(struct lws *connection,
                                   enum lws_callback_reasons reason, void *user,
                                   void *in, size_t len) {
  switch (reason) {
  case LWS_CALLBACK_ESTABLISHED:
    std::cout << "Client connected!" << std::endl;

    ws_connections.push_back({
        .room_id = "test_id",
        .username = "test_username",
        .socket = connection,
    });
    break;

  case LWS_CALLBACK_RECEIVE: {
    std::cout << "Received message: " << (const char *)in << " (length: " << len
              << ")" << std::endl;

    for (struct UserConnection &connection : ws_connections) {
      // Prepare the response buffer
      unsigned char buf[LWS_PRE + len];
      memcpy(&buf[LWS_PRE], in, len); // Copy the received data to the buffer

      // Echo the message back to the client
      size_t message_length = len;
      lws_write(connection.socket, &buf[LWS_PRE], message_length,
                LWS_WRITE_TEXT);
    }

    break;
  }

  case LWS_CALLBACK_CLOSED:
    std::cout << "Client disconnected!" << std::endl;
    removeConnection(connection);
    break;

  default:
    break;
  }

  // For debugging purposes
  return 0;
}