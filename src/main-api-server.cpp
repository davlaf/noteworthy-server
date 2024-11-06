#include "ServerState.hpp"
#include <iostream>
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
        Rest::Routes::Get(router, "/rooms/:room_id",
                          Rest::Routes::bind(&RoomHandler::getRoom, this));
    }

  private:
    void getRoom(const Rest::Request &request, Http::ResponseWriter response) {
        auto room_id = request.param(":room_id").as<std::string>();

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
                // int epic = 5;

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
};

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