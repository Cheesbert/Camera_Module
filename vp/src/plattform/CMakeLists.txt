# Add the .cpp file so CMake has something to compile!
add_library(platform STATIC
    main_camera.cpp
)

# This tells the compiler where to find camera.h and capture.h
target_include_directories(platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Important: This links the platform library to the executable we defined in the root
target_link_libraries(platform PUBLIC SystemC::systemc v4l2)