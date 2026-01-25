
#include "capture.h"
#include <cstdio>
#include <cstdlib>

int main()
{
    Capture capture;
    size_t buf_size = capture.get_buffer_size();
    unsigned char* image = new unsigned char[buf_size];
    size_t jpeg_size = capture.grab(image, buf_size);

    FILE* f = fopen("test_capture.jpg", "wb");
    fwrite(image, 1, jpeg_size, f);
    fclose(f);

    delete[] image;
    return 0;
}