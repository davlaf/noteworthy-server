#include "WebSocketHandler.hpp"
#include "main-api-server.hpp"
#include <iostream>
#include <pthread.h>

void *httpServerThread(void *arg) {
  startServer(8080);
  return nullptr;
}

void *wsServerThread(void *arg) {
  WebSocketHandler::startServer(8081);
  return nullptr;
}

int main() {
  std::cout << "Creating threads for HTTP and WebSocket server" << std::endl;

  pthread_t http_server_tid;
  pthread_t ws_server_tid;

  // Create a new thread to run the http server
  if (pthread_create(&http_server_tid, nullptr, httpServerThread, nullptr) !=
      0) {
    std::cerr << "Error: Unable to create thread for http server" << std::endl;
    return 1;
  }

  // Create a new thread to run the web socket server
  if (pthread_create(&ws_server_tid, nullptr, wsServerThread, nullptr) != 0) {
    std::cerr << "Error: Unable to create thread for web socket server"
              << std::endl;
    return 1;
  }

  pthread_join(http_server_tid, nullptr);
  pthread_join(ws_server_tid, nullptr);

  return 0;
}
