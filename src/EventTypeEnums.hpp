#pragma once

enum EventType {
    CREATE,
    DELETE,
    MOVE,
    SCALE,
    ROTATE,
    APPEND,
    EDIT,
};

enum EventObjectType {
    ROOM,
    PAGE,
    STROKE,
    SYMBOL,
    SHAPE,
    TEXT,
    BACKGROUND_IMAGE,
};
