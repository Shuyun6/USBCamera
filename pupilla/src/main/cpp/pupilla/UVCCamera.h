//
// Created by Shuyun on 2018/1/31 0031.
//


#ifndef USBCAMERA_UVCCAMERA_H
#define USBCAMERA_UVCCAMERA_H

#include "../libuvc/include/libuvc/libuvc.h"
#include "UVCPreview.h"
#include <stdint.h>
#include <android/native_window.h>
#include <jni.h>

typedef struct control_value {
    int res;
    int min;
    int max;
    int def;
    int current;
} control_value_t;

class UVCCamera {

    char *usbFS;
    uvc_context_t *context;
    int fd;
    uvc_device_t *device;
    uvc_device_handle_t *deviceHanlde;

    UVCPreview *preview;
    uint64_t ctrlSupports;
    uint64_t PUSupports;

public:
    UVCCamera();
    ~UVCCamera();

    int connect(int vid, int pid, int fd, int bus, int addr, char *usbfs);
    int release();
    char *getSupportedSize();
    int setPreviewSize(int width, int height, int fps_min, int fps_max, int mode, float bandWidth);
    int setPreviewDisplay(ANativeWindow *preview_window);
    int setOnFrameCallback(JNIEnv *env, jobject callback, int format);
    int startPreview();
    int stopPreview();
    int getCtrlSupports(uint64_t *supports);
    int getProcSupports(uint64_t *supports);
    void clearParameters();
};


#endif //USBCAMERA_UVCCAMERA_H
