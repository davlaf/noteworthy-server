#include "WebSocketHandler.hpp"
#include "nlohmann/json.hpp"
#include "RoomState.hpp"
#include "Symbol.hpp"
#include "UserConnection.hpp"

using json = nlohmann::json; // Define a shorthand for the json type

void WebSocketHandler::handleNewSymbol(UserConnection &user,
                                       const json &event) {

    Symbol symbol;
    symbol.from_json(event, symbol);
    Object symbol_container = symbol;


    user.sendEvent(R"({"msg": "handleNewSymbol"})");
};

void WebSocketHandler::handleTransformSymbol(UserConnection &user,
                                             const json &event) {

    //gets id of the transformed symbol, find the "new" size of the symbol, new rotation and new position                                            

    user.sendEvent(R"({"msg": "handleTransformSymbol"})");
};

void WebSocketHandler::handleDeleteSymbol(UserConnection &user,
                                          const json &event) {
    //get the id of the symbol and then delete symbol                                        
    user.sendEvent(R"({"msg": "handleDeleteSymbol"})");
};