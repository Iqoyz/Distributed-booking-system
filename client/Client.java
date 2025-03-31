package client;

import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Scanner;

public class Client {
    // Operation codes (adjust as needed)
    private static final byte OP_QUERY   = 1;
    private static final byte OP_BOOK    = 2;
    private static final byte OP_CHANGE  = 3;
    private static final byte OP_MONITOR = 4;
    private static final byte OP_EXTEND  = 5;
    private static final byte OP_CANCEL  = 6;
    
    private static int requestIdCounter = 1;
    private static String invocation = "most"; // 'least' or 'most' (for the server to decide which one to use)

    public static void main(String[] args) {
        try (Scanner scanner = new Scanner(System.in)) {
            // Prompt for server address and port
            System.out.print("Enter server ip address (e.g., 192.168.1.10): ");
            String serverAddress = scanner.nextLine().trim();
            System.out.print("Enter server port (e.g., 2222): ");
            int serverPort = Integer.parseInt(scanner.nextLine().trim());
            
            try (DatagramSocket socket = new DatagramSocket()) {
                InetAddress serverInetAddress = InetAddress.getByName(serverAddress);
            
                while (true) {
                    System.out.println("\n===== Facility Booking Client =====");
                    System.out.println("1. Query Facility Availability");
                    System.out.println("2. Book Facility");
                    System.out.println("3. Change Booking");
                    System.out.println("4. Monitor Facility Availability");
                    System.out.println("5. Extend Booking");
                    System.out.println("6. Cancel Booking");
                    System.out.println("7. Exit");
                    System.out.print("Enter your choice: ");
                    
                    int choice = Integer.parseInt(scanner.nextLine().trim());
                    if (choice == 7) {
                        System.out.println("Exiting client.");
                        break;
                    }
                    
                    byte operation;
                    String facilityName = "";
                    byte day = 0;
                    short startTime = 0;
                    short endTime = 0;
                    int extraField = 0; // used for change booking (offset), monitor (interval), extend (minutes) or cancel (booking id)
                    int bookingId = 0;
                    boolean includeExtra = false;
                    
                    switch (choice) {
                        case 1:
                            // Query facility availability: needs facility and day.
                            operation = OP_QUERY;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter number of days to query for (1-7): ");
                            int numDays = Integer.parseInt(scanner.nextLine().trim());
                            if (numDays < 1 || numDays > 7) {
                                System.out.println("Invalid number of days. Please choose between 1 and 7.");
                                continue;
                            }
                            if (numDays == 1) {
                                System.out.print("Enter day (0=Monday, 1=Tuesday, ...): ");
                                day = Byte.parseByte(scanner.nextLine().trim());
                                // For query, startTime and endTime are not used. Set them to 0.
                                startTime = 0;
                                endTime = 0;
                            } else {
                                // For multiple days, loop to send a query request for each day.
                                for (int i = 0; i < numDays; i++) {
                                    System.out.print("Enter day " + (i + 1) + " (0=Monday, 1=Tuesday, ...): ");
                                    byte d = Byte.parseByte(scanner.nextLine().trim());
                                    // For query, startTime and endTime are not used.
                                    short queryStart = 0;
                                    short queryEnd = 0;
                                    
                                    // Construct the RequestMessage binary payload for the query.
                                    byte[] facilityNameBytes = facilityName.getBytes("UTF-8");
                                    int totalLength = 4 + 1 + 2 + facilityNameBytes.length + 1 + 2 + 2; // base fields
                                    // No extra field for query
                                    ByteBuffer buffer = ByteBuffer.allocate(totalLength);
                                    buffer.order(ByteOrder.BIG_ENDIAN);
                                    buffer.putInt(requestIdCounter++);
                                    buffer.put(operation);
                                    buffer.putShort((short) facilityNameBytes.length);
                                    buffer.put(facilityNameBytes);
                                    buffer.put(d);
                                    buffer.putShort(queryStart);
                                    buffer.putShort(queryEnd);
                                    byte[] requestData = buffer.array();
                                    
                                    // Send the query request via UDP
                                    DatagramPacket sendPacket = new DatagramPacket(requestData, requestData.length, serverInetAddress, serverPort);
                                    socket.send(sendPacket);
                                    System.out.println("Query request for day " + d + " sent.");
                                    
                                    // Wait for the response with timeout/resend logic
                                    long totalStartTime = System.currentTimeMillis();
                                    boolean responseReceived = false;
                                    byte[] responseBuffer = new byte[1024];
                                    DatagramPacket receivePacket = new DatagramPacket(responseBuffer, responseBuffer.length);
                                    socket.setSoTimeout(10000); // 10-second timeout
                                    
                                    while (!responseReceived && (System.currentTimeMillis() - totalStartTime < 30000)) {
                                        try {
                                            socket.receive(receivePacket);
                                            responseReceived = true;
                                        } catch (java.net.SocketTimeoutException e) {
                                            if (System.currentTimeMillis() - totalStartTime < 30000) {
                                                System.out.println("No response received in 10 seconds. Resending query for day " + d + "...");
                                                socket.send(sendPacket);
                                            }
                                        }
                                    }
                                    
                                    if (!responseReceived) {
                                        System.out.println("Error: No response received from server for day " + d + " after 30 seconds.");
                                        continue;
                                    }
                                    
                                    // Reset timeout
                                    socket.setSoTimeout(0);
                                    
                                    ByteBuffer respBuffer = ByteBuffer.wrap(receivePacket.getData(), 0, receivePacket.getLength());
                                    respBuffer.order(ByteOrder.BIG_ENDIAN);
                                    int respRequestId = respBuffer.getInt();
                                    byte status = respBuffer.get();
                                    short messageLength = respBuffer.getShort();
                                    byte[] messageBytes = new byte[messageLength];
                                    respBuffer.get(messageBytes);
                                    String responseMessage = new String(messageBytes, "UTF-8");
                                    System.out.println("Response for day " + d + " received:");
                                    System.out.println("Request ID: " + respRequestId);
                                    System.out.println("Status: " + status);
                                    System.out.println("Message: " + responseMessage);
                                }
                                // After processing all days, return to the main menu.
                                continue;
                            }
                            break;
                        case 2:
                            // Book facility: needs facility, day, startTime, and endTime.
                            operation = OP_BOOK;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter day (0=Monday, 1=Tuesday, ...): ");
                            day = Byte.parseByte(scanner.nextLine().trim());
                            System.out.print("Enter start time (e.g., 900 for 9:00 AM): ");
                            startTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter end time (e.g., 1000 for 10:00 AM): ");
                            endTime = Short.parseShort(scanner.nextLine().trim());
                            break;
                        case 3:
                            // Modify booking: needs facility, booking ID, and offsetMinutes.
                            // Other fields are not used.
                            operation = OP_CHANGE;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter booking ID: ");
                            bookingId = Integer.parseInt(scanner.nextLine().trim());
                            System.out.print("Enter offset (in minutes, can be negative): ");
                            extraField = Integer.parseInt(scanner.nextLine().trim());
                            // Set unused fields to 0.
                            day = 0;
                            startTime = 0;
                            endTime = 0;
                            includeExtra = true;
                            break;
                        case 4:
                            // Monitor facility availability: needs facility, day, startTime, endTime, and monitor interval.
                            operation = OP_MONITOR;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter day (0=Monday, 1=Tuesday, ...): ");
                            day = Byte.parseByte(scanner.nextLine().trim());
                            System.out.print("Enter start time (e.g., 900 for 9:00 AM): ");
                            startTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter end time (e.g., 1000 for 10:00 AM): ");
                            endTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter monitor interval (in seconds): ");
                            extraField = Integer.parseInt(scanner.nextLine().trim());
                            includeExtra = true;
                            break;
                        case 5:
                            // Extend booking: needs facility, booking ID, and extension minutes.
                            operation = OP_EXTEND;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter booking ID: ");
                            bookingId = Integer.parseInt(scanner.nextLine().trim());
                            System.out.print("Enter extension minutes (e.g., 30 or 60): ");
                            extraField = Integer.parseInt(scanner.nextLine().trim());
                            // Unused fields are set to 0.
                            day = 0;
                            startTime = 0;
                            endTime = 0;
                            includeExtra = true;
                            break;
                        case 6:
                            // Cancel booking: needs facility and booking ID.
                            operation = OP_CANCEL;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter booking ID: ");
                            bookingId = Integer.parseInt(scanner.nextLine().trim());
                            // For cancel, the extra field holds the booking ID.
                            extraField = bookingId;
                            // Set unused fields to 0.
                            day = 0;
                            startTime = 0;
                            endTime = 0;
                            includeExtra = true;
                            break;
                        default:
                            System.out.println("Invalid choice. Please try again.");
                            continue;
                    }
                    
                    // Construct the RequestMessage binary payload.
                    // Base Structure:
                    // [Request ID (4 bytes)] [Operation (1 byte)]
                    // [Facility Name Length (2 bytes)] [Facility Name (variable)]
                    // [Day (1 byte)] [Start Time (2 bytes)] [End Time (2 bytes)]
                    // Extra Field:
                    // - For Change or Extend: 8 bytes (4 bytes bookingId, 4 bytes offset/extend minutes)
                    // - For Monitor or Cancel: 4 bytes (monitor interval or booking id)
                    byte[] facilityNameBytes = facilityName.getBytes("UTF-8");
                    int totalLength = 4 + 1 + 2 + facilityNameBytes.length + 1 + 2 + 2; // base fields
                    if (operation == OP_CHANGE || operation == OP_EXTEND) {
                        totalLength += 8; // 4 bytes for bookingId and 4 bytes for offset/extend minutes
                    } else if (operation == OP_MONITOR || operation == OP_CANCEL) {
                        totalLength += 4; // 4 bytes for monitor interval or booking id
                    }

                    ByteBuffer buffer = ByteBuffer.allocate(totalLength);
                    buffer.order(ByteOrder.BIG_ENDIAN);
                    buffer.putInt(requestIdCounter++);
                    buffer.put(operation);
                    buffer.putShort((short) facilityNameBytes.length);
                    buffer.put(facilityNameBytes);
                    buffer.put(day);
                    buffer.putShort(startTime);
                    buffer.putShort(endTime);
                    if (operation == OP_CHANGE || operation == OP_EXTEND) {
                        buffer.putInt(bookingId);
                        buffer.putInt(extraField); // offset (for change) or extend minutes (for extend)
                    } else if (operation == OP_MONITOR || operation == OP_CANCEL) {
                        buffer.putInt(extraField);
                    }

                    byte[] requestData = buffer.array();
                    
                    // Send the request via UDP
                    DatagramPacket sendPacket = new DatagramPacket(requestData, requestData.length, serverInetAddress, serverPort);
                    socket.send(sendPacket);
                    System.out.println("Request sent.");
                    
                    // Wait for the confirmation response using timeout/resend logic, depending on invocation semantic.
                    DatagramPacket receivePacket;
                    if (invocation.equals("least")) {
                        // At least once semantics: keep resending indefinitely until a response is received.
                        boolean responseReceived = false;
                        socket.setSoTimeout(10000); // Set timeout to 10 seconds for each attempt.
                        receivePacket = new DatagramPacket(new byte[1024], 1024);
                        while (!responseReceived) {
                            try {
                                socket.receive(receivePacket);
                                responseReceived = true;
                            } catch (java.net.SocketTimeoutException e) {
                                System.out.println("No response received in 10 seconds. Resending request...");
                                socket.send(sendPacket);
                            }
                        }
                        // Reset timeout to infinite after receiving response.
                        socket.setSoTimeout(0);
                    } else { // "most" semantics: at most once invocation.
                        long totalStartTime = System.currentTimeMillis();
                        boolean responseReceived = false;
                        receivePacket = new DatagramPacket(new byte[1024], 1024);
                        socket.setSoTimeout(10000); // Set timeout to 10 seconds.
                        while (!responseReceived && (System.currentTimeMillis() - totalStartTime < 30000)) {
                            try {
                                socket.receive(receivePacket);
                                responseReceived = true;
                            } catch (java.net.SocketTimeoutException e) {
                                if (System.currentTimeMillis() - totalStartTime < 30000) {
                                    System.out.println("No response received in 10 seconds. Resending request...");
                                    socket.send(sendPacket);
                                }
                            }
                        }
                        if (!responseReceived) {
                            System.out.println("Error: No response received from server after 30 seconds.");
                            continue; // Return to main menu.
                        }
                        // Reset timeout to infinite after receiving response.
                        socket.setSoTimeout(0);
                    }
                    
                    ByteBuffer respBuffer = ByteBuffer.wrap(receivePacket.getData(), 0, receivePacket.getLength());
                    respBuffer.order(ByteOrder.BIG_ENDIAN);
                    
                    // ResponseMessage structure:
                    // [Request ID (4 bytes)] [Status (1 byte)] [Message Length (2 bytes)] [Message (variable)]
                    int respRequestId = respBuffer.getInt();
                    byte status = respBuffer.get();
                    short messageLength = respBuffer.getShort();
                    byte[] messageBytes = new byte[messageLength];
                    respBuffer.get(messageBytes);
                    String responseMessage = new String(messageBytes, "UTF-8");
                    
                    System.out.println("Response received:");
                    System.out.println("Request ID: " + respRequestId);
                    System.out.println("Status: " + status);
                    System.out.println("Message: " + responseMessage);
                    
                    // If the operation is monitor, block further input until the monitor interval expires
                    if (operation == OP_MONITOR) {
                        // extraField holds the monitor interval (in seconds)
                        long monitorDurationMillis = extraField * 1000L;
                        long endTimeMonitor = System.currentTimeMillis() + monitorDurationMillis;
                        System.out.println("Monitoring facility " + facilityName + " for " + extraField + " seconds. Waiting for updates...");
                        
                        // Set a short socket timeout (e.g., 1000ms) so that receive() doesn't block indefinitely
                        socket.setSoTimeout(1000);
                        
                        while (System.currentTimeMillis() < endTimeMonitor) {
                            try {
                                byte[] monitorBuffer = new byte[1024];
                                DatagramPacket monitorPacket = new DatagramPacket(monitorBuffer, monitorBuffer.length);
                                socket.receive(monitorPacket);
                                ByteBuffer monitorRespBuffer = ByteBuffer.wrap(monitorPacket.getData(), 0, monitorPacket.getLength());
                                monitorRespBuffer.order(ByteOrder.BIG_ENDIAN);
                                @SuppressWarnings("unused")
                                int monitorRespRequestId = monitorRespBuffer.getInt();
                                @SuppressWarnings("unused")
                                byte monitorStatus = monitorRespBuffer.get();
                                short monitorMessageLength = monitorRespBuffer.getShort();
                                byte[] monitorMessageBytes = new byte[monitorMessageLength];
                                monitorRespBuffer.get(monitorMessageBytes);
                                String monitorResponseMessage = new String(monitorMessageBytes, "UTF-8");
                                System.out.println("Monitor Update: " + monitorResponseMessage);
                            } catch (java.net.SocketTimeoutException e) {
                                // No update received in this interval; continue waiting
                            }
                        }
                        // Reset socket timeout to infinite
                        socket.setSoTimeout(0);
                        System.out.println("Monitoring interval expired. Returning to main menu.");
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}