#include "camera.h"
#include <algorithm>
#include <cassert>
#include <cstring>


uint32_t* Camera::addr_to_reg(uint64_t addr) {
    uint32_t* reg = nullptr;

    switch (addr) {
        case CAPTURE_REG_CONTROL: reg = &control; break;
        case CAPTURE_REG_WIDTH:   reg = &width;   break;
        case CAPTURE_REG_HEIGHT:  reg = &height;  break;
        case CAPTURE_REG_STATUS:  reg = &status;  break;
        case CAPTURE_REG_INTERVAL: reg = &interval; break;
        case CAPTURE_REG_JPEG_SIZE: reg = &jpeg_size; break; 
        default: SC_REPORT_ERROR("Camera", "Invalid register address");
    }
    return reg; 
}

void Camera::capture_thread() {
    while(true) {
        // If control bit 0 is NOT set, wait for the event (triggered by write to control)
        if (!(control & 0x1)) {
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
    auto addr = trans.get_address();
    auto cmd = trans.get_command();
    auto len = trans.get_data_length();
    auto ptr = trans.get_data_ptr();

    if (addr < CAM_FRAME_BUFFER_SIZE) {
        assert(cmd == tlm::TLM_READ_COMMAND);
        assert((addr + len) <= CAM_FRAME_BUFFER_SIZE);
            
        memcpy(ptr, &framebuffer[addr], len);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        return;
    } 
    if (len != 4) {
        SC_REPORT_ERROR("Camera", "Register access must be 32-bit");
        return;
    }
    auto it = addr_to_reg(addr);

    if ((cmd == tlm::TLM_WRITE_COMMAND)) {
        uint32_t value = *((uint32_t * ) ptr);
        
        if (addr == CAPTURE_REG_WIDTH) {
            if (value > CAM_MAX_WIDTH) {
                SC_REPORT_WARNING("Camera", "Invalid width ignored");
                return; 
            }
        }else if (addr == CAPTURE_REG_HEIGHT) {
            if (value > CAM_MAX_HEIGHT) {
                SC_REPORT_WARNING("Camera", "Invalid height ignored");
                return; 
            }
        }else if (addr == CAPTURE_REG_CONTROL) {
            if (value & ~0x1) { // only bit 0 allowed
                SC_REPORT_WARNING("Camera", "Invalid CONTROL bits ignored");
                return;
            }
        }  
    }

    if (cmd == tlm::TLM_READ_COMMAND) {
        *((uint32_t *) ptr) = *it;
    } else if (cmd == tlm::TLM_WRITE_COMMAND) {
        *it = *((uint32_t *)ptr);
    } else {

    }

    if (cmd == tlm::TLM_WRITE_COMMAND) {
        *it = *((uint32_t *)ptr); // Update the register value first

        if (addr == CAPTURE_REG_CONTROL) {
            if (*it & 0x1) { // If bit 0 (Start) was just written
                capture_event.notify(sc_core::SC_ZERO_TIME);
        }
    }
    
    // Set response status (Crucial: fixes the assert error from earlier!)
    trans.set_response_status(tlm::TLM_OK_RESPONSE);

    }
}
