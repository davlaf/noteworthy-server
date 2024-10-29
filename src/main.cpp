#include "ServerState.hpp"
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

int startServer() {
    std::cout << "Creating threads for HTTP and WebSocket server" << std::endl;

    pthread_t http_server_tid;
    pthread_t ws_server_tid;

    // Create a new thread to run the http server
    if (pthread_create(&http_server_tid, nullptr, httpServerThread, nullptr) !=
        0) {
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

    pthread_join(http_server_tid, nullptr);
    pthread_join(ws_server_tid, nullptr);
    return 0;
}

int main() {

    // room code ABCDE
    std::string room_id = state.createRoom("joe", "password");
    state.manipulateRoom(room_id, [](RoomState &room) {
        auto new_page = std::make_unique<Page>();
        room.addPageAfter(0, 12345, std::move(new_page));
        // room.manipulatePage(page_id, [](Page &page) {
        //     page.addObject(IDGenerator::newID(), Stroke());
        // });
    });

    return startServer();
}
