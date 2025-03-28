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
    // Add a "MeetingRoom" facility so that client queries for it will succeed
    facilities.emplace("MeetingRoom", Facility("MeetingRoom"));
    // Optionally, add an availability slot to MeetingRoom
    facilities.at("MeetingRoom").addAvailability(Facility::TimeSlot(Util::Day::Monday, 900, 1000));

    // Also initialize other facilities as per your test setup
    facilities.emplace("Gym", Facility("Gym"));
    facilities.at("Gym").addAvailability(Facility::TimeSlot(Util::Day::Monday, 1000, 1030));
    facilities.at("Gym").addAvailability(Facility::TimeSlot(Util::Day::Monday, 1030, 1100));
    facilities.at("Gym").addAvailability(Facility::TimeSlot(Util::Day::Monday, 1100, 1130));
    facilities.at("Gym").addAvailability(Facility::TimeSlot(Util::Day::Monday, 1130, 1200));
    facilities.at("Gym").addAvailability(Facility::TimeSlot(Util::Day::Monday, 1230, 1300));
    facilities.at("Gym").addAvailability(Facility::TimeSlot(Util::Day::Monday, 1330, 1400));

    facilities.emplace("Swimming Pool", Facility("Swimming Pool"));
    facilities.at("Swimming Pool")
        .addAvailability(Facility::TimeSlot(Util::Day::Wednesday, 900, 1100));
    facilities.at("Swimming Pool")
        .addAvailability(Facility::TimeSlot(Util::Day::Friday, 1400, 1600));

    facilities.emplace("Tennis Court", Facility("Tennis Court"));
    facilities.at("Tennis Court")
        .addAvailability(Facility::TimeSlot(Util::Day::Monday, 1500, 1700));
    facilities.at("Tennis Court")
        .addAvailability(Facility::TimeSlot(Util::Day::Saturday, 1000, 1200));

    facilities.emplace("Study Room", Facility("Study Room"));
    facilities.at("Study Room").addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 800, 830));
    facilities.at("Study Room").addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 830, 900));
    facilities.at("Study Room").addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 900, 930));
    facilities.at("Study Room").addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 930, 1000));
    facilities.at("Study Room").addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 1000, 1030));
    facilities.at("Study Room").addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 1030, 1100));
    facilities.at("Study Room").addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 1100, 1130));
    facilities.at("Study Room").addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 1130, 1200));
    facilities.at("Study Room")
        .addAvailability(Facility::TimeSlot(Util::Day::Thursday, 1300, 1500));

    facilities.emplace("Fitness Center", Facility("Fitness Center"));
    facilities.at("Fitness Center")
        .addAvailability(Facility::TimeSlot(Util::Day::Friday, 800, 830));
    facilities.at("Fitness Center")
        .addAvailability(Facility::TimeSlot(Util::Day::Friday, 830, 900));
    facilities.at("Fitness Center")
        .addAvailability(Facility::TimeSlot(Util::Day::Friday, 900, 930));
    facilities.at("Fitness Center")
        .addAvailability(Facility::TimeSlot(Util::Day::Friday, 930, 1000));
    facilities.at("Fitness Center")
        .addAvailability(Facility::TimeSlot(Util::Day::Friday, 1000, 1030));
    facilities.at("Fitness Center")
        .addAvailability(Facility::TimeSlot(Util::Day::Friday, 1030, 1100));
    facilities.at("Fitness Center")
        .addAvailability(Facility::TimeSlot(Util::Day::Friday, 1130, 1200));

    cout << "[INFO] Facilities initialized successfully." << endl;
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