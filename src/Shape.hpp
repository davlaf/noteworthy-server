#pragma once
#ifdef NOTEWORTHY_QT
#include <qpainterpath.h>
#endif
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

class Shape{
private:
    std::vector<std::vector<double>> points;
    std::string owner_id;
    std::string room_id;
    uint64_t shape_id;

public:
    std::string type; //for different shapes eg. circle, rectangle ...
    double radius;     // For circle
    double width, height; // For rectangle
    std::vector<double> center; // For circle and rectangle (the anchor point)

     static void to_json(nlohmann::json &json, const Shape &shape){
        json = nlohmann::json::object();
        json["shape_id"] = shape.shape_id;
        json["owner_id"] = shape.owner_id;
        json["room_id"] = shape.room_id;
        json["type"] = shape.type;
        //possibly get the center of all of the shape 
        //json["center"] = shape.center;

        // Shape-specific fields
        if (shape.type == "circle") {
            json["center"] = shape.center;
            json["radius"] = shape.radius;
        } else if (shape.type == "rectangle") {
            json["center"] = shape.center;
            json["width"] = shape.width;
            json["height"] = shape.height;
        }
    }

    static void from_json(const nlohmann::json &json, Shape &shape){
        json.at("shape_id").get_to(shape.shape_id);
        json.at("owner_id").get_to(shape.owner_id);
        json.at("room_id").get_to(shape.room_id);
        json.at("type").get_to(shape.type);
        //center of all shapes
        //json.at("center").get_to(shape.center);

        // Shape-specific fields
        if (shape.type == "circle") {
            json.at("center").get_to(shape.center);
            json.at("radius").get_to(shape.radius);
        } else if (shape.type == "rectangle") {
            json.at("center").get_to(shape.center);
            json.at("width").get_to(shape.width);
            json.at("height").get_to(shape.height);
        }
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