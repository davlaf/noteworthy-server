#pragma once
#include "EventTypeEnums.hpp"
#include "SendableObject.hpp"
#include "nlohmann/json.hpp"

#ifdef NOTEWORTHY_QT
#include <qgraphicsitem.h>
#endif

class CanvasObject : public SendableObject {
public:
    std::string owner_id;
    uint64_t page_id;
    uint64_t object_id;
#ifdef NOTEWORTHY_QT
    QGraphicsItem* item;
#endif
    enum CanvasObjectEventType {
        CREATE, // IMPORTANT DO NOT MOVE
        DELETE, // IMPORTANT DO NOT MOVE
        MOVE,
        SCALE,
        ROTATE,
        APPEND,
        EDIT
    };

    virtual ~CanvasObject() = default;

    void addMetaInformation(nlohmann::json& json) override
    {
        json["owner_id"] = owner_id;
        json["page_id"] = page_id;
        json["object_id"] = object_id;
        SendableObject::addMetaInformation(json);
    };

    void retrieveMetaInformation(const nlohmann::json& json) override
    {
        json.at("owner_id").get_to(owner_id);
        json.at("page_id").get_to(page_id);
        json.at("object_id").get_to(object_id);
        SendableObject::retrieveMetaInformation(json);
    };

    void createMoveEvent(nlohmann::json& json, double distance_x,
        double distance_y)
    {
        addMetaInformation(json);
        json["event_type"] = CanvasObjectEventType::MOVE;
        json["distance_x"] = distance_x;
        json["distance_y"] = distance_y;
    }
    virtual void applyMoveEvent(double distance_x, double distance_y)
    {
        throw std::logic_error("Applying move not implemented.");
    }

    void createScaleEvent(nlohmann::json& json, double scale_center_x,
        double scale_center_y, double scale_factor_x,
        double scale_factor_y)
    {
        addMetaInformation(json);
        json["event_type"] = CanvasObjectEventType::SCALE;
        json["scale_center_x"] = scale_center_x;
        json["scale_center_y"] = scale_center_y;
        json["scale_factor_x"] = scale_factor_x;
        json["scale_factor_y"] = scale_factor_y;
    }
    virtual void applyScaleEvent(double scale_center_x, double scale_center_y,
        double scale_factor_x, double scale_factor_y)
    {
        throw std::logic_error("Applying scale not implemented.");
    }

    void createRotateEvent(nlohmann::json& json, double rotation_center_x,
        double rotation_center_y, double rotation_degrees)
    {
        addMetaInformation(json);
        json["event_type"] = CanvasObjectEventType::ROTATE;
        json["rotation_center_x"] = rotation_center_x;
        json["rotation_center_y"] = rotation_center_y;
        json["rotation_degrees"] = rotation_degrees;
    }
    virtual void applyRotateEvent(double rotation_center_x,
        double rotation_center_y,
        double rotation_degrees)
    {
        throw std::logic_error("Applying rotate not implemented.");
    }

    virtual void applyAppendEvent(const nlohmann::json& change)
    {
        throw std::logic_error("Applying append not implemented.");
    }

    virtual void applyEditEvent(const nlohmann::json& change)
    {
        throw std::logic_error("Applying edit not implemented.");
    }

    virtual void applyEvent(const nlohmann::json& json) override
    {
        auto event_type = static_cast<CanvasObjectEventType>(json["event_type"]);
        switch (event_type) {
        case CanvasObjectEventType::MOVE: {
            double distance_x = json["distance_x"];
            double distance_y = json["distance_y"];
            applyMoveEvent(distance_x, distance_y);
            break;
        }
        case CanvasObjectEventType::SCALE: {
            double scale_center_x = json["scale_center_x"];
            double scale_center_y = json["scale_center_y"];
            double scale_factor_x = json["scale_factor_x"];
            double scale_factor_y = json["scale_factor_y"];
            applyScaleEvent(scale_center_x, scale_center_y, scale_factor_x,
                scale_factor_y);
            break;
        }
        case CanvasObjectEventType::ROTATE: {
            double rotation_center_x = json["rotation_center_x"];
            double rotation_center_y = json["rotation_center_y"];
            double rotation_degrees = json["rotation_degrees"];
            applyRotateEvent(rotation_center_x, rotation_center_y,
                rotation_degrees);
            break;
        }
        case CanvasObjectEventType::APPEND: {
            applyAppendEvent(json);
            break;
        }
        case CanvasObjectEventType::EDIT: {
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
