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
                            // Query facility availability
                            operation = OP_QUERY;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter day (0=Monday, 1=Tuesday, ...): ");
                            day = Byte.parseByte(scanner.nextLine().trim());
                            System.out.print("Enter start time (e.g., 900 for 9:00 AM): ");
                            startTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter end time (e.g., 1000 for 10:00 AM): ");
                            endTime = Short.parseShort(scanner.nextLine().trim());
                            break;
                        case 2:
                            // Book facility
                            operation = OP_BOOK;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter day (0=Monday, 1=Tuesday, ...): ");
                            day = Byte.parseByte(scanner.nextLine().trim());
                            System.out.print("Enter start time: ");
                            startTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter end time: ");
                            endTime = Short.parseShort(scanner.nextLine().trim());
                            break;
                        case 3:
                            // Change booking
                            operation = OP_CHANGE;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter day (0=Monday, 1=Tuesday, ...): ");
                            day = Byte.parseByte(scanner.nextLine().trim());
                            System.out.print("Enter current start time: ");
                            startTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter current end time: ");
                            endTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter booking ID: ");
                            bookingId = Integer.parseInt(scanner.nextLine().trim());
                            System.out.print("Enter offset (in minutes, can be negative): ");
                            extraField = Integer.parseInt(scanner.nextLine().trim());
                            includeExtra = true;
                            break;
                        case 4:
                            // Monitor facility availability
                            operation = OP_MONITOR;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter day (0=Monday, 1=Tuesday, ...): ");
                            day = Byte.parseByte(scanner.nextLine().trim());
                            System.out.print("Enter start time: ");
                            startTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter end time: ");
                            endTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter monitor interval (in seconds): ");
                            extraField = Integer.parseInt(scanner.nextLine().trim());
                            includeExtra = true;
                            break;
                        case 5:
                            // Extend booking (non-idempotent)
                            operation = OP_EXTEND;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter day (0=Monday, 1=Tuesday, ...): ");
                            day = Byte.parseByte(scanner.nextLine().trim());
                            System.out.print("Enter current start time: ");
                            startTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter current end time: ");
                            endTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter booking ID: ");
                            bookingId = Integer.parseInt(scanner.nextLine().trim());
                            System.out.print("Enter extend minutes (e.g., 30 or 60): ");
                            extraField = Integer.parseInt(scanner.nextLine().trim());
                            includeExtra = true;
                            break;
                        case 6:
                            // Cancel booking (idempotent)
                            operation = OP_CANCEL;
                            System.out.print("Enter facility name: ");
                            facilityName = scanner.nextLine().trim();
                            System.out.print("Enter day (0=Monday, 1=Tuesday, ...): ");
                            day = Byte.parseByte(scanner.nextLine().trim());
                            System.out.print("Enter start time: ");
                            startTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter end time: ");
                            endTime = Short.parseShort(scanner.nextLine().trim());
                            System.out.print("Enter booking ID: ");
                            bookingId = Integer.parseInt(scanner.nextLine().trim());
                            // For cancel, the extra field holds the booking ID (4 bytes)
                            extraField = bookingId;
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
                    
                    // Receive the initial response
                    byte[] responseBuffer = new byte[1024];
                    DatagramPacket receivePacket = new DatagramPacket(responseBuffer, responseBuffer.length);
                    socket.receive(receivePacket);
                    
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