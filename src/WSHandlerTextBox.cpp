#include "WebSocketHandler.hpp"
#include "nlohmann/json.hpp"

#include "UserConnection.hpp"

using json = nlohmann::json; // Define a shorthand for the json type

void WebSocketHandler::handleNewTextBox(UserConnection &user,
                                        const json &event) {
    user.sendEvent(R"({"msg":"handleNewTextBox"})");
};
void WebSocketHandler::handleSetCursorTextBox(UserConnection &user,
                                              const json &event) {
    user.sendEvent(R"({"msg":"handleSetCursorTextBox"})");
};
void WebSocketHandler::handleDeleteCursorTextBox(UserConnection &user,
                                                 const json &event) {
    user.sendEvent(R"({"msg":"handleDeleteCursorTextBox"})");
};
void WebSocketHandler::handleEditTextBox(UserConnection &user,
                                         const json &event) {
    user.sendEvent(R"({"msg":"handleEditTextBox"})");
};
void WebSocketHandler::handleTransformTextBox(UserConnection &user,
                                              const json &event) {
    user.sendEvent(R"({"msg":"handleTransformTextBox"})");
};
void WebSocketHandler::handleDeleteTextBox(UserConnection &user,
                                           const json &event) {
    user.sendEvent(R"({"msg":"handleDeleteTextBox"})");
};