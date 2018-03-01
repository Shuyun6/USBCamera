//
// Created by Shuyun on 2018/1/31 0031.
//

#include <malloc.h>
#include <string.h>
#include "UVCCamera.h"

UVCCamera::UVCCamera() {
    fd = 0;
    usbFS = NULL;
    context = NULL;
    device = NULL;
    deviceHanlde = NULL;
    preview = NULL;
    ctrlSupports = 0;
    PUSupports = 0;

}

UVCCamera::~UVCCamera() {
    release();
    if (context) {
//        uvc_exit(context);
        context = NULL;
    }
    if (usbFS) {
        free(usbFS);
        usbFS = NULL;
    }
}

void UVCCamera::clearParameters() {
    ctrlSupports = 0;
    PUSupports = 0;
}

int UVCCamera::connect(int vid, int pid, int fd, int bus, int addr, char *usbfs) {

    uvc_error_t result = UVC_ERROR_BUSY;
    if (!deviceHanlde && fd) {
        if (usbfs) {
            free(usbfs);
        }
        usbFS = strdup(usbfs);
        if (!context) {
//            result = uvc_init(&context, NULL);//result ld error ??
//            uvc_exit(context);
        }
    }
    return 1;

}

int UVCCamera::release() {

}

int UVCCamera::setPreviewSize(int width, int height, int fps_min, int fps_max, int mode,
                              float bandWidth) {

}

char* UVCCamera::getSupportedSize() {

}

int UVCCamera::setPreviewDisplay(ANativeWindow *preview_window) {

}

int UVCCamera::setOnFrameCallback(JNIEnv *env, jobject callback, int format) {

}

int UVCCamera::startPreview() {

}

int UVCCamera::stopPreview() {

}

int UVCCamera::getCtrlSupports(uint64_t *supports) {

}

int UVCCamera::getProcSupports(uint64_t *supports) {

}
