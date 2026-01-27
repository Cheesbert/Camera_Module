#include "capture.h"
#include <cstdlib>
#include <cstring>

#include <systemc>
#include <tlm_utils/simple_target_socket.h>

#define CAM_MAX_WIDTH		1920
#define CAM_MAX_HEIGHT		1080
#define CAM_FRAME_BUFFER_SIZE	(CAM_MAX_WIDTH*CAM_MAX_HEIGHT)

SC_MODULE(Camera) {
    tlm_utils::simple_target_socket<Camera> tsock;
    sc_core::sc_out<bool> irq;
    uint32_t irq_number = 0;

    // Registers
    uint32_t control = 0;
    uint32_t width   = 640;
    uint32_t height  = 480;
    uint32_t status  = 0;
    uint32_t interval = 0; // fps
    uint32_t jpeg_size = 0; 

    std::vector<uint8_t> framebuffer;
    sc_core::sc_event capture_event;
    Capture* cam_hardware = nullptr;

    enum {
        CAPTURE_REG_CONTROL = 0xFF0000,
        CAPTURE_REG_WIDTH   = 0xFF0004,
        CAPTURE_REG_HEIGHT  = 0xFF0008,
        CAPTURE_REG_STATUS  = 0xFF000C,
        CAPTURE_REG_INTERVAL = 0xFF0010,
        CAPTURE_REG_JPEG_SIZE = 0xFF0014
    };

    SC_CTOR(Camera) : tsock("tsock"), irq("irq") {
        tsock.register_b_transport(this, &Camera::transport);
        framebuffer.resize(CAM_FRAME_BUFFER_SIZE);
        cam_hardware = new Capture();
        SC_THREAD(capture_thread);
        
    }

    uint32_t* addr_to_reg(uint64_t addr);
    void capture_thread();
    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay); 
};

