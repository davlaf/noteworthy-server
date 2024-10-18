#pragma once
#ifdef NOTEWORTHY_QT
#include <qpainterpath.h>
#endif
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

class Stroke {
  private:
    std::vector<std::vector<double>> points;
    std::string owner_id;
    std::string room_id;
    uint64_t stroke_id;

  public:
    static void to_json(nlohmann::json &json, const Stroke &stroke) {
        json = nlohmann::json::object();
        json["stroke_id"] = stroke.stroke_id;
        json["owner_id"] = stroke.owner_id;
        json["room_id"] = stroke.room_id;
        json["points"] = stroke.points;
    }

    static void from_json(const nlohmann::json &json, Stroke &stroke) {
        json.at("owner_id").get_to(stroke.owner_id);
        json.at("room_id").get_to(stroke.room_id);
        json.at("points").get_to(stroke.points);
        json.at("points").get_to(stroke.points);
    }

#ifdef NOTEWORTHY_QT
    QPainterPath *toQPainterPath() {
        std::vector<double> starting_vector = points.at(0);
        QPointF starting_point{starting_vector.at(0), starting_vector.at(1)};
        QPainterPath *newPath = new QPainterPath{starting_point};

        for (size_t i = 1; i < points.size(); i++) {
            QPointF next_point = {points.at(i).at(0), points.at(i).at(1)};
            newPath->lineTo(next_point);
        }

        return newPath;
    }

    void fromQPainterPath(QPainterPath &path) {
        int numElements = path.elementCount();

        points.clear();
        for (int i = 0; i < numElements; ++i) {
            QPainterPath::Element element = path.elementAt(i);

            points.push_back({element.x, element.y});
        }
    }
#endif
};
