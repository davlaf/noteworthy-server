#include "WebSocketHandler.hpp"
#include "nlohmann/json.hpp"

#include "UserConnection.hpp"

using json = nlohmann::json; // Define a shorthand for the json type

void WebSocketHandler::handleNewStroke(UserConnection &user,
                                       const json &event) {
    user.sendEvent(R"({"msg": "handleNewStroke"})");
};

void WebSocketHandler::handleMoveStroke(UserConnection &user,
                                        const json &event) {
    user.sendEvent(R"({"msg": "handleMoveStroke"})");
};

void WebSocketHandler::handleDeleteStroke(UserConnection &user,
                                          const json &event) {
    user.sendEvent(R"({"msg": "handleDeleteStroke"})");
};