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
    std::string createRoom(const std::string& owner,
        const std::string& room_password = "")
    {
        std::string new_room_id;
        do {
            new_room_id = ".....";
            for (int i = 0; i < 5; i++) {
                auto random_letter_index = IDGenerator::newID() % valid_room_id_chars.size();
                new_room_id[i] = valid_room_id_chars[random_letter_index];
            }
        } while (room_map.count(new_room_id) > 0);

        createRoom(new_room_id, owner, room_password);
        return new_room_id;
    }
    std::string createRoom(const std::string& room_id, const std::string& owner,
        const std::string& room_password)
    {
        // Create a unique page ID
        std::string new_room_id = room_id;

        // Create a page using smart pointer
        auto room = std::make_unique<RoomState>(room_id, owner, room_password);

        std::lock_guard<std::mutex> lock(room_map_mutex);
        room_map[new_room_id] = std::move(room);

        // Return the new ID
        return new_room_id;
    }

    void deleteRoom(const std::string& id)
    {
        std::lock_guard<std::mutex> lock(room_map_mutex);
        room_map.erase(id); // Automatic memory management by unique_ptr
    }

    void manipulateRoom(const std::string& id,
        const std::function<void(RoomState&)>& manipulator)
    {
        std::lock_guard<std::mutex> lock(room_map_mutex);
        if (!hasRoom(id)) {
            throw std::range_error("room not found");
        }
        manipulator(*room_map.at(id)); // Pass to manipulator by reference
    }

    bool hasRoom(const std::string& id) { return room_map.count(id) > 0; }

private:
    const std::string valid_room_id_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ12345679";

    std::mutex room_map_mutex;
    std::map<std::string, std::unique_ptr<RoomState>>
        room_map; // Use unique_ptr for automatic memory management
};

extern ServerState state;