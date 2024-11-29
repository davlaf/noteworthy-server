#include "ServerState.hpp"
#include "User.hpp"
#include "base64.hpp"
#include "fpdfview.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/http_headers.h>
#include <pistache/router.h>
#include <regex>
#include <string>
#define STB_IMAGE_WRITE_IMPLEMENTATION // Define the macro to enable implementation
#include "stb_image_write.h"
#include <fstream>

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
        Rest::Routes::Post(router, "/v1/rooms/:room_id/pdf_insert",
            Rest::Routes::bind(&RoomHandler::insertPDFPages, this));

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
        response.send(Http::Code::Created, room_id);
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
            nlohmann::json event;
            user->createCreateEvent(event);
            room.addUser(std::move(user));

            room.forEachUser([&](const User& room_user) {
                if (!room_user.is_connected) {
                    return;
                }

                if (room_user.is_connected && room_user.socket == nullptr) {
                    return;
                    throw "AAAA user is connected but their socket is null";
                }

                room_user.sendEvent(event.dump());
            });

            // Return success response
            response.send(Http::Code::Created, "User created successfully");
        });
    }

    std::string saveBitmapAsBase64(const uint8_t* buffer, int width, int height, int stride)
    {
        // Convert the bitmap to PNG format
        std::vector<uint8_t> pngData;
        auto writeCallback = [](void* context, void* data, int size) {
            auto* out = static_cast<std::vector<uint8_t>*>(context);
            out->insert(out->end(), static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size);
        };

        if (!stbi_write_png_to_func(writeCallback, &pngData, width, height, 4, buffer, stride)) {
            std::cerr << "Failed to write PNG.\n";
            return "";
        }

        // Now encode the PNG data to Base64 using std::string_view
        std::string_view dataView(reinterpret_cast<const char*>(pngData.data()), pngData.size());
        return base64::to_base64(dataView);
    }

    void insertPDFPages(const Rest::Request& request,
        Http::ResponseWriter response)
    {
        std::cout << "handling pdf upload" << std::endl;
        auto room_id = request.param(":room_id").as<std::string>();
        auto previous_page_id_query = request.query().get("previous_page_id").value_or("");

        if (!isAuthenticated(room_id, request, response)) {
            return;
        }

        if (previous_page_id_query.empty()) {
            response.send(Http::Code::Bad_Request,
                "previous_page_id parameter is required");
            return;
        }

        auto body = request.body();

        auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
        uint64_t previous_page_id;
        try {
            previous_page_id = std::stoull(previous_page_id_query);
        } catch (std::invalid_argument e) {
            response.send(Http::Code::Bad_Request,
                "Invalid previous page id");
            return;
        }

        // Save the body as a PDF file (assuming a single file upload)
        std::string output_filename = std::string("uploaded_file") + std::to_string(thread_id) + ".pdf";
        std::ofstream outFile(output_filename, std::ios::binary);
        auto data = body.data();
        outFile.write(body.data(), body.size());
        outFile.close();

        std::vector<std::string> base64_list = renderPDFtoBase64List(output_filename);

        if (base64_list.empty()) {
            response.send(Http::Code::Bad_Request,
                "Empty pdf file");
            return;
        }

        state.manipulateRoom(room_id, [&](RoomState& room) {
            for (std::string base64 : base64_list) {
                auto page = std::make_unique<Page>();
                page->room_id = room_id;
                uint64_t new_id = IDGenerator::newID();
                page->page_id = new_id;
                page->base64_image = base64;

                try {
                    room.addPageAfter(previous_page_id, std::move(page));
                } catch (std::runtime_error e) {
                    response.send(Http::Code::Bad_Request,
                        "previous page id not found");
                    return;
                }

                previous_page_id = new_id;
            }

            // make everyone reset their room
            nlohmann::json event;
            room.createResetEvent(event);

            room.forEachUser([&](const User& user) {
                if (!user.is_connected) {
                    return;
                }

                if (user.is_connected && user.socket == nullptr) {
                    return;
                    throw "AAAA user is connected but their socket is null";
                }

                user.sendEvent(event.dump());
            });
        });
        response.send(Http::Code::Created,
            "successfully created");
    }

    std::vector<std::string> renderPDFtoBase64List(std::string filename)
    {
        std::vector<std::string> base64_list;
        // Load a PDF document
        FPDF_DOCUMENT pdfDocument = FPDF_LoadDocument(filename.c_str(), nullptr);
        if (!pdfDocument) {
            return {};
        }

        int pageCount = FPDF_GetPageCount(pdfDocument);
        for (int pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
            FPDF_PAGE page = FPDF_LoadPage(pdfDocument, pageIndex);
            if (!page) {
                std::cerr << "Failed to load page " << pageIndex << ".\n";
                continue;
            }

            // Get page dimensions
            double width = FPDF_GetPageWidth(page);
            double height = FPDF_GetPageHeight(page);

            // Create a bitmap
            FPDF_BITMAP bitmap = FPDFBitmap_Create(static_cast<int>(width), static_cast<int>(height), 0);
            FPDFBitmap_FillRect(bitmap, 0, 0, static_cast<int>(width), static_cast<int>(height), 0xFFFFFFFF);

            // Render the page into the bitmap
            FPDF_RenderPageBitmap(bitmap, page, 0, 0, static_cast<int>(width), static_cast<int>(height), 0, 0);

            // Save the bitmap as PNG and encode as Base64
            const uint8_t* buffer = static_cast<const uint8_t*>(FPDFBitmap_GetBuffer(bitmap));
            int stride = FPDFBitmap_GetStride(bitmap);

            // Check and swap BGRA to RGBA (if needed)
            std::vector<uint8_t> imageData(buffer, buffer + stride * static_cast<int>(height));

            // Iterate through the image data and swap the red and blue channels if the image is in BGRA
            for (int i = 0; i < imageData.size(); i += 4) {
                unsigned char blue = imageData[i];
                unsigned char red = imageData[i + 2];

                // Swap the red and blue channels
                imageData[i] = red;
                imageData[i + 2] = blue;
            }

            std::string base64Image = saveBitmapAsBase64(imageData.data(), static_cast<int>(width), static_cast<int>(height), stride);

            // Output Base64 string
            base64_list.push_back(base64Image);

            // Clean up
            FPDFBitmap_Destroy(bitmap);
            FPDF_ClosePage(page);
        }
        return base64_list;
    }
};

// Main function to start the server
void startServer(int port)
{

    Http::Endpoint server(Address(Ipv4::any(), Port(port)));
    Rest::Router router;

    RoomHandler handler;
    handler.setupRoutes(router);

    auto options = Http::Endpoint::options().threads(5).maxRequestSize(10000000);
    server.init(options);
    server.setHandler(router.handler());
    std::cout << "Server is running at http://localhost:8080" << std::endl;
    server.serve();

    server.shutdown();
}