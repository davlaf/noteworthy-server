#pragma once

#include "CanvasObject.hpp"
#include <cstdint>
#include <list>
#include <map>
#include <memory> // Include for smart pointers
#include <mutex>
#ifdef NOTEWORTHY_QT
#include <clickablegraphicsview.h>
#include <qgraphicsscene.h>
#endif

#include "UserConnection.hpp"
#include "nlohmann/ordered_map.hpp"

class Page {
  public:
    uint64_t page_id;
#ifdef NOTEWORTHY_QT
    std::shared_ptr<QGraphicsScene> scene = std::make_shared<QGraphicsScene>();

    uint64_t getObjectIdFromGraphicsItem(QGraphicsItem *item) {
        return pointer_to_id_map.at(item);
    }
#endif

    std::unique_ptr<CanvasObject> deleteObject(uint64_t id) {
        std::lock_guard<std::mutex> lock(page_mutex);
        std::unique_ptr<CanvasObject> object = std::move(object_map.at(id));
        object_map.erase(id);
        return object;
    }

    void addObject(std::unique_ptr<CanvasObject> object) {
        std::lock_guard<std::mutex> lock(page_mutex);
#ifdef NOTEWORTHY_QT
        pointer_to_id_map[object->item] = object->object_id;
#endif
        object_map[object->object_id] = std::move(object);
    }

    void
    manipulateObject(uint64_t id,
                     const std::function<void(CanvasObject &)> &manipulator) {
        std::lock_guard<std::mutex> lock(page_mutex);
        manipulator(*object_map[id]);
    }

    void forEach(const std::function<void(CanvasObject &)> &manipulator) {
        std::lock_guard<std::mutex> lock(page_mutex);
        for (auto &[id, object] : object_map) {
            manipulator(*object);
        }
    }

  private:
    std::mutex page_mutex;
    std::map<uint64_t, std::unique_ptr<CanvasObject>> object_map;
#ifdef NOTEWORTHY_QT
    std::map<QGraphicsItem *, uint64_t> pointer_to_id_map;
#endif
};

class RoomState {
  public:
    std::string room_id;
    std::string owner_id;
    std::string password;

    RoomState(std::string room_id, std::string owner_id, std::string password)
        : room_id(room_id), owner_id(owner_id), password(password) {
        UserConnection owner_connection = {room_id, owner_id, nullptr,
                                           UserRole::OWNER};
        addUser(owner_connection);
    }

    void toJson(nlohmann::json &json) {
        json["owner_id"] = owner_id;
        json["room_id"] = room_id;
        json["object_type"] = ROOM;
    }

    void fromJson(const nlohmann::json &json) {
        json.at("owner_id").get_to(owner_id);
        json.at("room_id").get_to(room_id);
        json.at("password").get_to(password);
    }

    void toJsonEventList(nlohmann::json &json) {

        // std::lock_guard<std::mutex> lock(room_mutex);
        // first do a create room event
        nlohmann::json home_json;
        createCreateRoomEvent(home_json);
        json.push_back(home_json);
        // then add create page event each page and add all the objects of that
        // page add last page first, adding the next pages at position 0 so they
        // are in order
        forEachReverse([this, &json](Page &page) mutable {
            nlohmann::json create_page_json;
            createInsertPageEvent(create_page_json, 0, page.page_id);
            json.push_back(create_page_json);
            page.forEach([this, &json](CanvasObject &object) mutable {
                nlohmann::json create_canvas_object_json;
                object.createCreateEvent(create_canvas_object_json);
                json.push_back(create_canvas_object_json);
            });
        });
    }

    void createCreateRoomEvent(nlohmann::json &json) {
        toJson(json);
        json["event_type"] = CREATE;
    }

    void applyCreateRoomEvent(const nlohmann::json &json) {
        fromJson(json);
        page_map.clear();
    }

