#include "../server/Inc/Message.h"
#include "../server/Inc/UDPServer.h"
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <regex>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace boost::asio;

void initFacility(unordered_map<string, Facility> &facilities);

void queryTest(io_context &io_context, const udp::endpoint &server_endpoint);

void monitorTest(io_context &io_context, const udp::endpoint &server_endpoint);
void clientAMonitor(io_context &io_context,
                    const udp::endpoint &server_endpoint);
void clientBMonitor(io_context &io_context,
                    const udp::endpoint &server_endpoint);
void modifyTest(io_context &io_context, const udp::endpoint &server_endpoint);
void extendTest(io_context &io_context, const udp::endpoint &server_endpoint);

int main() {
  try {
    io_context io_context;
    unordered_map<string, Facility> facilities;

    initFacility(facilities);

    bool isAtLeastOnce = false; // use at most once mode

    // Start UDP server on port 9000
    UDPServer server(io_context, 9000, facilities, isAtLeastOnce);

    // Run server in a separate thread
    thread serverThread([&io_context]() { io_context.run(); });
    cout << "[TEST] UDP Server is running...\n";

    // Mock client setup
    udp::resolver resolver(io_context);
    udp::endpoint server_endpoint =
        *resolver.resolve(udp::v4(), "127.0.0.1", "9000").begin();

    // -----------------------------
    // QUERY TEST
    // -----------------------------
    queryTest(io_context, server_endpoint);

    // -----------------------------
    // MODIFY TEST
    // -----------------------------
    modifyTest(io_context, server_endpoint);

    // -----------------------------
    // EXTEND TEST
    // -----------------------------
    extendTest(io_context, server_endpoint);

    // -----------------------------
    // MONITORING TEST
    // -----------------------------
    monitorTest(io_context, server_endpoint);

    // Shutdown server after test
    io_context.stop();
    serverThread.join();

    cout << "[TEST] All tests completed successfully.\n";
  } catch (const exception &e) {
    cerr << "[ERROR] Exception: " << e.what() << endl;
  }

  return 0;
}

// Function to initialize multiple facilities
void initFacility(unordered_map<string, Facility> &facilities) {
  facilities.emplace("Gym", Facility("Gym"));
  facilities.at("Gym").addAvailability(
      Facility::TimeSlot(Util::Day::Monday, 1000, 1030));
  facilities.at("Gym").addAvailability(
      Facility::TimeSlot(Util::Day::Monday, 1030, 1100));
  facilities.at("Gym").addAvailability(
      Facility::TimeSlot(Util::Day::Monday, 1100, 1130));
  facilities.at("Gym").addAvailability(
      Facility::TimeSlot(Util::Day::Monday, 1130, 1200));
  facilities.at("Gym").addAvailability(
      Facility::TimeSlot(Util::Day::Monday, 1230, 1300));
  facilities.at("Gym").addAvailability(
      Facility::TimeSlot(Util::Day::Monday, 1330, 1400));

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
  facilities.at("Study Room")
      .addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 800, 830));
  facilities.at("Study Room")
      .addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 830, 900));
  facilities.at("Study Room")
      .addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 900, 930));
  facilities.at("Study Room")
      .addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 930, 1000));
  facilities.at("Study Room")
      .addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 1000, 1030));
  facilities.at("Study Room")
      .addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 1030, 1100));
  facilities.at("Study Room")
      .addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 1100, 1130));
  facilities.at("Study Room")
      .addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 1130, 1200));
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
  cout << "[INFO] Facilities initialized successfully.\n";
}

