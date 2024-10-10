
#include "WebSocketHandler.hpp"
#include "nlohmann/json.hpp"

#include "UserConnection.hpp"

using json = nlohmann::json; // Define a shorthand for the json type

void WebSocketHandler::handleNewShape(UserConnection &user, const json &event) {
    user.sendEvent(R"({"msg":"handleNewShape"})");
}
void WebSocketHandler::handleTransformShape(UserConnection &user,
                                            const json &event) {
    user.sendEvent(R"({"msg":"handleTransformShape"})");
}
void WebSocketHandler::handleDeleteShape(UserConnection &user,
                                         const json &event) {
    user.sendEvent(R"({"msg":"handleDeleteShape"})");
}