    void createInsertPageEvent(nlohmann::json &json, uint64_t previous_page_id,
                               uint64_t new_page_id) {
        json["room_id"] = room_id;
        json["event_type"] = CREATE;
        json["object_type"] = PAGE;
        json["page_id"] = new_page_id;
        json["previous_page_id"] = previous_page_id;
    }

    void applyInsertPageEvent(const nlohmann::json &json) {
        std::unique_ptr<Page> page = std::make_unique<Page>();
        json.at("page_id").get_to(page->page_id);
        uint64_t previous_page_id = json["previous_page_id"];
        addPageAfter(previous_page_id, std::move(page));
    }

    void createDeletePageEvent(nlohmann::json &json, uint64_t page_id) {
        json["room_id"] = room_id;
        json["event_type"] = DELETE;
        json["object_type"] = PAGE;
        json["page_id"] = page_id;
    }

    void applyDeletePageEvent(const nlohmann::json &json) {
        uint64_t page_id = json["page_id"];
        deletePage(page_id);
    }

    void deletePage(uint64_t id) {
        std::lock_guard<std::mutex> lock(room_mutex);
        page_map.erase(id); // Automatic memory management by unique_ptr
        page_order.remove(id);
    }

    void addPageAfter(uint64_t previous_page_id, std::unique_ptr<Page> page) {
        uint64_t new_page_id = page->page_id;
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

    void forEachReverse(const std::function<void(Page &)> &manipulator) {
        for (auto it = page_order.rbegin(); it != page_order.rend(); it++) {
            manipulatePage(*it, manipulator);
        }
    }

    bool getNextPageId(uint64_t page_id, uint64_t &next_page_id) {
        auto it = std::find(page_order.begin(), page_order.end(), page_id);

        if (it == page_order.end()) {
            throw "page doesn't exist when trying to find next page!!";
        }

        ++it; // Move to the next item

        if (it == page_order.end()) {
            return false;
        }

        next_page_id = *it;
        return true;
    }

    bool getPrevPageId(uint64_t page_id, uint64_t &prev_page_id) {
        auto it = std::find(page_order.begin(), page_order.end(), page_id);

        if (it == page_order.end()) {
            throw "page doesn't exist when trying to find prev page!!";
        }

        if (it == page_order.begin()) {
            return false;
        }

        --it; // Move to the previous item

        prev_page_id = *it;
        return true;
    }

    bool getFirstPageId(uint64_t &first_page_id) {
        if (page_order.empty()) {
            return false;
        }
        first_page_id = page_order.front();
        return true;
    }

    void addUser(const UserConnection &user) {
        std::lock_guard<std::mutex> lock(room_mutex);
        users[user.username] = user;
    }

    void removeUser(const std::string &username) {
        std::lock_guard<std::mutex> lock(room_mutex);
        users.erase(username);
    }

    void promoteUser(const std::string &username, UserRole newRole) {
        std::lock_guard<std::mutex> lock(room_mutex);
        if (users.count(username)) {
            users[username].role = newRole;
        }
    }

    bool isUserInRoom(const std::string &username) const {
        return users.count(username) > 0;
    }

    UserRole getUserRole(const std::string &username) const {
        return users.at(username).role;
    }

    // Method to list users in a room
    std::vector<nlohmann::json> listUsers() {
        std::lock_guard<std::mutex> lock(room_mutex);
        std::vector<nlohmann::json> user_list;

        for (const auto &[username, user_connection] : users) {
            nlohmann::json user_info;
            user_info["username"] = username;
            user_info["role"] = user_connection.role;
            user_list.push_back(user_info);
        }

        return user_list;
    }

  private:
    std::mutex room_mutex;
    std::map<uint64_t, std::unique_ptr<Page>>
        page_map; // Use unique_ptr for automatic memory management
    std::list<uint64_t> page_order;
    std::map<std::string, UserConnection> users; // User management map
};

#ifdef NOTEWORTHY_QT
extern RoomState state; // Declaration only
#endif