// -----------------------------
// MODIFY TEST
// -----------------------------
void modifyTest(io_context &io_context, const udp::endpoint &server_endpoint) {
  cout << "\n[MODIFY TEST]\n";

  udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));
  array<uint8_t, 1024> recv_buffer{};
  udp::endpoint sender_endpoint;
  vector<uint8_t> responseData;

  // -----------------------------
  // Step 1: Book a slot (10:00–11:00)
  // -----------------------------
  RequestMessage bookRequest;
  bookRequest.requestId = 2001;
  bookRequest.operation = Operation::BOOK;
  bookRequest.facilityName = "Study Room";
  bookRequest.day = Util::Day::Tuesday;
  bookRequest.startTime = 800;
  bookRequest.endTime = 900;

  socket.send_to(buffer(bookRequest.marshal()), server_endpoint);

  size_t len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  ResponseMessage bookResponse = ResponseMessage::unmarshal(responseData);
  cout << "[MODIFY TEST] Book Response: " << bookResponse.message << endl;

  // Extract booking ID from bookResponse
  uint32_t bookingId = 0;
  smatch match;
  regex bookingIdPattern(R"(Booking ID:\s*(\d+))");

  if (regex_search(bookResponse.message, match, bookingIdPattern) &&
      match.size() > 1) {
    bookingId = stoi(match[1]);
    cout << "[MODIFY TEST] Extracted Booking ID: " << bookingId << endl;
  } else {
    cerr << "[MODIFY TEST] Failed to extract booking ID.\n";
    return;
  }

  // -----------------------------
  // Step 2: Modify the booking (offset by +30 mins → 10:30–11:30)
  // -----------------------------
  std::this_thread::sleep_for(std::chrono::seconds(2)); // Optional delay

  RequestMessage modifyRequest;
  modifyRequest.requestId = 2002;
  modifyRequest.operation = Operation::CHANGE;
  modifyRequest.facilityName = "Study Room";
  modifyRequest.day =
      Util::Day::Tuesday;        // not really needed for modify but safe
  modifyRequest.startTime = 800; // original time (for reference/log)
  modifyRequest.endTime = 900;   // original time
  modifyRequest.bookingId = bookingId;
  modifyRequest.offsetMinutes = 30; // shift by 30 mins

  socket.send_to(buffer(modifyRequest.marshal()), server_endpoint);

  // Receive MODIFY response
  recv_buffer.fill(0);
  len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  ResponseMessage modifyResponse = ResponseMessage::unmarshal(responseData);
  cout << "[MODIFY TEST] Modify Response: " << modifyResponse.message << endl;

  cout << "[MODIFY TEST] Modify test completed.\n\n";
}

// -----------------------------
// EXTEND TEST
// -----------------------------
void extendTest(io_context &io_context, const udp::endpoint &server_endpoint) {
  cout << "\n[EXTEND TEST]\n";

  udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));
  array<uint8_t, 1024> recv_buffer{};
  udp::endpoint sender_endpoint;
  vector<uint8_t> responseData;

  // -----------------------------
  // Step 1: Book a 30-min slot (08:00–08:30)
  // -----------------------------
  RequestMessage bookRequest;
  bookRequest.requestId = 3001;
  bookRequest.operation = Operation::BOOK;
  bookRequest.facilityName = "Fitness Center";
  bookRequest.day = Util::Day::Friday;
  bookRequest.startTime = 900;
  bookRequest.endTime = 930;

  socket.send_to(buffer(bookRequest.marshal()), server_endpoint);

  size_t len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  ResponseMessage bookResponse = ResponseMessage::unmarshal(responseData);
  cout << "[EXTEND TEST] Book Response: " << bookResponse.message << endl;

  // Extract booking ID from bookResponse
  uint32_t bookingId = 0;
  smatch match;
  regex bookingIdPattern(R"(Booking ID:\s*(\d+))");

  if (regex_search(bookResponse.message, match, bookingIdPattern) &&
      match.size() > 1) {
    bookingId = stoi(match[1]);
    cout << "[EXTEND TEST] Extracted Booking ID: " << bookingId << endl;
  } else {
    cerr << "[EXTEND TEST] Failed to extract booking ID.\n";
    return;
  }

  // -----------------------------
  // Step 2: Extend the booking by 30 minutes → (08:00–09:00)
  // -----------------------------
  std::this_thread::sleep_for(std::chrono::seconds(2)); // Optional delay

  RequestMessage extendRequest;
  extendRequest.requestId = 3002;
  extendRequest.operation = Operation::EXTEND;
  extendRequest.facilityName = "Fitness Center";
  extendRequest.day = Util::Day::Friday;
  extendRequest.startTime = 900;
  extendRequest.endTime = 930;
  extendRequest.bookingId = bookingId;
  extendRequest.offsetMinutes = 60;

  socket.send_to(buffer(extendRequest.marshal()), server_endpoint);

  // Receive EXTEND response
  recv_buffer.fill(0);
  len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  ResponseMessage extendResponse = ResponseMessage::unmarshal(responseData);
  cout << "[EXTEND TEST] Extend Response: " << extendResponse.message << endl;

  cout << "[EXTEND TEST] Extend test completed.\n\n";
}

