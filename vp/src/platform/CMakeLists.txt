add_executable(tb_camera tb_camera.cpp camera.cpp)
target_link_libraries(tb_camera 
    PRIVATE 
        SystemC::systemc 
        v4l2 
        jpeg 
        Threads::Threads
)


