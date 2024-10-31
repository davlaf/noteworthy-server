#pragma once
#include "EventTypeEnums.hpp"
#include "nlohmann/json.hpp"

class CanvasObject {
  public:
    std::string owner_id;
    std::string room_id;
    uint64_t page_id;
    uint64_t object_id;

    virtual ~CanvasObject() = default;

    virtual EventObjectType getObjectType() = 0;

    virtual void toJson(nlohmann::json &json) = 0;
    virtual void fromJson(const nlohmann::json &json) = 0;

    void addMetaInformation(nlohmann::json &json) {
        json["owner_id"] = owner_id;
        json["room_id"] = room_id;
        json["page_id"] = page_id;
        json["object_type"] = getObjectType();
        json["object_id"] = object_id;
    };

    void retrieveMetaInformation(const nlohmann::json &json) {
        json.at("owner_id").get_to(owner_id);
        json.at("room_id").get_to(room_id);
        json.at("page_id").get_to(page_id);
        json.at("object_id").get_to(object_id);
    };

    // function that gets called after a change gets applied
    // only needs to be implemented for qt side
#ifdef NOTEWORTHY_QT
    virtual void updateQtScene() = 0;
#endif
    void createCreateEvent(nlohmann::json &json) {
        toJson(json);
        json["event_type"] = EventType::CREATE;
    }

    void createDeleteEvent(nlohmann::json &json) {
        addMetaInformation(json);
        json["event_type"] = EventType::DELETE;
    }

    void createMoveEvent(nlohmann::json &json, double distance_x,
                         double distance_y) {
        addMetaInformation(json);
        json["event_type"] = EventType::MOVE;
        json["distance_x"] = distance_x;
        json["distance_y"] = distance_y;
    }
    virtual void applyMoveEvent(double distance_x, double distance_y) {
        throw std::logic_error("Applying move not implemented.");
    }

    void createScaleEvent(nlohmann::json &json, double scale_center_x,
                          double scale_center_y, double scale_factor_x,
                          double scale_factor_y) {
        addMetaInformation(json);
        json["event_type"] = EventType::SCALE;
        json["scale_center_x"] = scale_center_x;
        json["scale_center_y"] = scale_center_y;
        json["scale_factor_x"] = scale_factor_x;
        json["scale_factor_y"] = scale_factor_y;
    }
    virtual void applyScaleEvent(double scale_center_x, double scale_center_y,
                                 double scale_factor_x, double scale_factor_y) {
        throw std::logic_error("Applying scale not implemented.");
    }

    void createRotateEvent(nlohmann::json &json, double rotation_center_x,
                           double rotation_center_y, double rotation_degrees) {
        addMetaInformation(json);
        json["event_type"] = EventType::ROTATE;
        json["rotation_center_x"] = rotation_center_x;
        json["rotation_center_y"] = rotation_center_y;
        json["rotation_degrees"] = rotation_degrees;
    }
    virtual void applyRotateEvent(double rotation_center_x,
                                  double rotation_center_y,
                                  double rotation_degrees) {
        throw std::logic_error("Applying rotate not implemented.");
    }

    virtual void applyAppendEvent(const nlohmann::json &change) {
        throw std::logic_error("Applying append not implemented.");
    }

    virtual void applyEditEvent(const nlohmann::json &change) {
        throw std::logic_error("Applying edit not implemented.");
    }

    void applyEvent(const nlohmann::json &json) {
        int event_type = json["event_type"];
        switch (event_type) {
        case EventType::MOVE: {
            double distance_x = json["distance_x"];
            double distance_y = json["distance_y"];
            applyMoveEvent(distance_x, distance_y);
            break;
        }
        case EventType::SCALE: {
            double scale_center_x = json["scale_center_x"];
            double scale_center_y = json["scale_center_y"];
            double scale_factor_x = json["scale_factor_x"];
            double scale_factor_y = json["scale_factor_y"];
            applyScaleEvent(scale_center_x, scale_center_y, scale_factor_x,
                            scale_factor_y);
            break;
        }
        case EventType::ROTATE: {
            double rotation_center_x = json["rotation_center_x"];
            double rotation_center_y = json["rotation_center_y"];
            double rotation_degrees = json["rotation_degrees"];
            applyRotateEvent(rotation_center_x, rotation_center_y,
                             rotation_degrees);
            break;
        }
        case EventType::APPEND: {
            applyAppendEvent(json);
            break;
        }
        case EventType::EDIT: {
            applyEditEvent(json);
            break;
        }
        default: {
            throw std::range_error("invalid canvas object");
        }
        }
#ifdef NOTEWORTHY_QT
        updateQtScene();
#endif
    };
};
