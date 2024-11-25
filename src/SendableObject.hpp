#pragma once
#include "EventTypeEnums.hpp"
#include "nlohmann/json.hpp"

#ifdef NOTEWORTHY_QT
#include <qgraphicsitem.h>
#endif

class SendableObject {
public:
    std::string room_id;

    enum EventType {
        CREATE,
        DELETE,
    };

    virtual ~SendableObject() = default;

    SendableObject() = default;

    SendableObject(const std::string& room_id)
        : room_id(room_id) {};

    virtual EventObjectType getObjectType() = 0;

    virtual void toJson(nlohmann::json& json) = 0;
    virtual void fromJson(const nlohmann::json& json) = 0;

    virtual void addMetaInformation(nlohmann::json& json)
    {
        json["room_id"] = room_id;
        json["object_type"] = getObjectType();
    };

    virtual void retrieveMetaInformation(const nlohmann::json& json)
    {
        json.at("room_id").get_to(room_id);
    };

    // function that gets called after a change gets applied
    // only needs to be implemented for qt side
#ifdef NOTEWORTHY_QT
    virtual void updateQtScene() = 0;
#endif
    void createCreateEvent(nlohmann::json& json)
    {
        toJson(json);
        json["event_type"] = EventType::CREATE;
    }

    void createDeleteEvent(nlohmann::json& json)
    {
        addMetaInformation(json);
        json["event_type"] = EventType::DELETE;
    }

    virtual void applyEvent(const nlohmann::json& json) = 0;
};
