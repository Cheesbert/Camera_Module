cmake_minimum_required(VERSION 3.16)
project(vp)

# Find SystemC
find_package(SystemCLanguage REQUIRED)

# We define the executable here, but we link the logic from the subdirectory
# Note: We don't list any .cpp files here because they are in the 'platform' library
add_executable(vp)

# Tell CMake to go into the subdirectory
add_subdirectory(src/plattform)

# Link everything together
target_link_libraries(vp
    PRIVATE
    platform          # Your camera logic
    SystemC::systemc  # The SystemC engine
    v4l2              # The Linux camera library
)