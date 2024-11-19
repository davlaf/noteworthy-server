#include "ServerState.hpp"
#include "UserConnection.hpp"
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
    explicit RoomHandler() {}

    void setupRoutes(Rest::Router &router) {
        Rest::Routes::Options(
            router, "/v1/rooms/:id",
            Rest::Routes::bind(&RoomHandler::handleOptionsRequest, this));
        Rest::Routes::Get(router, "/v1/rooms/:room_id",
                          Rest::Routes::bind(&RoomHandler::getRoom, this));
        Rest::Routes::Post(router, "/v1/rooms/:room_id/users",
                           Rest::Routes::bind(&RoomHandler::createUser, this));
        Rest::Routes::Get(router, "/v1/rooms/:room_id/users",
                          Rest::Routes::bind(&RoomHandler::listUsers, this));

        // Default handler for invalid routes
        router.addCustomHandler(
            Rest::Routes::bind(&RoomHandler::handleNotFound, this));
    }

  private:
  private:
    void handleNotFound(const Rest::Request &request,
                        Http::ResponseWriter response) {
        std::cout << "handling fake request for route:" << std::endl;
        std::cout << request.method() << ": " << request.resource()
                  << std::endl;
        response.send(Http::Code::Not_Found, "invalid route!!");
    }
    void handleOptionsRequest(const Rest::Request &request,
                              Http::ResponseWriter response) {
        response.headers()
            .add<Http::Header::AccessControlAllowOrigin>(
                "*") // or specify the origin: "http://localhost:30000"
            .add<Http::Header::AccessControlAllowMethods>("GET, OPTIONS")
            .add<Http::Header::AccessControlAllowHeaders>(
                "Authorization, Content-Type, Accept-Language");
        response.send(Http::Code::Ok, "epic");
    }

    void getRoom(const Rest::Request &request, Http::ResponseWriter response) {
        response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
        auto room_id = request.param(":room_id").as<std::string>();
        std::cout << "handling request for room id " << room_id << std::endl;

        auto authHeader =
            request.headers().tryGet<Http::Header::Authorization>();

        const std::string bearerPrefix = "Bearer ";
        if (authHeader) {
            const std::string authHeaderPrefix =
                authHeader->value().substr(0, bearerPrefix.size());

            if (authHeaderPrefix != bearerPrefix) {
                response.send(Http::Code::Unauthorized,
                              "Unauthorized: Using incorrect auth type");
                return;
            }
        }

        std::string receivedToken = "";
        if (authHeader) {
            receivedToken = authHeader->value().substr(7);
        }

        const std::function<void(RoomState &)> &manipulator =
            [&response, &receivedToken, &authHeader](RoomState &room) {
                bool room_has_password = room.password != "";

                if (!authHeader && room_has_password) {
                    response.send(Http::Code::Locked, "Room needs password");
                    return;
                }

                bool password_is_correct = room.password == receivedToken;
                bool is_authenticated =
                    !room_has_password | password_is_correct;

                if (!is_authenticated) {
                    response.send(Http::Code::Forbidden, "Incorrect Password");
                    return;
                }

                response.headers().add<Http::Header::AccessControlAllowOrigin>(
                    "*");
                nlohmann::json event_list_json;
                room.toJsonEventList(event_list_json);
                response.setMime(MIME(Application, Json));
                response.send(Http::Code::Ok, event_list_json.dump());
            };
        try {
            state.manipulateRoom(room_id, manipulator);
        } catch (std::range_error) {
            response.send(Http::Code::Not_Found, "Room not found");
            return;
        }
    }
    // Create a new user in the specified room
    void createUser(const Rest::Request &request,
                    Http::ResponseWriter response) {
        std::cout << "handling user add route" << std::endl;
        auto room_id = request.param(":room_id").as<std::string>();
        auto username_query = request.query().get("username").value_or("");

        if (username_query.empty()) {
            response.send(Http::Code::Bad_Request,
                          "Username parameter is required");
            return;
        }

        std::string username = username_query;
        auto role = UserRole::MEMBER; // Default role for new users

        // Manipulate the room state to add a new user if the username is unique
        state.manipulateRoom(room_id, [username, role,
                                       &response](RoomState &room) {
            if (room.isUserInRoom(username)) {
                response.send(Http::Code::Conflict,
                              "Username already exists in the room");
                return;
            }

            // Create the new user connection
            UserConnection newUser = {room.room_id, username, nullptr, role};
            room.addUser(newUser);

            // Return success response
            response.send(Http::Code::Created, "User created successfully");
        });
    }

    // List all users in the specified room
    void listUsers(const Rest::Request &request,
                   Http::ResponseWriter response) {
        auto room_id = request.param(":room_id").as<std::string>();

        state.manipulateRoom(room_id, [&response](RoomState &room) {
            auto users = room.listUsers();
            nlohmann::json userListJson = users;
            response.setMime(MIME(Application, Json));
            response.send(Http::Code::Ok, userListJson.dump());
        });
    }
};

// Main function to start the server
void startServer(int port) {

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