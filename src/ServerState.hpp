#pragma once

#include "RandomIdGenerator.hpp"
#include "RoomState.hpp"
#include "Shape.hpp"
#include "Stroke.hpp"
#include "Symbol.hpp"
#include <cstdint>
#include <list>
#include <map>
#include <memory> // Include for smart pointers
#include <mutex>
#include <variant>
#include <vector>

#include "nlohmann/ordered_map.hpp"

class ServerState {
  public:
    std::string createRoom(const std::string room_id, const std::string &owner,
                           const std::string &room_password) {
        // Create a unique page ID
        std::string new_room_id = room_id;
        // do {
        //     new_room_id = ".....";
        //     for (int i = 0; i < 5; i++) {
        //         auto random_letter_index =
        //             IDGenerator::newID() % valid_room_id_chars.size();
        //         new_room_id[i] = valid_room_id_chars[random_letter_index];
        //     }
        // } while (room_map.count(new_room_id) > 0);

        // Create a page using smart pointer
        auto room = std::make_unique<RoomState>(room_id, owner, room_password);

        // TODO: we need to set the owners and password and stuff
        // room.get()->
        std::lock_guard<std::mutex> lock(room_map_mutex);
        room_map[new_room_id] = std::move(room);

        // Return the new ID
        return new_room_id;
    }

    void deleteRoom(const std::string &id) {
        std::lock_guard<std::mutex> lock(room_map_mutex);
        room_map.erase(id); // Automatic memory management by unique_ptr
    }

    void manipulateRoom(const std::string &id,
                        const std::function<void(RoomState &)> &manipulator) {
        std::lock_guard<std::mutex> lock(room_map_mutex);
        auto it = room_map.find(id);
        if (it == room_map.end()) {
            throw std::range_error("room not found");
        }
        manipulator(*it->second); // Pass to manipulator by reference
    }

  private:
    const std::string valid_room_id_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456790";

    std::mutex room_map_mutex;
    std::map<std::string, std::unique_ptr<RoomState>>
        room_map; // Use unique_ptr for automatic memory management
};

extern ServerState state;