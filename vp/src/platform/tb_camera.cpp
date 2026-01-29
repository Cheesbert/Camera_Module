#include <systemc>
#include <fstream>
#include <sys/stat.h>
#include <cerrno>

#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include "camera.h"

SC_MODULE(CameraTB) {
    tlm_utils::simple_initiator_socket<CameraTB> isock;
    sc_core::sc_in<bool> irq;

    SC_CTOR(CameraTB) : isock("isock") {
        SC_THREAD(run);
        sensitive << irq;
    }

    template <typename T>
    void write(uint64_t addr, const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Data type must be trivial copyable");
        
        tlm::tlm_generic_payload trans; 
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME; 
        size_t size = sizeof(T)

        std::array<unsigned char, size> buffer{};
        memcpy(buffer.data(), &value, size); // write source value to buffer

        trans.set_command(tlm::TLM_WRITE_COMMAND); // cmd what to do
        trans.set_address(addr);
        trans.set_data_length(size); // number bytes transaction 4bytes 32bits 
        trans.set_streaming_width(size);
        trans.set_data_ptr(buffer.data());
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE); // transaction prepared but not completed 
        
        isock->b_transport(trans, delay);

    } 

    void write32(uint64_t addr, uint32_t value) {
        tlm::tlm_generic_payload trans; 
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME; 

        trans.set_command(tlm::TLM_WRITE_COMMAND); // Writr Command
        trans.set_address(addr); // write to addr
        trans.set_data_length(4); 
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

    void saveFrame(uint32_t jpeg_size, std::string frameName) { 
        std::ofstream ofs(frameName + ".jpg", std::ios::binary);

        if (!ofs) {
            std::cerr << "Error opening file\n";
            return;
        }

        for (uint32_t addr = 0; addr < jpeg_size; addr += 4) {
            uint32_t word = read32(addr);
            ofs.write(reinterpret_cast<char*>(&word),
                    std::min(4u, jpeg_size - addr));
        }

        ofs.close();
            
    }
    
    void run() {
        // New run video capture 
        uint32_t fps = 33333; 
        uint32_t width = 640;
        uint32_t height = 480; 
        write32(Camera::CAPTURE_REG_INTERVAL, fps); 
        write32(Camera::CAPTURE_REG_WIDTH, width); // Todo size handling
        write32(Camera::CAPTURE_REG_HEIGHT, height);

        uint8_t start = 1; 
        uint8_t stop = 0;
        uint32_t jpeg_size = 0; 

        std::cout << "Start camera" << std::endl; 
        write32(Camera::CAPTURE_REG_CONTROL, start);

        uint32_t frames = 10; 
        std::string test_frames_dir = "test_frames"; 

        // Create dir for test frames
        if (mkdir(test_frames_dir.c_str(), 0755) && errno != EEXIST) {
            perror("mkdir");
        }

        std::string frame_name = test_frames_dir + "/test_frame";
        std::string save_name; 

        for (int i = 0; i < frames; i++) {
            wait(irq.posedge_event()); 
            
            std::cout << "Received frame" << i << "at: " << sc_core::sc_time_stamp() << std::endl; 
            save_name = frame_name + std::to_string(i); 
            jpeg_size = read32(Camera::CAPTURE_REG_JPEG_SIZE);
            saveFrame(jpeg_size, save_name); 
            std::cout << "Saved frame: " << frame_name << " at " << sc_core::sc_time_stamp() << std::endl;
        }
        write32(Camera::CAPTURE_REG_CONTROL, stop); 
        std::cout << "Simulation finished." << std::endl;
        sc_core::sc_stop();
    }
};

int sc_main(int argc, char* argv[]) {
    CameraTB tb("tb");
    Camera   cam("cam");

    sc_core::sc_signal<bool> irq_signal;
    tb.isock.bind(cam.tsock);
    tb.irq.bind(irq_signal);
    cam.irq.bind(irq_signal);

    std::cout << "Starting SystemC Simulation..." << std::endl;
    sc_core::sc_start(); 

    return 0;
}