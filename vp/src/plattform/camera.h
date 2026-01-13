#include <cstdlib>
#include <cstring>

#include <systemc>
#include <tlm_utils/simple_target_socket.h>

#define CAM_MAX_WIDTH		1920
#define CAM_MAX_HEIGHT		1080
#define CAM_FRAME_BUFFER_SIZE	(CAM_MAX_WIDTH*CAM_MAX_HEIGHT)


SC_MODULE(Camera) {
    // bus interface
    uint32_t read(uint32_t addr);
    sc_core::sc_out<bool> irq;

    void write(uint32_t addr, uint32_t value);

    
    SC_CTOR(Camera);
};