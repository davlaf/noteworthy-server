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

#include "User.hpp"
#include "nlohmann/ordered_map.hpp"

class Page {
public:
    uint64_t page_id;
#ifdef NOTEWORTHY_QT
    std::shared_ptr<QGraphicsScene> scene = std::make_shared<QGraphicsScene>();

    uint64_t getObjectIdFromGraphicsItem(QGraphicsItem* item)
    {
        return pointer_to_id_map.at(item);
    }
#endif

    std::unique_ptr<CanvasObject> deleteObject(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(page_mutex);
        std::unique_ptr<CanvasObject> object = std::move(object_map.at(id));
        object_map.erase(id);
        return object;
    }

    void addObject(std::unique_ptr<CanvasObject> object)
    {
        std::lock_guard<std::mutex> lock(page_mutex);
#ifdef NOTEWORTHY_QT
        pointer_to_id_map[object->item] = object->object_id;
#endif
        object_map[object->object_id] = std::move(object);
    }

    void
    manipulateObject(uint64_t id,
        const std::function<void(CanvasObject&)>& manipulator)
    {
        std::lock_guard<std::mutex> lock(page_mutex);
        manipulator(*object_map[id]);
    }

    void forEach(const std::function<void(CanvasObject&)>& manipulator)
    {
        std::lock_guard<std::mutex> lock(page_mutex);
        for (auto& [id, object] : object_map) {
            manipulator(*object);
        }
    }

private:
    std::mutex page_mutex;
    std::map<uint64_t, std::unique_ptr<CanvasObject>> object_map;
#ifdef NOTEWORTHY_QT
    std::map<QGraphicsItem*, uint64_t> pointer_to_id_map;
#endif
};

class RoomState {
public:
    std::string room_id;
    std::string owner_id;
    std::string password;

    RoomState(std::string room_id, std::string owner_id, std::string password)
        : room_id(room_id)
        , owner_id(owner_id)
        , password(password)
    {
        auto owner_connection = std::make_unique<User>(room_id, owner_id, nullptr);
        addUser(std::move(owner_connection));
    }

    void toJson(nlohmann::json& json)
    {
        json["owner_id"] = owner_id;
        json["room_id"] = room_id;
        json["password"] = password;
        json["object_type"] = ROOM;
    }

    void fromJson(const nlohmann::json& json)
    {
        json.at("owner_id").get_to(owner_id);
        json.at("room_id").get_to(room_id);
        json.at("password").get_to(password);
    }

    void toJsonEventList(nlohmann::json& json)
    {

        // std::lock_guard<std::mutex> lock(room_mutex);
        // first do a create room event
        nlohmann::json home_json;
        createCreateRoomEvent(home_json);
        json.push_back(home_json);
        // add users
        for (const auto& [username, user] : users) {
            nlohmann::json user_info;
            user->toJson(user_info);
            json.push_back(user_info);
        }

        // then add create page event each page and add all the objects of that
        // page add last page first, adding the next pages at position 0 so they
        // are in order
        forEachReverse([this, &json](Page& page) mutable {
            nlohmann::json create_page_json;
            createInsertPageEvent(create_page_json, 0, page.page_id);
            json.push_back(create_page_json);
            page.forEach([this, &json](CanvasObject& object) mutable {
                nlohmann::json create_canvas_object_json;
                object.createCreateEvent(create_canvas_object_json);
                json.push_back(create_canvas_object_json);
            });
        });
    }

    void createCreateRoomEvent(nlohmann::json& json)
    {
        toJson(json);
        json["event_type"] = CREATE;
    }

    void applyCreateRoomEvent(const nlohmann::json& json)
    {
        fromJson(json);
        page_map.clear();
    }

    void createInsertPageEvent(nlohmann::json& json, uint64_t previous_page_id,
        uint64_t new_page_id)
    {
        json["room_id"] = room_id;
        json["event_type"] = CREATE;
        json["object_type"] = PAGE;
        json["page_id"] = new_page_id;
        json["previous_page_id"] = previous_page_id;
    }

    void applyInsertPageEvent(const nlohmann::json& json)
    {
        std::unique_ptr<Page> page = std::make_unique<Page>();
        json.at("page_id").get_to(page->page_id);
        uint64_t previous_page_id = json["previous_page_id"];
        addPageAfter(previous_page_id, std::move(page));
    }