// -----------------------------
// MONITORING TEST
// -----------------------------
void monitorTest(io_context &io_context, const udp::endpoint &server_endpoint) {
  cout << "[MONITER TEST]";
  // Launch separate threads for each client
  thread clientAThread(clientAMonitor, ref(io_context), ref(server_endpoint));
  thread clientBThread(clientBMonitor, ref(io_context), ref(server_endpoint));

  // Wait for clients to finish
  clientAThread.join();
  clientBThread.join();
}

// Client A: Monitor Gym from 10:00 to 12:00
void clientAMonitor(io_context &io_context,
                    const udp::endpoint &server_endpoint) {
  udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));
  vector<uint8_t> responseData;
  ResponseMessage response;

  // Send MONITOR request
  cout << "\n[Client A] Sending MONITOR request for Gym (10:00 to 12:00)...\n";

  RequestMessage monitorRequest;
  monitorRequest.requestId = 1001;
  monitorRequest.operation = Operation::MONITOR;
  monitorRequest.facilityName = "Gym";
  monitorRequest.day = Util::Day::Monday;
  monitorRequest.startTime = 1000;
  monitorRequest.endTime = 1200;
  monitorRequest.monitorInterval = 20; // Monitor for 20 seconds

  socket.send_to(buffer(monitorRequest.marshal()), server_endpoint);

  // Receive MONITOR response
  array<uint8_t, 1024> recv_buffer{};
  udp::endpoint sender_endpoint;
  size_t len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  response = ResponseMessage::unmarshal(responseData);
  cout << "[Client A]: Monitor Response: " << response.message << endl;

  // Wait for notifications with timeout
  cout << "[Client A] Waiting for booking/cancellation notifications (timeout: "
          "15s)...\n";

  for (int i = 0; i < 20; ++i) // Simulate 3 notifications or timeout
  {
    recv_buffer.fill(0);

    // Create a deadline timer for 15 seconds
    steady_timer timer(io_context);
    timer.expires_after(std::chrono::seconds(20));

    bool notificationReceived = false;

    // Start asynchronous wait for the timer
    timer.async_wait([&](const boost::system::error_code &ec) {
      if (!ec) {
        cout << "[Client A] Timeout reached without notifications. Ending "
                "thread.\n";
        socket.cancel(); // Cancel the socket to break the blocking call
      }
    });

    try {
      // Blocking receive with timer running
      len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
      timer.cancel(); // Cancel the timer if message received in time

      responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
      response = ResponseMessage::unmarshal(responseData);
      cout << "[Client A] Notification received: " << response.message << endl;
      notificationReceived = true;
    } catch (const boost::system::system_error &e) {
      cout << "[Client A] Receive operation was canceled due to timeout.\n";
      break; // Exit the loop on timeout
    }
  }

  cout << "[Client A] Monitoring ended.\n";
}

