add_executable(tb_camera tb_camera.cpp 
camera.cpp)

add_executable(test_capture test_capture.cpp)

target_link_libraries(tb_camera 
    PRIVATE 
        SystemC::systemc 
        v4l2 
        jpeg 
        Threads::Threads
)

