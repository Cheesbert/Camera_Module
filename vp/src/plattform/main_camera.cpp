

#include <systemc>
#include <iostream>

int sc_main(int argc, char* argv[])
{
    std::cout << "Camera test started" << std::endl;
    sc_core::sc_start();
    return 0;
}
