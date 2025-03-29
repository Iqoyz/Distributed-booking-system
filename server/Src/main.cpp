#include <boost/asio.hpp>
#include <iostream>
#include "UdpServer.h"
#include "Facility.h"
#include "Util.h"
#include <unordered_map>

using namespace boost::asio;
using namespace std;

// Function to initialize facilities based on server_test.cpp implementation
void initFacilities(unordered_map<string, Facility>& facilities) {
    vector<string> facilityNames = {"MeetingRoom",  "Gym",        "Swimming Pool",
                                    "Tennis Court", "Study Room", "Fitness Center"};

    for (const auto& name : facilityNames) {
        facilities.emplace(name, Facility(name));
        Facility& f = facilities.at(name);

        // Generate slots from 08:00 to 18:00 in 30-minute intervals for Monday to Friday
        for (int d = static_cast<int>(Util::Day::Monday); d <= static_cast<int>(Util::Day::Friday);
             ++d) {
            Util::Day day = static_cast<Util::Day>(d);
            int start = 800;
            while (start < 1800) {
                int end = Util::toHHMM(Util::toMinutes(start) + 30);
                f.addAvailability(Facility::TimeSlot(day, start, end));
                start = end;
            }
        }
    }
    cout << "[INFO] Facilities initialized successfully with full weekday slots (08:00–18:00)."
         << endl;
}

int main() {
    try {
        boost::asio::io_context io_context;

        unordered_map<string, Facility> facilities;
        initFacilities(facilities);  // Initialize facilities with test data

        // Instantiate the UDP server on port 2222 with At-Most-Once semantics (false).
        UDPServer server(io_context, 2222, facilities, false);

        cout << "[Server] Starting UDP Server on port 2222..." << endl;
        // Run the server. This call will block and continuously handle incoming UDP requests.
        server.start();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}