// Client B: Book Gym from 10:00 to 11:00, Cancel Booking, and Book Again
void clientBMonitor(io_context &io_context,
                    const udp::endpoint &server_endpoint) {
  std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate delay
  udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));

  array<uint8_t, 1024> recv_buffer{};
  udp::endpoint sender_endpoint;
  vector<uint8_t> responseData;

  // -----------------------------
  // Step 1: Book Gym (10:00 to 11:00)
  // -----------------------------
  cout << "\n[Client B] Sending BOOK request for Gym (10:00 to 11:00)...\n";

  RequestMessage bookRequest;
  bookRequest.requestId = 1002;
  bookRequest.operation = Operation::BOOK;
  bookRequest.facilityName = "Gym";
  bookRequest.day = Util::Day::Monday;
  bookRequest.startTime = 1000;
  bookRequest.endTime = 1100;

  socket.send_to(buffer(bookRequest.marshal()), server_endpoint);

  // Receive BOOK response
  recv_buffer.fill(0);
  size_t len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  ResponseMessage bookResponse = ResponseMessage::unmarshal(responseData);
  cout << "[Client B] Book Response: " << bookResponse.message << endl;

  // Extract Booking ID from response using regex
  uint32_t bookingId = 0;
  smatch match;
  regex bookingIdPattern(R"(Booking ID:\s*(\d+))");

  if (regex_search(bookResponse.message, match, bookingIdPattern) &&
      match.size() > 1) {
    bookingId = stoi(match[1]);
    cout << "[Client B] Extracted Booking ID: " << bookingId << endl;
  } else {
    cerr << "[Client B] Failed to extract booking ID from response.\n";
    return;
  }

  // -----------------------------
  // Step 2: Cancel Booking using Booking ID
  // -----------------------------
  std::this_thread::sleep_for(std::chrono::seconds(4)); // Simulate delay
  cout << "\n[Client B] Sending CANCEL request for Booking ID: " << bookingId
       << "...\n";

  RequestMessage cancelRequest;
  cancelRequest.requestId = 1003;
  cancelRequest.operation = Operation::CANCEL;
  cancelRequest.facilityName = "Gym";
  cancelRequest.bookingId = bookingId;

  socket.send_to(buffer(cancelRequest.marshal()), server_endpoint);

  // Receive CANCEL response
  recv_buffer.fill(0);
  len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  ResponseMessage cancelResponse = ResponseMessage::unmarshal(responseData);
  cout << "[Client B] Cancel Response: " << cancelResponse.message << endl;

  // -----------------------------
  // Step 3: Book Gym again (10:00 to 11:00)
  // -----------------------------
  std::this_thread::sleep_for(std::chrono::seconds(5)); // Simulate delay
  cout << "\n[Client B] Sending BOOK request for Gym (10:00 to 11:00) "
          "again...\n";

  RequestMessage bookAgainRequest;
  bookAgainRequest.requestId = 1004;
  bookAgainRequest.operation = Operation::BOOK;
  bookAgainRequest.facilityName = "Gym";
  bookAgainRequest.day = Util::Day::Monday;
  bookAgainRequest.startTime = 1000;
  bookAgainRequest.endTime = 1100;

  socket.send_to(buffer(bookAgainRequest.marshal()), server_endpoint);

  // Receive BOOK response for the second booking
  recv_buffer.fill(0);
  len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(),
                      recv_buffer.begin() + len); // Reuse responseData
  ResponseMessage bookAgainResponse = ResponseMessage::unmarshal(responseData);
  cout << "[Client B] Book Again Response: " << bookAgainResponse.message
       << endl;

  // Extract new Booking ID
  uint32_t newBookingId = 0;
  if (regex_search(bookAgainResponse.message, match, bookingIdPattern) &&
      match.size() > 1) {
    newBookingId = stoi(match[1]);
    cout << "[Client B] Extracted New Booking ID: " << newBookingId << endl;
  } else {
    cerr << "[Client B] Failed to extract new booking ID from response.\n";
    return;
  }

  // -----------------------------
  // Step 4: Modify Gym Booking from (10:00 to 11:00) to (10:30 to 11:30)
  // -----------------------------
  std::this_thread::sleep_for(std::chrono::seconds(2)); // Optional delay
  cout << "\n[Client B] Sending MODIFY request to shift booking +30 "
          "minutes...\n";

  RequestMessage modifyRequest;
  modifyRequest.requestId = 1005;
  modifyRequest.operation = Operation::CHANGE;
  modifyRequest.facilityName = "Gym";
  modifyRequest.day = Util::Day::Monday;
  modifyRequest.startTime = 1000; // Optional (used for logs)
  modifyRequest.endTime = 1100;
  modifyRequest.bookingId = newBookingId; // Use the booking ID from Step 3
  modifyRequest.offsetMinutes = 30;       // Shift by 30 minutes

  socket.send_to(buffer(modifyRequest.marshal()), server_endpoint);

  // Receive MODIFY response
  recv_buffer.fill(0);
  len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  ResponseMessage modifyResponse = ResponseMessage::unmarshal(responseData);
  cout << "[Client B] Modify Response: " << modifyResponse.message << endl;

  // -----------------------------
  // Step 5: Extend Gym Booking from (10:30 to 11:30) to (10:30 to 12:00)
  // -----------------------------
  std::this_thread::sleep_for(std::chrono::seconds(2)); // Optional delay
  cout << "\n[Client B] Sending EXTEND request to extend booking +30 "
          "minutes...\n";

  RequestMessage extendRequest;
  extendRequest.requestId = 1006;
  extendRequest.operation = Operation::EXTEND; // Make sure EXTEND is defined
  extendRequest.facilityName = "Gym";
  extendRequest.day = Util::Day::Monday;
  extendRequest.startTime = 1030; // Optional - for logs
  extendRequest.endTime = 1130;
  extendRequest.bookingId = newBookingId;
  extendRequest.offsetMinutes = 30; // Extend duration by +30 minutes

  socket.send_to(buffer(extendRequest.marshal()), server_endpoint);

  // Receive EXTEND response
  recv_buffer.fill(0);
  len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  ResponseMessage extendResponse = ResponseMessage::unmarshal(responseData);
  cout << "[Client B] Extend Response: " << extendResponse.message << endl;

  cout << "[Client B] Workflow completed successfully.\n";
}

