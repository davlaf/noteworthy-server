#include "UserConnection.hpp"
#include <iostream>
#include <libwebsockets.h>
#include <string>
#include <vector>

void UserConnection::sendEvent(const std::string &message) {
    // Prepare the message buffer
    size_t message_buffer_length =
        message.length() + 1; // Include null terminator
    std::vector<unsigned char> buf(LWS_PRE + message_buffer_length);

    // Copy the message into the buffer
    memcpy(&buf[LWS_PRE], message.c_str(), message_buffer_length);

    // Send the message back to the client
    size_t message_length = message_buffer_length;
    int result =
        lws_write(socket, &buf[LWS_PRE], message_length, LWS_WRITE_TEXT);

    if (result < 0) {
        // Handle error (log it, clean up, etc.)
        std::cerr << "Failed to send message, code: " << result << std::endl;
    }
}

// dead code RIP

// for (struct UserConnection &connection : ws_connections) {
//     // Prepare the response buffer
//     unsigned char buf[LWS_PRE + len];
//     memcpy(&buf[LWS_PRE], in,
//            len); // Copy the received data to the buffer

//     // Echo the message back to the client
//     size_t message_length = len;
//     lws_write(connection.socket, &buf[LWS_PRE], message_length,
//               LWS_WRITE_TEXT);
// }