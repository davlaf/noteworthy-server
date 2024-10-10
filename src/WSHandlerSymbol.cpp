#include "WebSocketHandler.hpp"
#include "nlohmann/json.hpp"

#include "UserConnection.hpp"

using json = nlohmann::json; // Define a shorthand for the json type

void WebSocketHandler::handleNewSymbol(UserConnection &user,
                                       const json &event) {
    user.sendEvent(R"({"msg": "handleNewSymbol"})");
};

void WebSocketHandler::handleTransformSymbol(UserConnection &user,
                                             const json &event) {
    user.sendEvent(R"({"msg": "handleTransformSymbol"})");
};

void WebSocketHandler::handleDeleteSymbol(UserConnection &user,
                                          const json &event) {
    user.sendEvent(R"({"msg": "handleDeleteSymbol"})");
};