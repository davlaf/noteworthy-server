#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <mutex>
#include <variant>

#include "nlohmann/ordered_map.hpp"

class Room {};

typedef std::variant<Stroke, Shape, Symbol, TextBox> Object;

class Page {
  public:
    // we need to add field for background image
    void deleteObject(uint64_t id) {
        std::lock_guard<std::mutex> lock(map_mutex);
        object_map.erase(id);
    }

    void addObject(uint64_t id, Object object) {
        std::lock_guard<std::mutex> lock(map_mutex);
        object_map[id] = object;
    }

    Object getObject(uint64_t id) {
        std::lock_guard<std::mutex> lock(map_mutex);
        return object_map[id];
    }

    // implemented this way so edited objects show up on top
    void editObject(uint64_t id, Object new_object) {
        std::lock_guard<std::mutex> lock(map_mutex);
        void deleteObject(uint64_t id);
        object_map[id] = new_object;
    }

  private:
    nlohmann::ordered_map<uint64_t, Object> object_map;
    // maybe want to use mutable std::shared_mutex mutex_; instead
    std::mutex map_mutex;
};

class Stroke {};

class Shape {};

class Symbol {};

class TextBox {};