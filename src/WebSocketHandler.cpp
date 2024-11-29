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
    lws_set_log_level(0, NULL);
    // lws_set_log_level(0b11111111111, NULL);

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

void close_connection(struct lws* connection, const std::string& message)
{
    std::cout << "closing connection: " << message << std::endl;
    size_t message_length = message.length();
    size_t payload_length = 2 + message_length; // 2 bytes for the close code + message length
    unsigned char buf[LWS_PRE + payload_length];

    // Construct the close frame
    buf[LWS_PRE] = 0x03; // Close code (e.g., 1000: Normal Closure)
    buf[LWS_PRE + 1] = 0xE8; // Status code 1000 in network byte order

    // Copy optional message payload, if provided
    if (!message.empty()) {
        memcpy(&buf[LWS_PRE + 2], message.c_str(), message_length);
    }

    // Write the frame
    lws_write(connection, &buf[LWS_PRE], payload_length, static_cast<lws_write_protocol>(4));
}

int WebSocketHandler::callbackEcho(
    struct lws* connection,
    enum lws_callback_reasons reason,
    void* user,
    void* in,
    size_t len)
{
    auto user_ptr_ptr = static_cast<User**>(user);
    switch (reason) {
    case LWS_CALLBACK_SERVER_WRITEABLE: {
        User& user = **user_ptr_ptr;
        if (user.room_id == "") {
            close_connection(connection, "not allowed to send message");
            return -1;
        }
        break;
    }

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
            std::cout << "error: " << e.what() << std::endl;
            close_connection(connection, "missing username and/or room_id param");
            return -1;
        }

        if (username == "") {
            close_connection(connection, "missing username");
            return -1;
        }

        if (room_id == "") {
            close_connection(connection, "missing room_id");
            return -1;
        }

        // check the room exists
        if (!state.hasRoom(room_id)) {
            close_connection(connection, "room doesn't exist");
            return -1;
        }

        bool is_user_connected_to_room = false;
        bool is_user_in_room = false;
        state.manipulateRoom(room_id, [&username, &is_user_connected_to_room, &is_user_in_room](RoomState& room) {
            is_user_connected_to_room = room.isUserConnectedToRoom(username);
            is_user_in_room = room.isUserInRoom(username);
        });

        if (!is_user_in_room) {
            close_connection(connection, "user not in room");
            return -1;
        }

        if (is_user_connected_to_room) {
            close_connection(connection, "user already connected");
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
        if (user.room_id == "") {
            close_connection(connection, "not assigned a user; not allowed to send message");
            return -1;
        }

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
            break;
        }

        user.is_connected = false;
        // // check if no one is in room anymore and delete it
        // // risky for demo should add some timer thing
        // bool is_anyone_connected = false;
        // state.manipulateRoom(user.room_id, [&is_anyone_connected](RoomState& room) {
        //     room.forEachUser([&is_anyone_connected](const User& room_user) {
        //         if (room_user.is_connected) {
        //             is_anyone_connected = true;
        //         }
        //     });
        // });
        // if (!is_anyone_connected) {
        //     std::cout << "deleting room " << user.room_id << " for inactivity" << std::endl;
        //     state.deleteRoom(user.room_id);
        // }

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
    case SYMBOL: {
        auto symbol = std::make_unique<Symbol>();
        return std::move(symbol);
    }
    case SHAPE: {
        auto shape = std::make_unique<Shape>();
        return std::move(shape);
    }
    case TEXT: {
        std::cout << "Text creation not supported." << std::endl;
        break;
    }
    default: {
        std::cout << "Unsupported object type!";
    }}
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
        auto event_type = static_cast<RoomState::RoomEventType>(event["event_type"]);
        switch (event_type) {
        case RoomState::RoomEventType::CREATE: {
            throw std::runtime_error("should only create room with http");
            break;
        }
        case RoomState::RoomEventType::DELETE: {
            throw std::runtime_error("shouldn't be able to delete room");
            break;
        }
        default: {
            state.manipulateRoom(event["room_id"], [&event](RoomState& room) {
                room.applyEvent(event);
            });
            break;
        }
        }
        break;
    }
    case PAGE: {
        auto event_type = static_cast<Page::PageEventType>(event["event_type"]);
        switch (event_type) {
        case Page::PageEventType::CREATE: {
            throw std::runtime_error("can only insert page not create");
            break;
        }
        case Page::PageEventType::DELETE: {
            state.manipulateRoom(event["room_id"], [&event](RoomState& room) {
                room.deletePage(event["page_id"]);
            });
            break;
        }
        case Page::PageEventType::INSERT: {
            state.manipulateRoom(event["room_id"], [&event](RoomState& room) {
                room.applyInsertPageEvent(event);
            });
            break;
        }
        case Page::PageEventType::INSERT_PDF: {
            throw std::runtime_error("should only be done with http");
            break;
        }
        default: {
            state.manipulateRoom(event["room_id"], [&event](RoomState& room) {
                room.manipulatePage(event["page_id"], [&](Page& page) {
                    page.applyEvent(event);
                });
            });
        }
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
            state.manipulateRoom(event["room_id"], [&event](RoomState& room) {
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
            state.manipulateRoom(event["room_id"], [&event, object_type](RoomState& room) {
                room.manipulatePage(event["page_id"], [event, object_type](Page& page) mutable {
                    std::unique_ptr<CanvasObject> object = createCanvasObject(object_type);
                    object->fromJson(event);
                    page.addObject(std::move(object));
                });
            });
            break;
        case CanvasObject::CanvasObjectEventType::DELETE:
            state.manipulateRoom(event["room_id"], [&event](RoomState& room) {
                uint64_t object_id = event["object_id"];
                room.manipulatePage(event["page_id"], [object_id](Page& page) {
                    page.deleteObject(object_id);
                });
            });
            break;
        default:
            state.manipulateRoom(event["room_id"], [&event](RoomState& room) {
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
    case RESET: {
        throw std::runtime_error("shouldn't get reset signal");
    }
    }

    // forward messages to everyone else in the room
    state.manipulateRoom(event["room_id"], [&](RoomState& room) {
        room.forEachUser([&](const User& other_user) {
            if (user == other_user) {
                return;
            }

            if (!other_user.is_connected) {
                return;
            }

            if (other_user.is_connected && other_user.socket == nullptr) {
                throw "AAAA user is connected but their socket is null";
            }

            other_user.sendEvent(message);
        });
    });
}