// -----------------------------
// QUERY TEST
// -----------------------------
void queryTest(io_context &io_context, const udp::endpoint &server_endpoint) {
  udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));
  array<uint8_t, 1024> recv_buffer{};
  udp::endpoint sender_endpoint;
  vector<uint8_t> responseData;
  ResponseMessage response;

  // -----------------------------
  // QUERY TEST CASE 1: Available Slot （at most once）
  // -----------------------------
  cout << "\n[QUERY TEST] Sending QUERY request for Gym (10:00 to 11:00)...\n";

  RequestMessage queryRequest1;
  queryRequest1.requestId = 2001;
  queryRequest1.operation = Operation::QUERY;
  queryRequest1.facilityName = "Gym";
  queryRequest1.day = Util::Day::Monday;
  queryRequest1.startTime = 1000; // 10:00
  queryRequest1.endTime = 1100;   // 11:00

  // Send QUERY request
  socket.send_to(buffer(queryRequest1.marshal()), server_endpoint);

  // Receive response
  size_t len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  response = ResponseMessage::unmarshal(responseData);

  cout << "[QUERY TEST] Response: " << response.message << endl;

  // -----------------------------
  // QUERY TEST CASE 1.2: Available Slot （send duplicate request, for testing
  // at most once）
  // -----------------------------
  // Send same QUERY request again
  cout << "\n[QUERY TEST] Sending QUERY request for Gym (10:00 to 11:00)... "
          "(duplicate)\n";
  socket.send_to(buffer(queryRequest1.marshal()), server_endpoint);

  // Receive response
  len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  response = ResponseMessage::unmarshal(responseData);

  cout << "[QUERY TEST] Response: " << response.message << endl;

  // -----------------------------
  // QUERY TEST CASE 2: Unavailable Slot
  // -----------------------------
  cout << "\n[QUERY TEST] Sending QUERY request for Gym (13:00 to 14:00)...\n";

  RequestMessage queryRequest2;
  queryRequest2.requestId = 2002;
  queryRequest2.operation = Operation::QUERY;
  queryRequest2.facilityName = "Gym";
  queryRequest2.day = Util::Day::Monday;
  queryRequest2.startTime = 1300; // 13:00
  queryRequest2.endTime = 1400;   // 14:00

  // Send QUERY request
  socket.send_to(buffer(queryRequest2.marshal()), server_endpoint);

  // Receive response
  len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  response = ResponseMessage::unmarshal(responseData);

  cout << "[QUERY TEST] Response: " << response.message << endl;

  // -----------------------------
  // QUERY TEST CASE 3: Invalid Facility
  // -----------------------------
  cout << "\n[QUERY TEST] Sending QUERY request for Nonexistent Facility...\n";

  RequestMessage queryRequest3;
  queryRequest3.requestId = 2003;
  queryRequest3.operation = Operation::QUERY;
  queryRequest3.facilityName = "InvalidFacility";
  queryRequest3.day = Util::Day::Monday;
  queryRequest3.startTime = 1000;
  queryRequest3.endTime = 1100;

  // Send QUERY request
  socket.send_to(buffer(queryRequest3.marshal()), server_endpoint);

  // Receive response
  len = socket.receive_from(buffer(recv_buffer), sender_endpoint);
  responseData.assign(recv_buffer.begin(), recv_buffer.begin() + len);
  response = ResponseMessage::unmarshal(responseData);

  cout << "[QUERY TEST] Response: " << response.message << endl;

  cout << "\n[QUERY TEST] All query tests completed successfully.\n";
}
