#include <stdint.h>
#include "irq.h"

// Define the register addresses
#define CAM_REG_CONTROL   ((volatile uint32_t*)0xFF0000)
#define CAM_REG_STATUS    ((volatile uint32_t*)0xFF000C)
#define CAM_REG_JPEG_SIZE ((volatile uint32_t*)0xFF0014)

// This flag is updated by the Interrupt Handler
extern volatile int frame_ready;

#include "irq.h"
#include <stdint.h>

// 1. Define your camera registers
#define CAM_REG_CONTROL   ((volatile uint32_t*)0xFF0000)

// 2. This is the ID of your camera in the simulator (usually 1, 2, or 3)
#define CAMERA_IRQ_ID 1 

volatile int frame_ready = 0;

// 3. The actual handler function
void my_camera_handler() {
    frame_ready = 1;
}

int main() {
    // 4. This replaces manually flipping bits.
    // It registers the function AND enables the IRQ in the PLIC for you.
    register_interrupt_handler(CAMERA_IRQ_ID, my_camera_handler);

    // 5. Start the camera
    *CAM_REG_CONTROL = 0x1;

    while(1) {
        if (frame_ready) {
            // ... process image ...
            frame_ready = 0;
            *CAM_REG_CONTROL = 0x1; // Trigger next frame
        }
    }
    return 0;
}