# SC4051-distributed-booking-system

## Folder Structure

- **/SC4051-distributed-booking-system/**
  - `client/` → Client-side code
  - `server/` → Server-side code
  - `docs/` → Documentation and reports
  - `tests/` → Test cases
  - `scripts/` → Utility scripts
  - `README.md`
  - `.gitignore`

## Build Instructions for Server

1. **Prerequisites:**

   - CMake 3.30+
   - Boost 1.87
   - MinGW
   - Clang-Format (optional for code formatting)

2. **Build the Server:**
   - Open terminal and navigate to the `scripts/` folder:
     ```
     cd scripts
     ```
   - Run the build script for windows:
     ```
     # run either command
     build_and_run_windows.bat test #for running test script
     build_and_run_windows.bat test #for running the server
     ```

## Troubleshooting

- Ensure `BOOST_PATH` is correctly set in `scripts/CMakeLists.txt`.
