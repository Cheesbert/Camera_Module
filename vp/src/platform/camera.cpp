#include "camera.h"
#include <algorithm>
#include <cassert>
#include <cstring>


uint32_t* Camera::addr_to_reg(uint64_t addr) {
    switch (addr) {
        case CAPTURE_REG_CONTROL: return &control; 
        case CAPTURE_REG_WIDTH:   return &width;  
        case CAPTURE_REG_HEIGHT:  return &height;  
        case CAPTURE_REG_STATUS:  return &status;   
        case CAPTURE_REG_INTERVAL: return &interval; 
        case CAPTURE_REG_JPEG_SIZE: return &jpeg_size; 
        default: return nullptr; 
    }
 
}
void Camera::capture_thread() {
    while(true) {
        if (!(control & 0x1)) { // wait for event
            wait(capture_event);
        }

        if (cam_hardware) {
            jpeg_size  = cam_hardware->grab(framebuffer.data(), framebuffer.size());

            wait(10, sc_core::SC_MS); 
            status |= 0x1; 

            irq.write(true);
            wait(1, sc_core::SC_US); // Pulse width
            irq.write(false);
        }

        if (interval > 0) {
            wait(sc_core::sc_time(interval, sc_core::SC_US));
        } else {
            wait(33, sc_core::SC_MS); // ~30 FPS
        }
    }
}

void Camera::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
    auto addr  = trans.get_address();
    auto cmd = trans.get_command();
    auto len = trans.get_data_length();
    auto* ptr = trans.get_data_ptr();

    std::cout << "TLM access addr=0x" << std::hex << addr << std::dec << std::endl;

    if (addr < CAM_FRAME_BUFFER_SIZE) { // framebuffer access
        if (cmd != tlm::TLM_READ_COMMAND || addr + len > CAM_FRAME_BUFFER_SIZE) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        memcpy(ptr, &framebuffer[addr], len);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return;
    }

    if (len != 4) { // Register
        trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
        return;
    }

    uint32_t* reg = addr_to_reg(addr);
    if (!reg) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }

    if (cmd == tlm::TLM_READ_COMMAND) { // register to buffer
        std::memcpy(ptr, reg, 4);
    }
    else if (cmd == tlm::TLM_WRITE_COMMAND) { // buffer to register
        std::memcpy(reg, ptr, 4);

        if (addr == CAPTURE_REG_CONTROL && (*reg & 0x1)) {
            capture_event.notify(sc_core::SC_ZERO_TIME);
        }
    }

    trans.set_response_status(tlm::TLM_OK_RESPONSE);

}
