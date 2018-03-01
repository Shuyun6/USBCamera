//
// Created by Shuyun on 2018/1/24 0024.
//

#include <jni.h>
#include <android/native_window_jni.h>
#include <malloc.h>
#include "UVCCamera.h"

extern "C"
JNIEXPORT jlong JNICALL
/**
 * create an UVCCamera instance
 * @param env
 * @param instance
 * @return long type camera ID
 */
Java_com_shuyun_pupilla_Pupilla_create(JNIEnv *env, jobject instance) {

    // TODO
    UVCCamera *camera = new UVCCamera();
    jlong cameraId = reinterpret_cast<jlong>(camera);
    return cameraId;

}

extern "C"
JNIEXPORT void JNICALL
Java_com_shuyun_pupilla_Pupilla_destroy(JNIEnv *env, jobject instance, jlong cameraId) {

    // TODO
    UVCCamera *camera = reinterpret_cast<UVCCamera *>(cameraId);
    if(camera){
        delete (camera);
        camera = NULL;
    }
    return;
}
JNIEXPORT jint JNICALL
Java_com_shuyun_pupilla_Pupilla_setPreviewDisplay(JNIEnv *env, jobject instance, jlong cameraId,
                                                  jobject surface) {

    // TODO
    jint result = JNI_ERR;
    UVCCamera *camera = reinterpret_cast<UVCCamera *>(cameraId);
    if(camera){
        ANativeWindow *window = surface ? ANativeWindow_fromSurface(env, surface) : NULL;
        result = camera->setPreviewDisplay(window);
    }
    return result;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_shuyun_pupilla_Pupilla_stopPreview(JNIEnv *env, jobject instance, jlong cameraId) {

    // TODO
    jint result = JNI_ERR;
    UVCCamera *camera = reinterpret_cast<UVCCamera*>(cameraId);
    if (camera) {
        result = camera->stopPreview();
    }
    return result;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_shuyun_pupilla_Pupilla_startPreview(JNIEnv *env, jobject instance, jlong cameraId) {

    // TODO
    UVCCamera *camera = reinterpret_cast<UVCCamera *>(cameraId);
    if (camera) {
        return camera->startPreview();
    }
    return (jint)JNI_ERR;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_shuyun_pupilla_Pupilla_getSupportSize(JNIEnv *env, jobject instance, jlong cameraId) {

    // TODO
    jstring result = NULL;
    UVCCamera *camera = reinterpret_cast<UVCCamera*>(cameraId);
    if (camera) {
        char* sz = camera->getSupportedSize();
        if (sz) {
            result = env->NewStringUTF(sz);
            free(sz);
        }
    }
    return result;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_shuyun_pupilla_Pupilla_setPreviewSize(JNIEnv *env, jobject instance, jlong cameraId,
                                               jint width, jint height, jint fps_min, jint fps_max,
                                               jint mode, jint bandWidth) {

    // TODO
    UVCCamera *camera = reinterpret_cast<UVCCamera *>(cameraId);
    if (camera) {
        return camera->setPreviewSize(width, height, fps_min, fps_max, mode, bandWidth);
    }
    return JNI_ERR;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_shuyun_pupilla_Pupilla_connect(JNIEnv *env, jobject instance, jlong cameraId,
                                        jint vendorId, jint productId, jint fileDescriptor,
                                        jint busNumber, jint deviceAddress, jstring usbFS_) {
    // TODO
    int result = JNI_ERR;
    UVCCamera *camera = reinterpret_cast<UVCCamera *>(cameraId);
    char *usbFS = (char *) env->GetStringUTFChars(usbFS_, JNI_FALSE);
    if(camera && fileDescriptor > 0){
        result = camera->connect(vendorId, productId, fileDescriptor, busNumber, deviceAddress, usbFS);
    }

    env->ReleaseStringUTFChars(usbFS_, usbFS);
    return result;
}