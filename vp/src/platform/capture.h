#ifndef CAPTURE_H
#define CAPTURE_H

#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstdint> 
#include <unistd.h>
#include <vector>
#include <cstring>
#include <cstdio>
#include <jpeglib.h>

class Capture { // camera logic 
    int fd_camera = -1; // file descriptor for /dev/video0
    void* buffer = nullptr;
    size_t length = 0;

    int width;
    int height;
    uint32_t pixelformat;

public:
    Capture() {
        startCapture();
    }

    ~Capture() {
        stopCapture();
    }
    
    void startCapture() {
        fd_camera = open("/dev/video0", O_RDWR); // linux dir file descripor file 
        if (fd_camera < 0) {
            perror("open /dev/video0 failed");
            stopCapture();
            return;
        }
        
        struct v4l2_format format = {0}; // struct for storing camera properties
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // video capture
        if (ioctl(fd_camera, VIDIOC_S_FMT, &format) < 0) { // apply video settings and negotiate format
            perror("Failed to apply video settings VIDIOC_S_FMT");
            stopCapture();
            return;
        }

        width = format.fmt.pix.width;
        height = format.fmt.pix.height; 
        pixelformat = format.fmt.pix.pixelformat; 
    
        
        struct v4l2_requestbuffers req = {0};
        req.count = 1; // one buffer
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
        req.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd_camera, VIDIOC_REQBUFS, &req) < 0) {
            perror("Failed to allocate buffer memory VIDIOC_REQBUFS ");
            stopCapture();
            return;
        }

        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = 0;
        if (ioctl(fd_camera, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("Failed to get allocated buffer data VIDIOC_QUERYBUF ");
            stopCapture();
            return;
        }

        length = buf.length;
        buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_camera, buf.m.offset);
        if (buffer == MAP_FAILED) {
            perror("Failed mmap allcoating video buffer ");
            stopCapture();
            return;
        }
        
        struct v4l2_buffer qbuf = {0};
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.index = 0;
        if (ioctl(fd_camera, VIDIOC_QBUF, &qbuf) < 0) { 
            perror("Failed to queue buffer VIDIOC_QBUF ");
            stopCapture();
            return;
        }

        if (ioctl(fd_camera, VIDIOC_STREAMON, &format.type) < 0) {
            perror("Failed to start capturing process VIDIOC_STREAMON ");
            stopCapture();
            return;
        }
    }
    void stopCapture() {
        if (fd_camera < 0) {
            return; 
        }
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd_camera, VIDIOC_STREAMOFF, &type) < 0) {
            perror("Coudnt stop capturing VIDIOC_STREAMOFF");
        }

        if (buffer && buffer != MAP_FAILED) {
            munmap(buffer, length);
            buffer = NULL;
        }

        close(fd_camera);
        fd_camera = -1;
    }

    size_t grab(unsigned char* out_ptr, size_t max_size) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        ioctl(fd_camera, VIDIOC_DQBUF, &buf); // todo error handling

        size_t copy = buf.bytesused;
        if (copy > max_size) copy = max_size;

        memcpy(out_ptr, buffer, copy);
        ioctl(fd_camera, VIDIOC_QBUF, &buf);

        return copy;
    }
    
    int get_width() const { return width; }
    int get_height() const { return height; }
    uint32_t get_pixelformat() const { return pixelformat; }
    size_t get_buffer_size() const { return length; }
};
#endif

