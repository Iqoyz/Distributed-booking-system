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

int main() {
  try {
    io_context io_context;
    unordered_map<string, Facility> facilities;

    initFacility(facilities);

    // Start UDP server on port 9000
    UDPServer server(io_context, 9000, facilities);

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
      Facility::TimeSlot(Util::Day::Monday, 1000, 1100));
  facilities.at("Gym").addAvailability(
      Facility::TimeSlot(Util::Day::Monday, 1100, 1200));
  facilities.at("Gym").addAvailability(
      Facility::TimeSlot(Util::Day::Tuesday, 1300, 1500));

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
      .addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 800, 1000));
  facilities.at("Study Room")
      .addAvailability(Facility::TimeSlot(Util::Day::Thursday, 1300, 1500));

  facilities.emplace("Fitness Center", Facility("Fitness Center"));
  facilities.at("Fitness Center")
      .addAvailability(Facility::TimeSlot(Util::Day::Monday, 600, 800));
  facilities.at("Fitness Center")
      .addAvailability(Facility::TimeSlot(Util::Day::Friday, 1800, 2000));

  cout << "[INFO] Facilities initialized successfully.\n";
}

// -----------------------------
// MONITORING TEST
// -----------------------------
void monitorTest(io_context &io_context, const udp::endpoint &server_endpoint) {
  cout << "[MONITER TEST]\n";
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
  monitorRequest.extraMessage = 10; // Monitor for 10 seconds

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

  for (int i = 0; i < 3; ++i) // Simulate 3 notifications or timeout
  {
    recv_buffer.fill(0);

    // Create a deadline timer for 15 seconds
    steady_timer timer(io_context);
    timer.expires_after(std::chrono::seconds(15));

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
  cancelRequest.extraMessage = bookingId;

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
  // QUERY TEST CASE 1: Available Slot
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
