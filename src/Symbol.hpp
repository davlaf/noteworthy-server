#pragma once
#ifdef NOTEWORTHY_QT
#include <qpainterpath.h>
#endif
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

class Symbol{

private:
    std::vector<std::vector<double>> points; //position of the symbol
    std::string owner_id;
    std::string room_id;
    uint64_t symbol_id;

public:
    std::string type; //for different types of symbols 
    double size; 
    double rotation;
    std::vector<double> center; // For center of symbol

     static void to_json(nlohmann::json &json, const Symbol &symbol){
        json = nlohmann::json::object();
        json["symbol_id"] = symbol.symbol_id;
        json["owner_id"] = symbol.owner_id;
        json["room_id"] = symbol.room_id;
        json["type"] = symbol.type;
        json["position"] = symbol.points;
        json["rotation"] = symbol.rotation;
        json["size"] = symbol.size;
    
    }

    static void from_json(const nlohmann::json &json, Symbol &symbol){
        json.at("symbol_id").get_to(symbol.symbol_id);
        json.at("owner_id").get_to(symbol.owner_id);
        json.at("room_id").get_to(symbol.room_id);
        json.at("type").get_to(symbol.type);
        json.at("position").get_to(symbol.points);
        json.at("rotation").get_to(symbol.rotation);
        json.at("size").get_to(symbol.size);

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