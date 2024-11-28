#pragma once

#include "CanvasObject.hpp"
#include "nlohmann/json.hpp"
#include <vector>
#include <cmath>
#ifdef NOTEWORTHY_QT
#include <QGraphicsSvgItem>
#endif


class Symbol : public CanvasObject {
public:
    virtual ~Symbol() = default;

    std::vector<double> top_left;
    std::vector<double> bottom_left;
    std::vector<double> bottom_right;

    enum SymbolType {
        CAPACITOR,
        RESISTOR,
        INDUCTOR,
        BATTERY,
        DIODE,
        SWITCH,
        AC_SOURCE,
        DC_SOURCE,
        CURRENT_SOURCE,
    };

#ifdef NOTEWORTHY_QT
    const static std::unordered_map<SymbolType, QString> symbolSvgPaths;
#endif

    enum SymbolType symbol_type;

    virtual EventObjectType getObjectType() {return SYMBOL;};

    virtual void toJson(nlohmann::json& json) {
        addMetaInformation(json);
        json["top_left"] = top_left;
        json["bottom_left"] = bottom_left;
        json["bottom_right"] = bottom_right;
        json["symbol_type"] = symbol_type;
    };

    virtual void fromJson(const nlohmann::json& json) {
        retrieveMetaInformation(json);
        json.at("top_left").get_to(top_left);
        json.at("bottom_left").get_to(bottom_left);
        json.at("bottom_right").get_to(bottom_right);
        json.at("symbol_type").get_to(symbol_type);
#ifdef NOTEWORTHY_QT
        updateQtScene();
#endif
    };

    virtual void applyMoveEvent(double distance_x, double distance_y)
    {
        std::vector<std::vector<double>> points = {top_left, bottom_left, bottom_right};
        for (auto &point : points)
        {
            point[0] += distance_x;
            point[1] += distance_y;
        }
    }

    virtual void applyScaleEvent(double scale_center_x, double scale_center_y,
        double scale_factor_x, double scale_factor_y)
    {
        std::vector<std::vector<double>> points = {top_left, bottom_left, bottom_right};
        for (auto &point : points)
        {
            point[0] += (scale_center_x - point[0]) * scale_factor_x;
            point[1] += (scale_center_y - point[1]) * scale_factor_y;
        }
    }

private:
    void rotatePoint(double cx, double cy, double angle_degrees,
        std::vector<double> &p)
    {
        double angle_radians = M_PI / 180 * angle_degrees;
        p[0] = cos(angle_radians) * (p[0] - cx) -
            sin(angle_radians) * (p[1] - cy) + cx;
        p[1] = sin(angle_radians) * (p[0] - cx) +
            cos(angle_radians) * (p[1] - cy) + cy;
    }

public:
    virtual void applyRotateEvent(double rotation_center_x,
        double rotation_center_y,
        double rotation_degrees)
    {
        std::vector<std::vector<double>> points = {top_left, bottom_left, bottom_right};
        for (auto &point : points)
        {
            rotatePoint(rotation_center_x, rotation_center_y, rotation_degrees,
                point);
        }
    }

#ifdef NOTEWORTHY_QT
    Symbol(SymbolType type, QGraphicsSvgItem *symbol_svg)
        : symbol_type(type) {
        item = static_cast<QGraphicsItem*>(symbol_svg);
    };

    Symbol(SymbolType type): symbol_type(type) {};

    void updateQtScene()
    {
        auto svg_item = static_cast<QGraphicsSvgItem*>(item);
        // set the position of the svg item
        // Extract the points
        QPointF top_left_point(top_left[0], top_left[1]);
        QPointF bottom_left_point(bottom_left[0], bottom_left[1]);
        QPointF bottom_right_point(bottom_right[0], bottom_right[1]);

        svg_item->setPos(top_left_point);

        double width = std::hypot(bottom_right_point.x() - bottom_left_point.x(),
            bottom_right_point.y() - bottom_left_point.y());
        double height = std::hypot(bottom_left_point.x() - top_left_point.x(),
            bottom_left_point.y() - top_left_point.y());
        svg_item->setScale(1.0); // Reset scaling before applying a transformation

        double angle_radians = std::atan2(bottom_left_point.y() - top_left_point.y(),
            bottom_left_point.x() - top_left_point.x());
        double angle_degrees = angle_radians * 180.0 / M_PI;

        QTransform transform;
        transform.translate(top_left_point.x(), top_left_point.y());
        transform.rotate(angle_degrees);
        transform.scale(width / svg_item->boundingRect().width(),
            height / svg_item->boundingRect().height());
        svg_item->setTransform(transform, false);
    }
#endif
};


