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
    int fd_camera; // File descriptor for /dev/video0
    void* buffer;
    size_t length;

    int width;
    int height;
    uint32_t pixelformat;

public:
    Capture() {
        fd_camera = open("/dev/video0", O_RDWR); // devide file for 
        if (fd_camera < 0) {
            perror("open /dev/video0 failed");
            return;
        }
        
        struct v4l2_format fmt = {0};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        
        if (ioctl(fd_camera, VIDIOC_S_FMT, &fmt) < 0) {
            perror("ioctl(fd_camera, VIDIOC_S_FMT, &fmt);\n");
        }

        width = fmt.fmt.pix.width;
        height = fmt.fmt.pix.height; 
        pixelformat = fmt.fmt.pix.pixelformat; 
    
        struct v4l2_requestbuffers req = {0};
        req.count = 1; req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; req.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd_camera, VIDIOC_REQBUFS, &req) < 0) {
            perror("ioctl(fd_camera, VIDIOC_REQBUFS, &req);\n");
        }

        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; buf.memory = V4L2_MEMORY_MMAP; buf.index = 0;
        if (ioctl(fd_camera, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("ioctl(fd_camera, VIDIOC_QUERYBUF, &buf);\n");
        }

        length = buf.length;
        buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_camera, buf.m.offset);
        
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd_camera, VIDIOC_STREAMON, &type) < 0) {
            perror("VIDIOC_STREAMON");
        }
        struct v4l2_buffer qbuf = {0};
        qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.index = 0;

        if (ioctl(fd_camera, VIDIOC_QBUF, &qbuf) < 0) { 
            perror("ioctl(fd_camera, VIDIOC_QBUF)");
        }
    }

    size_t grab(unsigned char* out_ptr, size_t max_size) {
        struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        ioctl(fd_camera, VIDIOC_DQBUF, &buf);

        size_t copy = buf.bytesused;
        if (copy > max_size) copy = max_size;

        memcpy(out_ptr, buffer, copy);
        ioctl(fd_camera, VIDIOC_QBUF, &buf);

        return copy;
    }
    void decode_mjpeg() {
        int c = 1;
        return;
    }
    
    int get_width() const { return width; }
    int get_height() const { return height; }
    uint32_t get_pixelformat() const { return pixelformat; }
    size_t get_buffer_size() const { return length; }
};
#endif

