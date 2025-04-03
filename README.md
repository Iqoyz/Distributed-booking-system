# SC4051-distributed-booking-system

## Folder Structure

- **/SC4051-distributed-booking-system/**
  - `client/` → Client-side code
  - `server/` → Server-side code
  - `docs/` → report
  - `tests/` → Test cases
  - `scripts/` → Utility scripts
  - `README.md`
  - `.gitignore`

## Build Instructions for Server (C++)

1. **Prerequisites:**
   - **CMake** ≥ 3.30
   - **Boost 1.87**
   - **Compiler**:
     - Windows: MinGW or MSYS2 (`g++` supporting C++20)
     - macOS/Linux: `clang++` or `g++`
   - **(Optional)**: `clang-format` (for code formatting)


2. **Build the Server:**
   - Open a terminal and navigate to the `scripts/` folder:
     ```
     cd scripts
     ```
   - Run the build script (for Windows) or use CMake commands on other platforms:
     ```
     # For Windows:
     # RUN EITHER COMMAND
     build_and_run_windows.bat test     # to build and run the test harness (server_test)
     build_and_run_windows.bat server   # To build and run the main server
     
     # On macOS/Linux:
     mkdir build && cd build
     cmake ..
     make
     ```
   - The server executable `booking_system_server` will be generated. Run it to start the server:
     ```
     ./booking_system_server
     ```

  - The server executable `server_test` will also be generated. Run it to start the server test:
     ```
     ./server_test
     ```

## Build Instructions for Client

1. **Prerequisites:**
   - Java Development Kit (JDK) installed
   - Optionally, an IDE such as NetBeans, IntelliJ IDEA, or Visual Studio Code with the Java Extension Pack

2. **Compile and Run the Client:**
   - Open a terminal and navigate to the project root directory:
     ```
     cd /path/to/SC4051-distributed-booking-system
     ```
   - Compile the client:
     ```
     javac -d bin client/Client.java
     ```
   - Run the client:
     ```
     java -cp bin client.Client
     ```
   - The client provides a text-based interface for interacting with the server over UDP.

## Troubleshooting

- **Boost Configuration:**
  ### MacOS Users: Boost Setup
  Ensure that Boost is correctly installed. On macOS, if you installed Boost via Homebrew, confirm that the include paths (e.g., `/opt/homebrew/include`) are correctly referenced in your IDE's configuration (e.g., in VSCode's `c_cpp_properties.json`) and in your CMakeLists.txt.

  ### 🧱 Windows Users: Boost Setup
  Download Boost 1.87, extract it to a local folder, and build it. Then, open the `CMakeLists.txt` file located in the `scripts/` folder and replace the placeholder with your actual Boost path:
```cmake
if(WIN32)
    set(BOOST_PATH "C:/Users/YourName/Documents/boost_1_87_0")  # <-- Modify this path
```

- **CMake and Compiler Settings:**  
  - On Windows, ensure that MinGW (or your chosen compiler) is configured correctly.
  - On macOS/Linux, verify that your CMake configuration picks up the correct Boost installation.

- **Java Environment:**  
  - Make sure your JDK is installed and your environment variables (like JAVA_HOME) are set properly.
  - If you experience issues compiling or running the client, check that your `javac` and `java` commands point to the correct JDK version.

- **General:**  
  If you encounter issues with building the server or client, double-check the corresponding build instructions and verify that your system meets all prerequisites.
