#pragma once
#include "nlohmann/json.hpp"
#include <libwebsockets.h>
#include <list>
#include <string>
using json = nlohmann::json; // Define a shorthand for the json type
#define EVENT_PARAMS UserConnection &user, const json &event

class WebSocketHandler {
  public:
    static std::map<struct lws *, struct UserConnection> ws_connections;

    static int startServer(int port);
    static void removeConnection(struct lws *connection);
    static int callbackEcho(struct lws *connection,
                            enum lws_callback_reasons reason, void *user,
                            void *in, size_t len);
    static void handleEvent(UserConnection &user, const std::string &event);
    enum EventType {
        NEW_STROKE,
        MOVE_STROKE,
        DELETE_STROKE,

        NEW_TEXT_BOX,
        SET_CURSOR_TEXT_BOX,
        DELETE_CURSOR_TEXT_BOX,
        EDIT_TEXT_BOX,
        TRANSFORM_TEXT_BOX,
        DELETE_TEXT_BOX,

        NEW_SHAPE,
        TRANSFORM_SHAPE,
        DELETE_SHAPE,

        NEW_SYMBOL,
        TRANSFORM_SYMBOL,
        DELETE_SYMBOL,
    };

    static void handleNewStroke(EVENT_PARAMS);
    static void handleMoveStroke(EVENT_PARAMS);
    static void handleDeleteStroke(EVENT_PARAMS);

    static void handleNewTextBox(EVENT_PARAMS);
    static void handleSetCursorTextBox(EVENT_PARAMS);
    static void handleDeleteCursorTextBox(EVENT_PARAMS);
    static void handleEditTextBox(EVENT_PARAMS);
    static void handleTransformTextBox(EVENT_PARAMS);
    static void handleDeleteTextBox(EVENT_PARAMS);

    static void handleNewShape(EVENT_PARAMS);
    static void handleTransformShape(EVENT_PARAMS);
    static void handleDeleteShape(EVENT_PARAMS);

    static void handleNewSymbol(EVENT_PARAMS);
    static void handleTransformSymbol(EVENT_PARAMS);
    static void handleDeleteSymbol(EVENT_PARAMS);
};