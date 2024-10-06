#include "main-api-server.hpp"
#include <iostream>
#include <pthread.h>

void *serverThread(void *arg) {
  startServer();
  return nullptr;
}

int main() {
  std::cout << "boutta start server on a separate thread" << std::endl;

  pthread_t server_tid;

  // Create a new thread to run the server
  if (pthread_create(&server_tid, nullptr, serverThread, nullptr) != 0) {
    std::cerr << "Error: Unable to create thread for server" << std::endl;
    return 1;
  }

  std::cout << "Server is running in a separate thread" << std::endl;

  pthread_join(server_tid, nullptr);

  return 0;
}
