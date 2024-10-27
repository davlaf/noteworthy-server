#pragma once
#include "nlohmann/json.hpp"

class CanvasObject {
  public:
    std::string owner_id;
    std::string room_id;
    uint64_t page_id;
    uint64_t id;

    virtual ~CanvasObject() = default;

    enum EventType {
        CREATE,
        DELETE,
        MOVE,
        SCALE,
        ROTATE,
        APPEND,
        EDIT,
    };

    // I don't like this, you have to edit the parent class to add
    // a child class, but I can't think of another way to do it
    // without hard coding object type strings in code
    // that depends on the type field
    // so I think this the least worst solution
    enum ObjectType {
        STROKE,
        SYMBOL,
        SHAPE,
        TEXT,
        BACKGROUND_IMAGE,
    };

    virtual ObjectType getObjectType() = 0;

    virtual void toJson(nlohmann::json &json) = 0;
    virtual void fromJson(const nlohmann::json &json) = 0;

    void addMetaInformation(nlohmann::json &json) {
        json["owner_id"] = owner_id;
        json["room_id"] = room_id;
        json["page_id"] = page_id;
        json["object_type"] = getObjectType();
        json["object_id"] = id;
    };

    void retrieveMetaInformation(const nlohmann::json &json) {
        json.at("owner_id").get_to(owner_id);
        json.at("room_id").get_to(room_id);
        json.at("page_id").get_to(page_id);
        json.at("object_id").get_to(id);
    };

    // function that gets called after a change gets applied
    // only needs to be implemented for qt side
    virtual void updateQtScene() {};

    void createCreateEvent(nlohmann::json &json) {
        toJson(json);
        json["event_type"] = CREATE;
    }

    void createDeleteEvent(nlohmann::json &json) {
        addMetaInformation(json);
        json["event_type"] = DELETE;
    }

    void createMoveEvent(nlohmann::json &json, double distance_x,
                         double distance_y) {
        addMetaInformation(json);
        json["event_type"] = MOVE;
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
        json["event_type"] = SCALE;
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
        json["event_type"] = ROTATE;
        json["rotation_center_x"] = rotation_center_x;
        json["rotation_center_y"] = rotation_center_y;
        json["rotation_degrees"] = rotation_degrees;
    }
    virtual void applyRotateEvent(double rotation_center_x,
                                  double rotation_center_y,
                                  double rotation_degrees) {
        throw std::logic_error("Applying rotate not implemented.");
    }

  protected:
    virtual void onCreateAppendEvent(nlohmann::json &json) {
        throw std::logic_error("Creating append not implemented.");
    }

  public:
    virtual void applyAppendEvent(const nlohmann::json &change) {
        throw std::logic_error("Applying append not implemented.");
    }

  protected:
    virtual void onCreateEditEvent(nlohmann::json &json) {
        throw std::logic_error("Creating edit not implemented.");
    }

  public:
    virtual void applyEditEvent(const nlohmann::json &change) {
        throw std::logic_error("Applying edit not implemented.");
    }

    void applyEvent(const nlohmann::json &json) {
        int event_type = json["event_type"];
        switch (event_type) {
        case MOVE: {
            double distance_x = json["distance_x"];
            double distance_y = json["distance_y"];
            applyMoveEvent(distance_x, distance_y);
            break;
        }
        case SCALE: {
            double scale_center_x = json["scale_center_x"];
            double scale_center_y = json["scale_center_y"];
            double scale_factor_x = json["scale_factor_x"];
            double scale_factor_y = json["scale_factor_y"];
            applyScaleEvent(scale_center_x, scale_center_y, scale_factor_x,
                            scale_factor_y);
            break;
        }
        case ROTATE: {
            double rotation_center_x = json["rotation_center_x"];
            double rotation_center_y = json["rotation_center_y"];
            double rotation_degrees = json["rotation_degrees"];
            applyRotateEvent(rotation_center_x, rotation_center_y,
                             rotation_degrees);
            break;
        }
        case APPEND: {
            applyAppendEvent(json);
            break;
        }
        case EDIT: {
            applyEditEvent(json);
            break;
        }
        default: {
            throw std::range_error("invalid canvas object");
        }
        }
        updateQtScene();
    };
};
