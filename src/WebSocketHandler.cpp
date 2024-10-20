#include "WebSocketHandler.hpp"

#include "UserConnection.hpp"
#include "nlohmann/json.hpp"
#include <iostream>

using json = nlohmann::json; // Define a shorthand for the json type

std::map<struct lws *, struct UserConnection> WebSocketHandler::ws_connections;

int WebSocketHandler::startServer(int port) {
    lws_set_log_level(0, NULL);

    struct lws_context_creation_info context_info;
    memset(&context_info, 0, sizeof(context_info));

    // Define the protocols
    static struct lws_protocols protocols[] = {
        {"http", lws_callback_http_dummy, 0, 0},
        // max message size of 10kb
        {"echo-protocol", callbackEcho, 0, 10000},
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
    ws_connections.erase(connection);
}

// TODO: look into using the user argument
int WebSocketHandler::callbackEcho(struct lws *connection,
                                   enum lws_callback_reasons reason, void *user,
                                   void *in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        std::cout << "Client connected!" << std::endl;

        ws_connections[connection] = {
            .room_id = "test_id",
            .username = "test_username",
            .socket = connection,
        };
        break;

    case LWS_CALLBACK_RECEIVE: {

        std::cout << "Received message: " << (const char *)in
                  << " (length: " << len << ")" << std::endl;

        UserConnection &user = ws_connections[connection];
        std::string event = std::string((const char *)in).substr(0, len);

        handleEvent(user, event);

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

void WebSocketHandler::handleEvent(UserConnection &user,
                                   const std::string &event) {
    // parse event
    json event_map = json::parse(event);

    // could be changed to use enum
    EventType event_type = static_cast<EventType>(event_map["type"]);
    switch (event_type) {
    case NEW_STROKE:
        handleNewStroke(user, event_map);
        break;
    case MOVE_STROKE:
        handleMoveStroke(user, event_map);
        break;
    case DELETE_STROKE:
        handleDeleteStroke(user, event_map);
        break;
    case NEW_TEXT_BOX:
        handleNewTextBox(user, event_map);
        break;
    case SET_CURSOR_TEXT_BOX:
        handleSetCursorTextBox(user, event_map);
        break;
    case DELETE_CURSOR_TEXT_BOX:
        handleDeleteCursorTextBox(user, event_map);
        break;
    case EDIT_TEXT_BOX:
        handleEditTextBox(user, event_map);
        break;
    case TRANSFORM_TEXT_BOX:
        handleTransformTextBox(user, event_map);
        break;
    case DELETE_TEXT_BOX:
        handleDeleteTextBox(user, event_map);
        break;
    case NEW_SHAPE:
        handleNewShape(user, event_map);
        break;
    case TRANSFORM_SHAPE:
        handleTransformShape(user, event_map);
        break;
    case DELETE_SHAPE:
        handleDeleteShape(user, event_map);
        break;
    case NEW_SYMBOL:
        handleNewSymbol(user, event_map);
        break;
    case TRANSFORM_SYMBOL:
        handleTransformSymbol(user, event_map);
        break;
    case DELETE_SYMBOL:
        handleDeleteSymbol(user, event_map);
        break;
    default:
        std::cerr << "invalid event" << std::endl;
        break;
    }
}