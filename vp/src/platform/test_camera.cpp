#include <systemc>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include "camera.h"

SC_MODULE(CameraTB) {
    tlm_utils::simple_initiator_socket<CameraTB> isock;
    sc_core::sc_in<bool> irq;

    SC_CTOR(CameraTB) : isock("isock") {
        SC_THREAD(run); // Registration
        sensitive << irq;
    }


    void write32(uint64_t addr, uint32_t value) {
        tlm::tlm_generic_payload trans; // Container Data
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME; // Keep track of simulation

        trans.set_command(tlm::TLM_WRITE_COMMAND); // Writr Command
        trans.set_address(addr); // Write to addr
        trans.set_data_length(4); // 
        trans.set_streaming_width(4);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&value)); // Ptr to write value
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE); // Incomcomplete atm 

        isock->b_transport(trans, delay); // Block until write has been done 
        assert(trans.get_response_status() == tlm::TLM_OK_RESPONSE); // Assert write complete
    }

    uint32_t read32(uint64_t addr) {
        uint32_t value = 0;
        tlm::tlm_generic_payload trans;
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;

        trans.set_command(tlm::TLM_READ_COMMAND);
        trans.set_address(addr);
        trans.set_data_length(4);
        trans.set_streaming_width(4);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&value));
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        isock->b_transport(trans, delay);
        assert(trans.get_response_status() == tlm::TLM_OK_RESPONSE);

        return value;
    }

    // --- THIS IS THE IMPLEMENTATION ---
    void run() {
        std::cout << "Testbench started!" << std::endl;

        // 1. Configure the camera (Address 0xFF0010 is Interval)
        // Let's set it to 33ms (approx 30 FPS)
        write32(0xFF0010, 33000); 

        // 2. Start the camera (Address 0xFF0000 is Control)
        write32(0xFF0000, 1);

        for(int i = 0; i < 5; i++) {
            // 3. Wait for the camera to signal an interrupt
            wait(); // Wakes up when irq changes (because of sensitivity)

            if (irq.read() == true) {
                std::cout << "Frame " << i << " received!" << std::endl;

                // 4. Read data from the camera framebuffer
                // Address 0x0 is the start of the frame
                uint32_t pixel = read32(0); 
                
                // 5. Reset status if needed
                write32(0xFF000C, 0); 
            }
        }

        std::cout << "Simulation finished." << std::endl;
        sc_core::sc_stop();
    }
};

int sc_main(int argc, char* argv[]) {
    // 1. Instantiate the modules
    CameraTB tb("tb");
    Camera   cam("cam");

    // 2. Create signals to connect them
    // This acts as the physical wire for the interrupt
    sc_core::sc_signal<bool> irq_signal;

    // 3. Bind (connect) the ports and sockets
    // Connect Initiator to Target
    tb.isock.bind(cam.tsock);
    
    // Connect Interrupt ports to the signal
    tb.irq.bind(irq_signal);
    cam.irq.bind(irq_signal);

    // 4. Start the simulation
    std::cout << "Starting SystemC Simulation..." << std::endl;
    sc_core::sc_start(); 

    return 0;
}