//
// Created by Shuyun on 2018/1/31 0031.
//

#include "objectarray.h"
#include <jni.h>
#include <android/native_window.h>

#define DEFAULT_PREVIEW_WIDTH 640
#define DEFAULT_PREVIEW_HEIGHT 480
#define DEFAULT_PREVIEW_FPS_MIN 1
#define DEFAULT_PREVIEW_FPS_MAX 30
#define DEFAULT_PREVIEW_MODE 0
#define DEFAULT_BANDWIDTH 1.0f

typedef uvc_error_t (*convFunc_t)(uvc_frame_t *in, uvc_frame_t *out);

//#define PIXEL_FORMAT_YUV 1
#define PIXEL_FORMAT_RAW 0		// same as PIXEL_FORMAT_YUV
#define PIXEL_FORMAT_YUV 1
#define PIXEL_FORMAT_RGB565 2
#define PIXEL_FORMAT_RGBX 3
#define PIXEL_FORMAT_YUV20SP 4
#define PIXEL_FORMAT_NV21 5

typedef struct {
    jmethodID onFrame;
} Fields_iframecallback;

class UVCPreview {

private:
    uvc_device_handle_t *mDeviceHandle;
    ANativeWindow *mPreviewWindow;
    volatile bool mIsRunning;
    int requestWidth, requestHeight, requestMode;
    int requestMinFps, requestMaxFps;
    float requestBandwidth;
    int frameWidth, frameHeight;
    int frameMode;
    size_t frameBytes;
    pthread_t preview_thread;
    pthread_mutex_t preview_mutex;
    pthread_cond_t preview_sync;
    ObjectArray<uvc_frame_t *> previewFrames;
    int previewFormat;
    size_t previewBytes;

    volatile bool mIsCapturing;
    ANativeWindow *mCaptureWindow;
    pthread_t capture_thread;
    pthread_mutex_t capture_mutex;
    pthread_cond_t capture_sync;
    uvc_frame_t *captureQueue;
    jobject mFrameCallbackObj;
    convFunc_t mFrameCallbackFunc;
    Fields_iframecallback iframecallback_fields;
    int mPixelFormat;
    size_t callbackPixelBytes;

    pthread_mutex_t pool_mutex;
    ObjectArray<uvc_frame_t *> mFramePool;
    uvc_frame_t *get_frame(size_t data_bytes);
    void recycle_frame(uvc_frame_t *frame);
    void init_pool(size_t data_bytes);
    void clear_pool();

    void clearDisplay();
    static void uvc_preview_frame_callback(uvc_frame_t *frame, void *vptr_args);
    void addPreviewFrame(uvc_frame_t *frame);
    uvc_frame_t *waitPreviewFrame();
    void clearPreviewFrame();
    static void *preview_thread_func(void *vptr_args);
    int prepare_preview(uvc_stream_ctrl_t *ctrl);
    void do_preview(uvc_stream_ctrl_t *ctrl);
    uvc_frame_t *draw_preview_one(uvc_frame_t *frame, ANativeWindow **window, convFunc_t func, int pixelBytes);

    void addCaptureFrame(uvc_frame_t *frame);
    uvc_frame_t *waitCaptureFrame();
    void clearCaptureFrame();
    static void *capture_thread_func(void *vptr_args);
    void do_capture(JNIEnv *env);
    void do_capture_surface(JNIEnv *env);
    void do_capture_idle_loop(JNIEnv *env);
    void do_capture_callback(JNIEnv *env, uvc_frame_t *frame);
    void callbackPixelFormatChanged();

public:
    UVCPreview(uvc_device_handle_t *devh);
    ~UVCPreview();

    inline const bool isRunning() const;
    int setPreviewSize(int width, int height, int min_fps, int max_fps, int mode, float bandwidth = 1.0f);
    int setPreviewDisplay(ANativeWindow *preview_window);
    int setFrameCallback(JNIEnv *env, jobject frame_callback_obj, int pixel_format);
    int startPreview();
    int stopPreview();
    inline const bool isCapturing() const;
    int setCaptureDisplay(ANativeWindow *capture_window);

};