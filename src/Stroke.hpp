#pragma once
#ifdef NOTEWORTHY_QT
#include <qgraphicsitem.h>
#include <qpainterpath.h>
#endif
#include "CanvasObject.hpp"
#include "EventTypeEnums.hpp"
#include <vector>

#include "nlohmann/json.hpp"
#include <math.h>

class Stroke : public CanvasObject {
  private:
    std::vector<std::vector<double>> points;
#ifdef NOTEWORTHY_QT
  public:
    QPainterPath path;
    QGraphicsPathItem *path_item;
#endif

  public:
    EventObjectType getObjectType() { return STROKE; };

    virtual void toJson(nlohmann::json &json) {
        addMetaInformation(json);
        json["points"] = points;
    }

    virtual void fromJson(const nlohmann::json &json) {
        retrieveMetaInformation(json);
        json.at("points").get_to(points);
    }

    virtual void applyMoveEvent(double distance_x, double distance_y) {
        for (auto &point : points) {
            point[0] += distance_x;
            point[1] += distance_y;
        }
    }

    virtual void applyScaleEvent(double scale_center_x, double scale_center_y,
                                 double scale_factor_x, double scale_factor_y) {
        for (auto &point : points) {
            point[0] += (scale_center_x - point[0]) * scale_factor_x;
            point[1] += (scale_center_y - point[1]) * scale_factor_y;
        }
    }

  private:
    void rotatePoint(double cx, double cy, double angle_degrees,
                     std::vector<double> &p) {
        double angle_radians = M_PI / 180 * angle_degrees;
        p[0] = cos(angle_radians) * (p[0] - cx) -
               sin(angle_radians) * (p[1] - cy) + cx;
        p[1] = sin(angle_radians) * (p[0] - cx) +
               cos(angle_radians) * (p[1] - cy) + cy;
    }

  public:
    virtual void applyRotateEvent(double rotation_center_x,
                                  double rotation_center_y,
                                  double rotation_degrees) {
        for (auto &point : points) {
            rotatePoint(rotation_center_x, rotation_center_y, rotation_degrees,
                        point);
        }
    }

    virtual void
    createAppendEvent(nlohmann::json &json,
                      std::vector<std::vector<double>> new_points) {
        addMetaInformation(json);
        json["event_type"] = APPEND;
        json["new_points"] = new_points;
    }

    virtual void applyAppendEvent(const nlohmann::json &change) {
        std::vector<std::vector<double>> new_points = change["new_points"];
        for (auto &point : new_points) {
            points.push_back(point);
        }
    }

#ifdef NOTEWORTHY_QT
    Stroke(QPainterPath &path, QGraphicsPathItem *path_item)
        : path(path), path_item(path_item) {};

    Stroke(QPainterPath &path) : path(path) {};

    Stroke() { qDebug("Created empty stroke??? (a copy happened) (bad)"); };

    ~Stroke() // TODO: look into this
    {
        // free(path); ??
    }

    void updateQtPath() {
        path.clear();
        std::vector<double> starting_vector = points.at(0);
        QPointF starting_point{starting_vector.at(0), starting_vector.at(1)};
        path.moveTo(starting_point);

        for (size_t i = 1; i < points.size(); i++) {
            QPointF next_point = {points.at(i).at(0), points.at(i).at(1)};
            path.lineTo(next_point);
        }
    }

    void updateQtScene() {
        updateQtPath();
        // it its a placeholder for a stroke in progress
        if (path_item == nullptr) {
            return;
        }
        path_item->setPath(path);
    }
#endif
};
