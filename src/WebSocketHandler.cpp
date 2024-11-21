#include "WebSocketHandler.hpp"

#include "CanvasObject.hpp"
#include "SendableObject.hpp"
#include "ServerState.hpp"
#include "Stroke.hpp"
#include "User.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <libwebsockets.h>

using json = nlohmann::json; // Define a shorthand for the json type

int WebSocketHandler::startServer(int port)
{
    // lws_set_log_level(0, NULL);
    lws_set_log_level(0b11111111111, NULL);

    struct lws_context_creation_info context_info;
    memset(&context_info, 0, sizeof(context_info));

    // Define the protocols
    static struct lws_protocols protocols[] = {
        { "http", lws_callback_http_dummy, 0, 0 },
        // max message size of 10kb
        { "echo-protocol", callbackEcho, sizeof(User*), 10000 },
        { NULL, NULL, 0, 0 } // terminator
    };

    // Setup context information
    context_info.port = port;
    context_info.protocols = protocols;

    // Create context

    lws_context* context = lws_create_context(&context_info);

    if (!context) {
        std::cerr << "Failed to create websocket context!" << std::endl;
        return -1;
    }

    std::cout << "WebSocket server started on port " << context_info.port << std::endl;

    // Event loop
    while (lws_service(context, 1000) >= 0) {
        // Keep running
    }

    // Cleanup
    lws_context_destroy(context);
    return 0;
}

std::string extract_query_parameter(struct lws* connection, const std::string& query_param)
{
    char url_encoded_string[128] = { 0 };
    char url_decoded_string[128] = { 0 };

    if (!lws_get_urlarg_by_name(connection, query_param.c_str(), url_encoded_string, sizeof(url_encoded_string))) {
        throw std::runtime_error("failed to extract " + query_param);
    }

    int decode_err = lws_urldecode(url_decoded_string, url_encoded_string, sizeof(url_decoded_string));

    if (decode_err < 0) { // Check for successful decoding
        throw std::runtime_error("failed to extract " + query_param);
    }

    // The value after the '=' in a query parameter (e.g., username=david) needs to be extracted
    std::string decoded_str(url_decoded_string);

    // Find the position of the '=' character in the decoded string (e.g., username=david)
    size_t pos = decoded_str.find('=');
    if (pos != std::string::npos) {
        // Return the substring after the '=' (the actual value, e.g., "david")
        return decoded_str.substr(pos + 1);
    }

    return decoded_str;
}

int WebSocketHandler::callbackEcho(struct lws* connection, enum lws_callback_reasons reason, void* user, void* in,
    size_t len)
{
    auto user_ptr_ptr = static_cast<User**>(user);
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED: {
        std::cout << "Client connected!" << std::endl;

        // create temporary shitass user
        User* user = new User { "", "" };
        *user_ptr_ptr = user;

        std::string username;
        std::string room_id;

        try {
            username = extract_query_parameter(connection, "username=");
            room_id = extract_query_parameter(connection, "room_id=");
        } catch (std::runtime_error e) {
            delete *user_ptr_ptr;
            std::cout << "error: " << e.what() << std::endl;
            const char* msg = e.what();
            lws_close_reason(connection, LWS_CLOSE_STATUS_NORMAL, (unsigned char*)msg, strlen(msg));
            return -1;
        }

        // check the room exists
        if (!state.hasRoom(room_id)) {
            delete *user_ptr_ptr;
            const char* msg = "room doesn't exist";
            lws_close_reason(connection, LWS_CLOSE_STATUS_NORMAL, (unsigned char*)msg, strlen(msg));
            return -1;
        }

        bool is_user_connected_to_room = false;
        bool is_user_in_room = false;
        state.manipulateRoom(room_id, [&username, &is_user_connected_to_room, &is_user_in_room](RoomState& room) {
            is_user_connected_to_room = room.isUserConnectedToRoom(username);
            is_user_in_room = room.isUserInRoom(username);
        });

        if (!is_user_in_room) {
            delete *user_ptr_ptr;
            const char* msg = "user not in room";
            lws_close_reason(connection, LWS_CLOSE_STATUS_NORMAL, (unsigned char*)msg, strlen(msg));
            return -1;
        }

        if (is_user_connected_to_room) {
            delete *user_ptr_ptr;
            const char* msg = "user already connected";
            lws_close_reason(connection, LWS_CLOSE_STATUS_NORMAL, (unsigned char*)msg, strlen(msg));
            return -1;
        }

        state.manipulateRoom(room_id, [username, connection, user_ptr_ptr](RoomState& room) {
            room.manipulateUser(username, [connection, user_ptr_ptr](User& room_user) {
                room_user.socket = connection;
                room_user.is_connected = true;
            });
            // Store connection data
            delete *user_ptr_ptr;
            *user_ptr_ptr = room.getUserPtr(username);
        });

        break;
    }
    case LWS_CALLBACK_RECEIVE: {
        std::string message((const char*)in, len);
        User& user = **user_ptr_ptr;

        try {
            handleEvent(user, message);
        } catch (const std::exception& e) {
            std::cerr << "Error handling event: " << e.what() << std::endl;
        } catch (std::string s) {
            std::cerr << "Error handling event" << s << std::endl;
        }
        break;
    }
    case LWS_CALLBACK_CLOSED: {
        std::cout << "Client disconnected!" << std::endl;
        User& user = **user_ptr_ptr;
        // if it was temporary user
        if (user.room_id == "") {
            delete *user_ptr_ptr;
        }

        user.is_connected = false;

        break;
    }
    default:
        break;
    }
    return 0;
}

