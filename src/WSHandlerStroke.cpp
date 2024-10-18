#include "WebSocketHandler.hpp"
#include "nlohmann/json.hpp"

#include "RoomState.hpp"
#include "Stroke.hpp"
#include "UserConnection.hpp"

using json = nlohmann::json; // Define a shorthand for the json type

void WebSocketHandler::handleNewStroke(UserConnection &user,
                                       const json &event) {

    Stroke stroke;
    stroke.from_json(event, stroke);
    Object stroke_container = stroke;

    // objects.push_back(stroke_container);
    std::string event_string = event.dump();

    for (auto other_user : ws_connections) {
        if (other_user.second == user)
            continue;

        other_user.second.sendEvent(event_string);
    }

    // user.sendEvent(R"({"msg": "handleNewStroke"})");
};

void WebSocketHandler::handleMoveStroke(UserConnection &user,
                                        const json &event) {
    user.sendEvent(R"({"msg": "handleMoveStroke"})");
};

void WebSocketHandler::handleDeleteStroke(UserConnection &user,
                                          const json &event) {
    user.sendEvent(R"({"msg": "handleDeleteStroke"})");
};