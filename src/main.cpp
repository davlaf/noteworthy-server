#include "ServerState.hpp"
#include "WebSocketHandler.hpp"
#include "main-api-server.hpp"
#include <atomic>
#include <csignal>
#include <iostream>
#include <pthread.h>
#include <thread>

// Global atomic flag for termination
std::atomic<bool> terminate_flag(false);

// Signal handler to set the termination flag
void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nSignal received, shutting down..." << std::endl;
        terminate_flag.store(true);
    }
}

// HTTP server thread function
void* httpServerThread(void* arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr); // Allow thread cancellation
    while (!terminate_flag.load()) {
        startServer(8080); // Ensure startServer can clean up if terminated
    }
    std::cout << "HTTP server thread exiting." << std::endl;
    return nullptr;
}

// WebSocket server thread function
void* wsServerThread(void* arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr); // Allow thread cancellation
    while (!terminate_flag.load()) {
        WebSocketHandler::startServer(8081); // Ensure WebSocketHandler can clean up if terminated
    }
    std::cout << "WebSocket server thread exiting." << std::endl;
    return nullptr;
}
int startServer()
{
    std::cout << "Creating threads for HTTP and WebSocket server" << std::endl;

    pthread_t http_server_tid;
    pthread_t ws_server_tid;

    // Create a new thread to run the http server
    if (pthread_create(&http_server_tid, nullptr, httpServerThread, nullptr) != 0) {
        std::cerr << "Error: Unable to create thread for http server"
                  << std::endl;
        return 1;
    }

    // Create a new thread to run the web socket server
    if (pthread_create(&ws_server_tid, nullptr, wsServerThread, nullptr) != 0) {
        std::cerr << "Error: Unable to create thread for web socket server"
                  << std::endl;
        return 1;
    }

    // Wait for termination signal
    while (!terminate_flag.load()) {
        // Polling main loop to wait for the termination signal
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cancel threads
    pthread_cancel(http_server_tid);
    pthread_cancel(ws_server_tid);

    // Join threads to clean up resources
    pthread_join(http_server_tid, nullptr);
    pthread_join(ws_server_tid, nullptr);

    std::cout << "All threads exited gracefully." << std::endl;
    return 0;
    return 0;
}

int main()
{
    std::string room_id = state.createRoom("ABCDE", "joe", "ggggghh");
    std::string room_id2 = state.createRoom("12345", "david", "");

    return startServer();
}
