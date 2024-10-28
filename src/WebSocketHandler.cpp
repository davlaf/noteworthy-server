#include "WebSocketHandler.hpp"

#include "CanvasObject.hpp"
#include "ServerState.hpp"
#include "Stroke.hpp"
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

        // std::cout << "Received message: " << (const char *)in
        //           << " (length: " << len << ")" << std::endl;

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

std::unique_ptr<CanvasObject>
createObject(CanvasObject::ObjectType object_type) {
    switch (object_type) {
    case CanvasObject::STROKE:
        return std::make_unique<Stroke>();
    case CanvasObject::SYMBOL:
        std::cout << "Symbol creation not supported.";
        break;
    case CanvasObject::SHAPE:
        std::cout << "Shape creation not supported.";
        break;
    case CanvasObject::TEXT:
        std::cout << "Text creation not supported.";
        break;
    case CanvasObject::BACKGROUND_IMAGE:
        std::cout << "Background image creation not supported.";
        break;
    default:
        std::cout << "Unsupported object type!";
    }
    assert(false); // Unsupported object
    return nullptr;
}

void WebSocketHandler::handleEvent(UserConnection &user,
                                   const std::string &message) {
    std::cout << message << std::endl;
    nlohmann::json event = nlohmann::json::parse(message);

    auto event_type = static_cast<CanvasObject::EventType>(event["event_type"]);
    switch (event_type) {
    case CanvasObject::CREATE: {
        state.manipulateRoom(event["room_id"], [user, event](RoomState &room) {
            room.manipulatePage(event["page_id"], [user,
                                                   event](Page &page) mutable {
                auto object_type =
                    static_cast<CanvasObject::ObjectType>(event["object_type"]);

                std::unique_ptr<CanvasObject> object =
                    createObject(object_type);
                object->fromJson(event);
                page.addObject(std::move(object));
            });
        });
        break;
    }
    case CanvasObject::DELETE: {
        state.manipulateRoom(event["room_id"], [event](RoomState &room) {
            room.manipulatePage(event["page_id"], [event](Page &page) {
                page.deleteObject(event["object_id"]);
            });
        });

        break;
    }
    case CanvasObject::MOVE:
    case CanvasObject::SCALE:
    case CanvasObject::ROTATE:
    case CanvasObject::APPEND:
    case CanvasObject::EDIT: {
        state.manipulateRoom(event["room_id"], [event](RoomState &room) {
            room.manipulatePage(event["page_id"], [event](Page &page) {
                page.manipulateObject(event["object_id"],
                                      [event](CanvasObject &canvas_object) {
                                          canvas_object.applyEvent(event);
                                      });
            });
        });
        break;
    }
    default: {
        std::cout << "event type not recognized in websockethandler";
        assert(false);
        break;
    }
    }

    // forward messages to everyone
    for (auto other_user : ws_connections) {
        if (other_user.second == user)
            continue;
        // TODO: make this work
        // if (other_user.second.room_id != event["room_id"]) {
        //     continue;
        // }

        other_user.second.sendEvent(message);
    }
}