std::unique_ptr<CanvasObject> createCanvasObject(EventObjectType object_type)
{
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
    default:
        std::cout << "Unsupported object type!";
    }
    throw "unsupported object type!";
    return nullptr;
}

void WebSocketHandler::handleEvent(User& user, const std::string& message)
{
    std::cout << message << std::endl;
    nlohmann::json event = nlohmann::json::parse(message);

    auto object_type = static_cast<EventObjectType>(event["object_type"]);
    switch (object_type) {
    case ROOM: {
        auto event_type = static_cast<EventType>(event["event_type"]);

        throw std::logic_error("room creation and deletion is http only");
        break;
    }
    case PAGE: {
        auto event_type = static_cast<EventType>(event["event_type"]);
        switch (event_type) {
        case CREATE:
            state.manipulateRoom(event["room_id"], [event](RoomState& room) {
                room.applyInsertPageEvent(event);
            });
            break;
        case DELETE:
            state.manipulateRoom(event["room_id"], [event](RoomState& room) {
                room.applyDeletePageEvent(event);
            });
            break;
        }
        break;
    }
    case USER: {
        auto event_type = static_cast<User::UserEventType>(event["event_type"]);
        switch (event_type) {
        case User::UserEventType::CREATE:
            throw std::logic_error("only http request should create user");
            break;
        case User::UserEventType::DELETE:
            throw std::logic_error("can't delete user");
            break;
        default:
            state.manipulateRoom(event["room_id"], [event](RoomState& room) {
                room.manipulateUser(event["username"], [&](User& user) {
                    user.applyEvent(event);
                });
            });
            break;
        }
        break;
    }
    case STROKE:
    case SYMBOL:
    case SHAPE:
    case TEXT: {
        auto event_type = static_cast<CanvasObject::CanvasObjectEventType>(event["event_type"]);
        switch (event_type) {
        case CanvasObject::CanvasObjectEventType::CREATE:
            state.manipulateRoom(event["room_id"], [event, object_type](RoomState& room) {
                room.manipulatePage(event["page_id"], [event, object_type](Page& page) mutable {
                    std::unique_ptr<CanvasObject> object = createCanvasObject(object_type);
                    object->fromJson(event);
                    page.addObject(std::move(object));
                });
            });
            break;
        case CanvasObject::CanvasObjectEventType::DELETE:
            state.manipulateRoom(event["room_id"], [event](RoomState& room) {
                uint64_t object_id = event["object_id"];
                room.manipulatePage(event["page_id"], [object_id](Page& page) {
                    page.deleteObject(object_id);
                });
            });
            break;
        default:
            state.manipulateRoom(event["room_id"], [event](RoomState& room) {
                uint64_t object_id = event["object_id"];
                room.manipulatePage(event["page_id"], [object_id, event](Page& page) {
                    page.manipulateObject(object_id,
                        [event](CanvasObject& canvas_object) {
                            canvas_object.applyEvent(event);
                        });
                });
            });
            break;
        }
        break;
    }
    }

    // forward messages to everyone else in the room
    state.manipulateRoom(event["room_id"], [&](RoomState& room) {
        room.forEachUser([&](const User& other_user) {
            if (user == other_user) {
                return;
            }
            other_user.sendEvent(message);
        });
    });
}
