#include "WebSocketHandler.hpp"

#include "CanvasObject.hpp"
#include "ServerState.hpp"
#include "Stroke.hpp"
#include "UserConnection.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <libwebsockets.h>

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

int WebSocketHandler::callbackEcho(struct lws *connection,
                                   enum lws_callback_reasons reason, void *user,
                                   void *in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED: {
        std::cout << "Client connected!" << std::endl;

        char raw_room_id[64] = {0};
        char raw_user_id[128] = {0};
        char decoded_room_id[64] = {0};
        char decoded_user_id[128] = {0};

        // Extract room_id
        if (lws_get_urlarg_by_name(connection, "room_id=", raw_room_id,
                                   sizeof(raw_room_id))) {
            // Decode room_id
            int decode_len = lws_urldecode(raw_room_id, decoded_room_id,
                                           sizeof(decoded_room_id));
            if (decode_len >= 0) { // Check for successful decoding
                std::cout << "Decoded Room ID: " << decoded_room_id
                          << std::endl;
            } else {
                std::cerr << "Failed to decode room_id!" << std::endl;
                std::memset(decoded_room_id, 0, sizeof(decoded_room_id));
            }
        } else {
            std::cerr << "Failed to extract room_id!" << std::endl;
        }

        // Extract user_id
        if (lws_get_urlarg_by_name(connection, "user_id=", raw_user_id,
                                   sizeof(raw_user_id))) {
            // Decode user_id
            int decode_len = lws_urldecode(raw_user_id, decoded_user_id,
                                           sizeof(decoded_user_id));
            if (decode_len >= 0) { // Check for successful decoding
                std::cout << "Decoded User ID: " << decoded_user_id
                          << std::endl;
            } else {
                std::cerr << "Failed to decode user_id!" << std::endl;
                std::memset(decoded_user_id, 0, sizeof(decoded_user_id));
            }
        } else {
            std::cerr << "Failed to extract user_id!" << std::endl;
        }

        // Store connection data
        ws_connections[connection] = {std::string(decoded_room_id),
                                      std::string(decoded_user_id), connection,
                                      UserRole::MEMBER};
        break;
    }
    case LWS_CALLBACK_RECEIVE: {
        std::string message((const char *)in, len);
        UserConnection &user = ws_connections[connection];

        try {
            handleEvent(user, message);
        } catch (const std::exception &e) {
            std::cerr << "Error handling event: " << e.what() << std::endl;
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
    return 0;
}

std::unique_ptr<CanvasObject> createCanvasObject(EventObjectType object_type) {
    switch (object_type) {
    case STROKE: {
        // Create a Stroke using the current_path
        auto stroke = std::make_unique<Stroke>();
        return std::move(stroke);
    }
    case SYMBOL:
        std::cout << "Symbol creation not supported." << std::endl;
        break;
    case SHAPE:
        std::cout << "Shape creation not supported." << std::endl;
        break;
    case TEXT:
        std::cout << "Text creation not supported." << std::endl;
        break;
    case BACKGROUND_IMAGE:
        std::cout << "Background image creation not supported." << std::endl;
        break;
    default:
        std::cout << "Unsupported object type!";
    }
    throw "unsupported object type!";
    return nullptr;
}

void WebSocketHandler::handleEvent(UserConnection &user,
                                   const std::string &message) {
    std::cout << message << std::endl;
    nlohmann::json event = nlohmann::json::parse(message);

    auto event_type = static_cast<EventType>(event["event_type"]);
    switch (event_type) {
    case CREATE: {
        auto object_type = static_cast<EventObjectType>(event["object_type"]);
        switch (object_type) {

        case ROOM: {
            // replace everything
            throw "not implemented yet";
            // state.fromJson(event);
            break;
        }
        case PAGE: {
            state.manipulateRoom(event["room_id"], [event](RoomState &room) {
                room.applyInsertPageEvent(event);
            });
            break;
        }

        case STROKE:
        case SYMBOL:
        case SHAPE:
        case TEXT:
        case BACKGROUND_IMAGE:
            state.manipulateRoom(event["room_id"], [event, object_type](
                                                       RoomState &room) {
                room.manipulatePage(event["page_id"],
                                    [event, object_type](Page &page) mutable {
                                        std::unique_ptr<CanvasObject> object =
                                            createCanvasObject(object_type);
                                        object->fromJson(event);
                                        page.addObject(std::move(object));
                                    });
            });
            break;
        default:
            throw "invalid object type";
        }
        break;
    }
    case DELETE: {
        auto object_type = static_cast<EventObjectType>(event["object_type"]);
        switch (object_type) {
        case ROOM: {
            throw "room deletion not implemented";
            break;
        }
        case PAGE: {
            state.manipulateRoom(event["room_id"], [event](RoomState &room) {
                room.applyDeletePageEvent(event);
            });
            break;
        }
        case STROKE:
        case SYMBOL:
        case SHAPE:
        case TEXT:
        case BACKGROUND_IMAGE: {
            state.manipulateRoom(event["room_id"], [event](RoomState &room) {
                uint64_t object_id = event["object_id"];
                room.manipulatePage(event["page_id"], [object_id](Page &page) {
                    page.deleteObject(object_id);
                });
            });
            break;
        }
        default:
            throw "invalid object type";
        }
        break;
    }
    case MOVE:
    case SCALE:
    case ROTATE:
    case APPEND:
    case EDIT: {
        // assume its an object
        state.manipulateRoom(event["room_id"], [event](RoomState &room) {
            uint64_t object_id = event["object_id"];
            room.manipulatePage(
                event["page_id"], [object_id, event](Page &page) {
                    page.manipulateObject(object_id,
                                          [event](CanvasObject &canvas_object) {
                                              canvas_object.applyEvent(event);
                                          });
                });
        });
        break;
    }
    default: {
        throw "event type not recognized in clientwebsockethandler";
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
