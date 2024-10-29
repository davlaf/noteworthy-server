#pragma once

#include "CanvasObject.hpp"
#include "RandomIdGenerator.hpp"
#include <cstdint>
#include <list>
#include <map>
#include <memory> // Include for smart pointers
#include <mutex>

#include "nlohmann/ordered_map.hpp"

class Page {
  public:
    std::unique_ptr<CanvasObject> deleteObject(uint64_t id) {
        std::lock_guard<std::mutex> lock(page_mutex);
        std::unique_ptr<CanvasObject> object = std::move(object_map.at(id));
        object_map.erase(id);
        return object;
    }

    void addObject(std::unique_ptr<CanvasObject> object) {
        std::lock_guard<std::mutex> lock(page_mutex);
        object_map[object.get()->object_id] = std::move(object);
    }

    void
    manipulateObject(uint64_t id,
                     const std::function<void(CanvasObject &)> &manipulator) {
        std::lock_guard<std::mutex> lock(page_mutex);
        manipulator(*object_map[id]);
    }

    void
    forEach(const std::function<void(uint64_t, CanvasObject &)> &manipulator) {
        std::lock_guard<std::mutex> lock(page_mutex);
        for (auto &[id, object] : object_map) {
            manipulator(id, *object);
        }
    }

  private:
    std::mutex page_mutex;
    nlohmann::ordered_map<uint64_t, std::unique_ptr<CanvasObject>> object_map;
};

class RoomState {
  public:
    std::string room_id;

    RoomState(const std::string &room_id) : room_id(room_id) {};

    uint64_t createPageAfter(uint64_t previous_page_id) {
        // Create a unique page ID
        uint64_t new_page_id;
        do {
            new_page_id = IDGenerator::newID();
        } while (page_map.count(new_page_id) > 0);

        // Create a page using smart pointer
        auto page = std::make_unique<Page>();

        // Add page after
        addPageAfter(previous_page_id, new_page_id, std::move(page));

        // Return the new ID
        return new_page_id;
    }

    void deletePage(uint64_t id) {
        std::lock_guard<std::mutex> lock(room_mutex);
        page_map.erase(id); // Automatic memory management by unique_ptr
        page_order.remove(id);
    }

    void addPageAfter(uint64_t previous_page_id, uint64_t new_page_id,
                      std::unique_ptr<Page> page) {
        std::lock_guard<std::mutex> lock(room_mutex);
        page_map[new_page_id] = std::move(page); // Store unique_ptr in the map

        if (previous_page_id == 0) {
            page_order.push_back(new_page_id);
            return;
        }

        // Insert page after previous page in the order
        auto it =
            std::find(page_order.begin(), page_order.end(), previous_page_id);
        assert(it != page_order.end()); // Ensure the previous page was found
        page_order.insert(std::next(it), new_page_id);
    }

    void manipulatePage(uint64_t id,
                        const std::function<void(Page &)> &manipulator) {
        std::lock_guard<std::mutex> lock(room_mutex);
        auto it = page_map.find(id);
        assert(it != page_map.end());
        manipulator(*it->second); // Pass to manipulator by reference
    }

  private:
    std::mutex room_mutex;
    std::map<uint64_t, std::unique_ptr<Page>>
        page_map; // Use unique_ptr for automatic memory management
    std::list<uint64_t> page_order;
};

#ifdef NOTEWORTHY_QT
extern RoomState state; // Declaration only
#endif
