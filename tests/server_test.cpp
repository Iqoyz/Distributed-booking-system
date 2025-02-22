#include "../server/Inc/UDPServer.h"
#include <boost/asio.hpp>
#include <iostream>
#include <thread>

using namespace std;
using namespace boost::asio;

int main() {
    try {
        io_context io_context;

        // Start UDP server on port 9000
        UDPServer server(io_context, 9000);

        // Run server in a separate thread
        thread serverThread([&io_context]() { io_context.run(); });
        cout << "[TEST] UDP Server is running...\n";

        // Mock client for testing
        udp::resolver resolver(io_context);
        udp::endpoint server_endpoint = *resolver.resolve(udp::v4(), "127.0.0.1", "9000").begin();
        udp::socket client_socket(io_context, udp::endpoint(udp::v4(), 0));

        array<char, 1024> reply{};
        udp::endpoint sender_endpoint;
        size_t len;

        // 1️⃣ Test: Query Availability
        string queryRequest = "QUERY Gym Monday 10 0";
        client_socket.send_to(buffer(queryRequest), server_endpoint);
        len = client_socket.receive_from(buffer(reply), sender_endpoint);
        cout << "[TEST] Response to QUERY: " << string(reply.data(), len) << endl;

        // 2️⃣ Test: Book Facility
        string bookRequest = "BOOK Gym Monday 10 0";
        client_socket.send_to(buffer(bookRequest), server_endpoint);
        len = client_socket.receive_from(buffer(reply), sender_endpoint);
        cout << "[TEST] Response to BOOK: " << string(reply.data(), len) << endl;

        // 3️⃣ Test: Re-query after booking
        client_socket.send_to(buffer(queryRequest), server_endpoint);
        len = client_socket.receive_from(buffer(reply), sender_endpoint);
        cout << "[TEST] Response to QUERY after BOOK: " << string(reply.data(), len) << endl;

        // Shutdown server
        io_context.stop();
        serverThread.join();

        cout << "[TEST] All tests completed successfully.\n";
    } catch (exception &e) {
        cerr << "[ERROR] Exception: " << e.what() << endl;
    }

    return 0;
}
