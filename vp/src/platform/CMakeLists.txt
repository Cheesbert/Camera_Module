add_executable(test_camera test_camera.cpp camera.cpp)

target_link_libraries(test_camera 
    PRIVATE 
        SystemC::systemc 
        v4l2 
        jpeg 
        Threads::Threads
)

