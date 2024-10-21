
#include "WebSocketHandler.hpp"
#include "nlohmann/json.hpp"
#include "RoomState.hpp"
#include "Shape.hpp"
#include "UserConnection.hpp"

using json = nlohmann::json; // Define a shorthand for the json type

void WebSocketHandler::handleNewShape(UserConnection &user, const json &event) {

    Shape shape;
    shape.from_json(event, shape);
    Object shapeContainer = shape;


    user.sendEvent(R"({"msg":"handleNewShape"})");
}
void WebSocketHandler::handleTransformShape(UserConnection &user,
                                            const json &event) {   

    // uint64_t shape_id = event["shape_id"];
    // std::vector<double> new_position = event["new_position"];
    // std::vector<double> new_size = event["new_size"];
    // double new_rotation = event["new_rotation"];                                        
    // user.sendEvent(R"({"msg":"handleTransformShape"})");
}
void WebSocketHandler::handleDeleteShape(UserConnection &user,
                                         const json &event) {
    user.sendEvent(R"({"msg":"handleDeleteShape"})");
}