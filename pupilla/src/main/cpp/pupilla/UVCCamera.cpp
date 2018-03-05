//
// Created by Shuyun on 2018/1/31 0031.
//

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "UVCCamera.h"

UVCCamera::UVCCamera() {
    mFd = 0;
    mUsbFS = NULL;
    mContext = NULL;
    mDevice = NULL;
    mDeviceHanlde = NULL;
    mPreview = NULL;
    mCtrlSupports = 0;
    mPUSupports = 0;

}

UVCCamera::~UVCCamera() {
    release();
    if (mContext) {
//        uvc_exit(context);
        mContext = NULL;
    }
    if (mUsbFS) {
        free(mUsbFS);
        mUsbFS = NULL;
    }
}

void UVCCamera::clearParameters() {
    mCtrlSupports = 0;
    mPUSupports = 0;
}

int UVCCamera::connect(int vid, int pid, int fd, int bus, int addr, char *usbfs) {

    uvc_error_t result = UVC_ERROR_BUSY;
    if (!mDeviceHanlde && fd) {
        if (mUsbFS) {
            free(mUsbFS);
        }
        mUsbFS = strdup(usbfs);
        if (!mContext) {
            result = uvc_init2(&mContext, NULL, mUsbFS);
            if (0 > result) {
                return result;
            }
        }
        clearParameters();
        fd = dup(fd);
        result = uvc_get_device_with_fd(mContext, &mDevice, vid, pid, NULL, fd, bus, addr);
        if (!result) {
            result = uvc_open(mDevice, &mDeviceHanlde);
            if (!result) {
                mFd = fd;

            }
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