    void createDeletePageEvent(nlohmann::json& json, uint64_t page_id)
    {
        json["room_id"] = room_id;
        json["event_type"] = DELETE;
        json["object_type"] = PAGE;
        json["page_id"] = page_id;
    }

    void applyDeletePageEvent(const nlohmann::json& json)
    {
        uint64_t page_id = json["page_id"];
        deletePage(page_id);
    }

    void deletePage(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(room_mutex);
        page_map.erase(id); // Automatic memory management by unique_ptr
        page_order.remove(id);
    }

    void addPageAfter(uint64_t previous_page_id, std::unique_ptr<Page> page)
    {
        uint64_t new_page_id = page->page_id;
        std::lock_guard<std::mutex> lock(room_mutex);
        page_map[new_page_id] = std::move(page); // Store unique_ptr in the map

        if (previous_page_id == 0) {
            page_order.push_back(new_page_id);
            return;
        }

        // Insert page after previous page in the order
        auto it = std::find(page_order.begin(), page_order.end(), previous_page_id);
        assert(it != page_order.end()); // Ensure the previous page was found
        page_order.insert(std::next(it), new_page_id);
    }

    void manipulatePage(uint64_t id,
        const std::function<void(Page&)>& manipulator)
    {
        std::lock_guard<std::mutex> lock(room_mutex);
        if (page_map.count(id) == 0) {
            throw std::runtime_error("page not found");
        }
        manipulator(*page_map.at(id)); // Pass to manipulator by reference
    }

    void manipulateUser(const std::string& username,
        const std::function<void(User&)>& manipulator)
    {
        std::lock_guard<std::mutex> lock(room_mutex);
        if (users.count(username) == 0) {
            throw std::runtime_error("user not found");
        }
        manipulator(*users.at(username)); // Pass to manipulator by reference
    }

    // DANGEROUS!!!
    User* getUserPtr(const std::string& username)
    {
        std::lock_guard<std::mutex> lock(room_mutex);
        if (users.count(username) == 0) {
            throw std::runtime_error("user not found");
        }
        return users.at(username).get();
    }

    void forEachReverse(const std::function<void(Page&)>& manipulator)
    {
        for (auto it = page_order.rbegin(); it != page_order.rend(); it++) {
            manipulatePage(*it, manipulator);
        }
    }

    bool getNextPageId(uint64_t page_id, uint64_t& next_page_id)
    {
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

    bool getPrevPageId(uint64_t page_id, uint64_t& prev_page_id)
    {
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

    bool getFirstPageId(uint64_t& first_page_id)
    {
        if (page_order.empty()) {
            return false;
        }
        first_page_id = page_order.front();
        return true;
    }

    void addUser(std::unique_ptr<User> user)
    {
        std::lock_guard<std::mutex> lock(room_mutex);
        users[user->username] = std::move(user);
    }

    void removeUser(const std::string& username)
    {
        std::lock_guard<std::mutex> lock(room_mutex);
        users.erase(username);
    }

    void setRoomOwner(const std::string& username)
    {
        std::lock_guard<std::mutex> lock(room_mutex);
        if (users.count(username) == 0) {
            return;
        }
        owner_id = username;
    }

    bool isUserInRoom(const std::string& username) const
    {
        return users.count(username) > 0;
    }

    bool isUserKicked(const std::string& username) const
    {
        if (!isUserInRoom(username)) {
            return false;
        }
        return users.at(username)->is_kicked;
    }

    bool isUserConnectedToRoom(const std::string& username) const
    {
        if (users.count(username) == 0) {
            return false;
        }
        return users.at(username)->is_connected;
    }

    void forEachUser(const std::function<void(const User&)>& manipulator)
    {
        for (auto const& [username, user] : users) {
            manipulator(*user);
        }
    }

private:
    std::mutex room_mutex;
    std::map<uint64_t, std::unique_ptr<Page>>
        page_map; // Use unique_ptr for automatic memory management
    std::list<uint64_t> page_order;
    std::map<std::string, std::unique_ptr<User>> users; // User management map
};

#ifdef NOTEWORTHY_QT
extern RoomState state; // Declaration only
#endif
