#include "ServerState.hpp"
#include "User.hpp"
#include "UserRoles.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/http_headers.h>
#include <pistache/router.h>
#include <regex>
#include <string>

using namespace Pistache;

class RoomHandler {
public:
    explicit RoomHandler() { }

    void setupRoutes(Rest::Router& router)
    {
        Rest::Routes::Post(router, "/v1/rooms",
            Rest::Routes::bind(&RoomHandler::createRoom, this));

        // Rest::Routes::Options(
        //     router, "/v1/rooms/:id",
        //     Rest::Routes::bind(&RoomHandler::handleOptionsRequest, this));
        Rest::Routes::Get(router, "/v1/rooms/:room_id",

            Rest::Routes::bind(&RoomHandler::getRoom, this));
        Rest::Routes::Post(router, "/v1/rooms/:room_id/users",
            Rest::Routes::bind(&RoomHandler::createUser, this));

        // Default handler for invalid routes
        router.addCustomHandler(
            Rest::Routes::bind(&RoomHandler::handleNotFound, this));
    }

private:
    void createRoom(const Rest::Request& request,
        Http::ResponseWriter response)
    {
        auto username_query = request.query().get("username").value_or("");

        if (username_query.empty()) {
            response.send(Http::Code::Bad_Request,
                "Username parameter is required");
            return;
        }

        // create a room with that user as owner
        // with no password
        std::string room_id = state.createRoom(username_query);
        response.send(Http::Code::Ok, room_id);
    }

    void handleNotFound(const Rest::Request& request,
        Http::ResponseWriter response)
    {
        std::cout << "handling fake request for route:" << std::endl;

        auto method = request.method();
        auto resource = request.resource();

        if (resource.empty()) {
            response.send(Http::Code::Bad_Request, "Invalid resource");
            return;
        }

        std::cout << method << ": " << resource << std::endl;
        response.send(Http::Code::Not_Found, "Invalid route!!");
    }

    // void handleOptionsRequest(const Rest::Request& request,
    //     Http::ResponseWriter response)
    // {
    //     response.headers()
    //         .add<Http::Header::AccessControlAllowOrigin>(
    //             "*") // or specify the origin: "http://localhost:30000"
    //         .add<Http::Header::AccessControlAllowMethods>("GET, OPTIONS")
    //         .add<Http::Header::AccessControlAllowHeaders>(
    //             "Authorization, Content-Type, Accept-Language");
    //     response.send(Http::Code::Ok, "epic");
    // }
    bool isAuthenticated(
        std::string& room_id,
        const Rest::Request& request,
        Http::ResponseWriter& response)
    {
        auto authHeader = request.headers().tryGet<Http::Header::Authorization>();

        const std::string bearerPrefix = "Bearer ";
        if (authHeader) {
            const std::string authHeaderPrefix = authHeader->value().substr(0, bearerPrefix.size());

            if (authHeaderPrefix != bearerPrefix) {
                response.send(Http::Code::Unauthorized,
                    "Unauthorized: Using incorrect auth type");
                return false;
            }
        }

        std::string receivedToken = "";
        if (authHeader) {
            receivedToken = authHeader->value().substr(7);
        }

        if (!state.hasRoom(room_id)) {
            response.send(Http::Code::Not_Found, "Room not found");
            return false;
        }

        std::string room_password;
        state.manipulateRoom(room_id, [&](RoomState& room) {
            room_password = room.password;
        });

        bool room_has_password = room_password != "";

        if (!room_has_password) {
            // the room exists, and the room has no password
            return true;
        }

        if (!authHeader && room_has_password) {
            response.send(Http::Code::Locked, "Room needs password");
            return false;
        }

        bool password_is_correct = room_password == receivedToken;

        if (!password_is_correct) {
            response.send(Http::Code::Forbidden, "Incorrect Password");
            return false;
        }

        // the room exists, has a password and the password was correct
        return true;
    }

    void getRoom(const Rest::Request& request, Http::ResponseWriter response)
    {
        // response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
        auto room_id = request.param(":room_id").as<std::string>();
        std::cout << "handling request for room id " << room_id << std::endl;

        if (!isAuthenticated(room_id, request, response)) {
            return;
        }

        nlohmann::json event_list_json;
        state.manipulateRoom(room_id, [&](RoomState& room) {
            room.toJsonEventList(event_list_json);
        });
        response.setMime(MIME(Application, Json));
        response.send(Http::Code::Ok, event_list_json.dump());
    }

    void createUser(const Rest::Request& request,
        Http::ResponseWriter response)
    {
        std::cout << "handling user add route" << std::endl;
        auto room_id = request.param(":room_id").as<std::string>();
        auto username_query = request.query().get("username").value_or("");

        if (!isAuthenticated(room_id, request, response)) {
            return;
        }

        if (username_query.empty()) {
            response.send(Http::Code::Bad_Request,
                "Username parameter is required");
            return;
        }

        std::string username = username_query;

        // Manipulate the room state to add a new user if the username is unique
        state.manipulateRoom(room_id, [&room_id, &username, &response](RoomState& room) {
            if (room.isUserConnectedToRoom(username)) {
                response.send(Http::Code::Conflict,
                    "Username already connected in the room");
                return;
            }

            if (room.isUserKicked(username)) {
                response.send(Http::Code::Forbidden,
                    "you were kicked");
                return;
            }

            if (room.isUserInRoom(username)) {
                response.send(Http::Code::Ok, "Resumed an existing user");
                return;
            }

            auto user = std::make_unique<User>(room_id, username);
            room.addUser(std::move(user));

            // Return success response
            response.send(Http::Code::Created, "User created successfully");
        });
    }
};

// Main function to start the server
void startServer(int port)
{

    Http::Endpoint server(Address(Ipv4::any(), Port(port)));
    Rest::Router router;

    RoomHandler handler;
    handler.setupRoutes(router);

    auto options = Http::Endpoint::options().threads(1);
    server.init(options);
    server.setHandler(router.handler());
    std::cout << "Server is running at http://localhost:8080" << std::endl;
    server.serve();

    server.shutdown();
}