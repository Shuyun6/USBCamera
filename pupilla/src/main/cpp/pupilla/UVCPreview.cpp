//
// Created by Shuyun on 2018/1/31 0031.
//

#include <pthread.h>
#include <libuvc/libuvc.h>
#include <stdlib.h>
#include "UVCPreview.h"
#include "utilbase.h"

#define MAX_FRAME 4
#define PREVIEW_PIXEL_BYTES 4
#define FRAME_POOL_SZ MAX_FRAME + 2

UVCPreview::UVCPreview(uvc_device_handle_t *devh) {
    mPreviewWindow = NULL;
    mCaptureWindow = NULL;
    mDeviceHandle = devh;
    requestWidth = DEFAULT_PREVIEW_WIDTH;
    requestHeight = DEFAULT_PREVIEW_HEIGHT;
    requestMinFps = DEFAULT_PREVIEW_FPS_MIN;
    requestMaxFps = DEFAULT_PREVIEW_FPS_MAX;
    requestMode = DEFAULT_PREVIEW_MODE;
    requestBandwidth = DEFAULT_BANDWIDTH;
    frameWidth = DEFAULT_PREVIEW_WIDTH;
    frameHeight = DEFAULT_PREVIEW_HEIGHT;
    frameBytes = DEFAULT_PREVIEW_WIDTH * DEFAULT_PREVIEW_HEIGHT * 2;
    frameMode = 0;
    previewBytes = DEFAULT_PREVIEW_WIDTH * DEFAULT_PREVIEW_HEIGHT * PREVIEW_PIXEL_BYTES;
    previewFormat = WINDOW_FORMAT_RGBA_8888;
    mIsCapturing = false;
    mIsRunning = false;
    captureQueue = NULL;
    mFrameCallbackFunc = NULL;
    mFrameCallbackObj = NULL;
    callbackPixelBytes = 2;

    pthread_cond_init(&preview_sync, NULL);
    pthread_mutex_init(&preview_mutex, NULL);
    //
    pthread_cond_init(&capture_sync, NULL);
    pthread_mutex_init(&capture_mutex, NULL);
    //
    pthread_mutex_init(&pool_mutex, NULL);


}

UVCPreview::~UVCPreview() {
    if (mPreviewWindow) {
        ANativeWindow_release(mPreviewWindow);
    }
    mPreviewWindow = NULL;
    if (mCaptureWindow) {
        ANativeWindow_release(mCaptureWindow);
    }
    mCaptureWindow = NULL;
    clearPreviewFrame();
    clearCaptureFrame();
    clear_pool();

    pthread_mutex_destroy(&preview_mutex);
    pthread_cond_destroy(&preview_sync);
    pthread_mutex_destroy(&capture_mutex);
    pthread_cond_destroy(&capture_sync);
    pthread_mutex_destroy(&pool_mutex);
}

/**
 * 在预览帧池中，并初始化内存空间
 * @param data_bytes
 */
void UVCPreview::init_pool(size_t data_bytes) {
    clear_pool();
    pthread_mutex_lock(&pool_mutex);
    {
        for (int i = 0; i < FRAME_POOL_SZ; i++) {
            mFramePool.put(uvc_allocate_frame(data_bytes));
        }
    }
    pthread_mutex_unlock(&pool_mutex);
}

inline const bool UVCPreview::isRunning() const {
    return mIsRunning;
}

inline const bool UVCPreview::isCapturing() const {
    return mIsCapturing;
}

int UVCPreview::setPreviewSize(int width, int height, int min_fps, int max_fps, int mode, float bandwidth) {
    int result = 0;
    if ((requestWidth != width) || (requestHeight != height) || (requestMode != mode)) {
        requestWidth = width;
        requestHeight = height;
        requestMinFps = min_fps;
        requestMaxFps = max_fps;
        requestMode = mode;
        requestBandwidth = bandwidth;

        uvc_stream_ctrl_t ctrl;
        result = uvc_get_stream_ctrl_format_size_fps(mDeviceHandle, &ctrl, UVC_FRAME_FORMAT_YUYV,
            requestWidth, requestHeight, requestMinFps, requestMaxFps);
    }
    return result;
}

int UVCPreview::setPreviewDisplay(ANativeWindow *preview_window) {
    //set the pointer parameter to global pointer mPreviewWindow
    pthread_mutex_lock(&preview_mutex);
    {
        if (mPreviewWindow != preview_window) {
            if (mPreviewWindow)
                ANativeWindow_release(mPreviewWindow);
            mPreviewWindow = preview_window;
            if ((mPreviewWindow)) {
                ANativeWindow_setBuffersGeometry(mPreviewWindow,
                    frameWidth, frameHeight, previewFormat);
            }
        }
    }
    pthread_mutex_unlock(&preview_mutex);
    return 0;
}

int UVCPreview::setFrameCallback(JNIEnv *env, jobject frame_callback_obj, int pixel_format) {
    pthread_mutex_lock(&capture_mutex);
    {
        if (isRunning() && isCapturing()) {
            mIsCapturing = false;
            if (mFrameCallbackObj) {
                pthread_cond_signal(&capture_sync);
                // waiting for finishing capturing
                pthread_cond_wait(&capture_sync, &capture_mutex);
            }
        }
        if (!env->IsSameObject(mFrameCallbackObj, frame_callback_obj))	{
            iframecallback_fields.onFrame = NULL;
            if (mFrameCallbackObj) {
                env->DeleteGlobalRef(mFrameCallbackObj);
            }
            mFrameCallbackObj = frame_callback_obj;
            if (frame_callback_obj) {
                jclass clazz = env->GetObjectClass(frame_callback_obj);
                if ((clazz)) {
                    iframecallback_fields.onFrame = env->GetMethodID(clazz, "onFrame",	"(Ljava/nio/ByteBuffer;)V");
                } else {
                    LOGW("failed to get object class");
                }
                env->ExceptionClear();
                if (!iframecallback_fields.onFrame) {
                    LOGE("Can't find IFrameCallback#onFrame");
                    env->DeleteGlobalRef(frame_callback_obj);
                    mFrameCallbackObj = frame_callback_obj = NULL;
                }
            }
        }
        if (frame_callback_obj) {
            mPixelFormat = pixel_format;
            callbackPixelFormatChanged();
        }
    }
    pthread_mutex_unlock(&capture_mutex);
    return 0;
}

/**
 * 根据mPixelFormal计算callbackPixelBytes长度
 */
void UVCPreview::callbackPixelFormatChanged() {
    mFrameCallbackFunc = NULL;
    const size_t sz = requestWidth * requestHeight;
    switch (mPixelFormat) {
        case PIXEL_FORMAT_RAW:
            LOGI("PIXEL_FORMAT_RAW:");
            callbackPixelBytes = sz * 2;
            break;
        case PIXEL_FORMAT_YUV:
            LOGI("PIXEL_FORMAT_YUV:");
            callbackPixelBytes = sz * 2;
            break;
        case PIXEL_FORMAT_RGB565:
            LOGI("PIXEL_FORMAT_RGB565:");
            mFrameCallbackFunc = uvc_any2rgb565;
            callbackPixelBytes = sz * 2;
            break;
        case PIXEL_FORMAT_RGBX:
            LOGI("PIXEL_FORMAT_RGBX:");
            mFrameCallbackFunc = uvc_any2rgbx;
            callbackPixelBytes = sz * 4;
            break;
        case PIXEL_FORMAT_YUV20SP:
            LOGI("PIXEL_FORMAT_YUV20SP:");
            mFrameCallbackFunc = uvc_yuyv2iyuv420SP;
            callbackPixelBytes = (sz * 3) / 2;
            break;
        case PIXEL_FORMAT_NV21:
            LOGI("PIXEL_FORMAT_NV21:");
            mFrameCallbackFunc = uvc_yuyv2yuv420SP;
            callbackPixelBytes = (sz * 3) / 2;
            break;
    }
}

void UVCPreview::clearDisplay() {
    ANativeWindow_Buffer buffer;
    pthread_mutex_lock(&capture_mutex);
    {
        if ((mCaptureWindow)) {
            if ((ANativeWindow_lock(mCaptureWindow, &buffer, NULL) == 0)) {
                uint8_t *dest = (uint8_t *)buffer.bits;
                const size_t bytes = buffer.width * PREVIEW_PIXEL_BYTES;
                const int stride = buffer.stride * PREVIEW_PIXEL_BYTES;
                for (int i = 0; i < buffer.height; i++) {
                    memset(dest, 0, bytes);
                    dest += stride;
                }
                ANativeWindow_unlockAndPost(mCaptureWindow);
            }
        }
    }
    pthread_mutex_unlock(&capture_mutex);
    pthread_mutex_lock(&preview_mutex);
    {
        if ((mPreviewWindow)) {
            if ((ANativeWindow_lock(mPreviewWindow, &buffer, NULL) == 0)) {
                uint8_t *dest = (uint8_t *)buffer.bits;
                const size_t bytes = buffer.width * PREVIEW_PIXEL_BYTES;
                const int stride = buffer.stride * PREVIEW_PIXEL_BYTES;
                for (int i = 0; i < buffer.height; i++) {
                    memset(dest, 0, bytes);
                    dest += stride;
                }
                ANativeWindow_unlockAndPost(mPreviewWindow);
            }
        }
    }
    pthread_mutex_unlock(&preview_mutex);
}

/**
 * 调用预览线程preview_thread_func
 * @return
 */
int UVCPreview::startPreview() {
    int result = EXIT_FAILURE;
    if (!isRunning()) {
        mIsRunning = true;
        pthread_mutex_lock(&preview_mutex);
        {
            if (mPreviewWindow) {
                result = pthread_create(&preview_thread, NULL, preview_thread_func, (void *)this);
            }
        }
        pthread_mutex_unlock(&preview_mutex);
        if (result != EXIT_SUCCESS) {
            LOGW("UVCCamera::window does not exist/already running/could not create thread etc.");
            mIsRunning = false;
            pthread_mutex_lock(&preview_mutex);
            {
                pthread_cond_signal(&preview_sync);
            }
            pthread_mutex_unlock(&preview_mutex);
        }
    }
    return result;
}

int UVCPreview::stopPreview() {
    bool b = isRunning();
    if (b) {
        mIsRunning = false;
        pthread_cond_signal(&preview_sync);
        pthread_cond_signal(&capture_sync);
        if (pthread_join(capture_thread, NULL) != EXIT_SUCCESS) {
            LOGW("UVCPreview::terminate capture thread: pthread_join failed");
        }
        if (pthread_join(preview_thread, NULL) != EXIT_SUCCESS) {
            LOGW("UVCPreview::terminate preview thread: pthread_join failed");
        }
        clearDisplay();
    }
    clearPreviewFrame();
    clearCaptureFrame();
    pthread_mutex_lock(&preview_mutex);
    if (mPreviewWindow) {
        ANativeWindow_release(mPreviewWindow);
        mPreviewWindow = NULL;
    }
    pthread_mutex_unlock(&preview_mutex);
    pthread_mutex_lock(&capture_mutex);
    if (mCaptureWindow) {
        ANativeWindow_release(mCaptureWindow);
        mCaptureWindow = NULL;
    }
    pthread_mutex_unlock(&capture_mutex);
    return 0;
}

/**
 * start preview thread
 * @param args
 * @return
 */
void *UVCPreview::preview_thread_func(void *args) {
    int result;
    UVCPreview *preview = reinterpret_cast<UVCPreview *>(args);
    if (preview) {
        uvc_stream_ctrl_t ctrl;
        result = preview->prepare_preview(&ctrl);
        if (!result) {
            preview->do_preview(&ctrl);
        }
    }
    pthread_exit(NULL);
}

/**
 * Get latest frame from frame pool or get an empty frame
 * @param data_bytes
 * @return
 */
uvc_frame_t *UVCPreview::get_frame(size_t data_bytes) {
    uvc_frame_t *frame = NULL;
    pthread_mutex_lock(&pool_mutex);
    {
        if (!mFramePool.isEmpty()) {
            frame = mFramePool.last();
        }
    }
    pthread_mutex_unlock(&pool_mutex);
    if (!frame) {
        frame = uvc_allocate_frame(data_bytes);
    }
    return frame;
}

void UVCPreview::clearPreviewFrame() {
    pthread_mutex_lock(&preview_mutex);
    {
        for (int i = 0; i < previewFrames.size(); i++)
            recycle_frame(previewFrames[i]);
        previewFrames.clear();
    }
    pthread_mutex_unlock(&preview_mutex);
}

/**
 * 将captureQueue放入帧池
 */
void UVCPreview::clearCaptureFrame() {
    pthread_mutex_lock(&capture_mutex);
    {
        if (captureQueue)
            recycle_frame(captureQueue);
        captureQueue = NULL;
    }
    pthread_mutex_unlock(&capture_mutex);
}

/**
 * 清空帧池
 */
void UVCPreview::clear_pool() {
    pthread_mutex_lock(&pool_mutex);
    {
        const int n = mFramePool.size();
        for (int i = 0; i < n; i++) {
            uvc_free_frame(mFramePool[i]);
        }
        mFramePool.clear();
    }
    pthread_mutex_unlock(&pool_mutex);
}

/**
 * 处理当前帧。如果帧池有位置则填入帧池，否则丢弃
 * @param frame
 */
void UVCPreview::recycle_frame(uvc_frame_t *frame) {
    pthread_mutex_lock(&pool_mutex);
    if ((mFramePool.size() < FRAME_POOL_SZ)) {
        mFramePool.put(frame);
        frame = NULL;
    }
    pthread_mutex_unlock(&pool_mutex);
    if ((frame)) {
        uvc_free_frame(frame);
    }
}

/**
 * 从摄像头中获取一帧，并放到帧数组中
 * @param frame
 * @param args
 */
void UVCPreview::uvc_preview_frame_callback(uvc_frame_t *frame, void *args) {
    //预览中，不断低
    UVCPreview *preview = reinterpret_cast<UVCPreview *>(args);
    if (!preview->isRunning() || !frame || !frame->frame_format || !frame->data || !frame->data_bytes){
        return;
    }
    if (((frame->frame_format != UVC_FRAME_FORMAT_MJPEG) && (frame->actual_bytes < preview->frameBytes))
            || (frame->width != preview->frameWidth) || (frame->height != preview->frameHeight) ) {
        return;
    }
    if (preview->isRunning()) {
        //获取一个等长的空帧
        uvc_frame_t *copy = preview->get_frame(frame->data_bytes);
        if (!copy) {
            return;
        }
        //复制帧
        //SUCCESS = 0
        uvc_error_t ret = uvc_duplicate_frame(frame, copy);
        if (ret) {
            preview->recycle_frame(copy);
            return;
        }
        preview->addPreviewFrame(copy);
    }
}

/**
 * 将帧放入帧数组中
 * @param frame
 */
void UVCPreview::addPreviewFrame(uvc_frame_t *frame) {
    pthread_mutex_lock(&preview_mutex);
    if (isRunning() && (previewFrames.size() < MAX_FRAME)) {
        previewFrames.put(frame);
        frame = NULL;
        pthread_cond_signal(&preview_sync);
    }
    pthread_mutex_unlock(&preview_mutex);
    if (frame) {
        recycle_frame(frame);
    }
}

/**
 * 等待同步信号，然后从帧数组中移出一帧
 * @return
 */
uvc_frame_t *UVCPreview::waitPreviewFrame() {
    uvc_frame_t *frame = NULL;
    pthread_mutex_lock(&preview_mutex);
    {
        if (!previewFrames.size()) {
            pthread_cond_wait(&preview_sync, &preview_mutex);
        }
        if (LIKELY(isRunning() && previewFrames.size() > 0)) {
            frame = previewFrames.remove(0);
        }
    }
    pthread_mutex_unlock(&preview_mutex);
    return frame;
}

/**
 * prepare for preview, mainly set the size of window
 * @param ctrl
 * @return
 */
int UVCPreview::prepare_preview(uvc_stream_ctrl_t *ctrl) {
    uvc_error_t result;
    int temp = requestMode;
    result = uvc_get_stream_ctrl_format_size_fps(mDeviceHandle, ctrl, UVC_FRAME_FORMAT_YUYV,
                                                 requestWidth, requestHeight, requestMinFps, requestMaxFps
    );
    if (!result) {
        uvc_frame_desc_t *frame_desc;
        //get a frame descriptor
        result = uvc_get_frame_desc(mDeviceHandle, ctrl, &frame_desc);
        if (!result) {
            frameWidth = frame_desc->wWidth;
            frameHeight = frame_desc->wHeight;
//            LOGE("frameSize=(%d,%d)@%s", frameWidth, frameHeight, "YUYV");
//            int re = requestMode;
//            if(re){
//            }
            pthread_mutex_lock(&preview_mutex);
            if (mPreviewWindow) {
                ANativeWindow_setBuffersGeometry(mPreviewWindow,
                                                 frameWidth, frameHeight, previewFormat);
            }
            pthread_mutex_unlock(&preview_mutex);
        } else {
            frameWidth = requestWidth;
            frameHeight = requestHeight;
        }
        frameMode = requestMode;
        frameBytes = frameWidth * frameHeight * (!requestMode ? 2 : 4);
        previewBytes = frameWidth * frameHeight * PREVIEW_PIXEL_BYTES;
    } else {
        LOGE("could not negotiate with camera:err=%d", result);
    }
    return result;
}

/**
 *设置uvc出流属性，然后调用捕获线程，并一直循环获取帧，然后绘制
 * @param ctrl
 */
void UVCPreview::do_preview(uvc_stream_ctrl_t *ctrl) {
    uvc_frame_t *frame = NULL;
    //get stream in here, UVC_SUCCESS = 0
    uvc_error_t result = uvc_start_streaming_bandwidth(
            mDeviceHandle, ctrl, uvc_preview_frame_callback, (void *)this, requestBandwidth, 0);
    if (!result) {
        //success
        clearPreviewFrame();
        //在这里开始预览了
        pthread_create(&capture_thread, NULL, capture_thread_func, (void *)this);
        for (; isRunning();) {
            frame = waitPreviewFrame();
            if (frame) {
                frame = draw_preview_one(frame, &mPreviewWindow, uvc_any2rgbx, 4);
                addCaptureFrame(frame);
            }
        }
        pthread_cond_signal(&capture_sync);
        uvc_stop_streaming(mDeviceHandle);
    } else {
        uvc_perror(result, "failed start_streaming");
    }
}

/**
 * Copy frame data to surface buffer for display
 * @param src
 * @param dest
 * @param width
 * @param height
 * @param stride_src
 * @param stride_dest
 */
static void copyFrame(const uint8_t *src, uint8_t *dest, const int width, int height, const int stride_src, const int stride_dest) {
    const int h8 = height % 8;
    for (int i = 0; i < h8; i++) {
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
    }
    for (int i = 0; i < height; i += 8) {
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
        memcpy(dest, src, width);
        dest += stride_dest; src += stride_src;
    }
}

/**
 * preview frame from *frame to **window's buffer
 * @param frame
 * @param window
 * @return
 */
int copyToSurface(uvc_frame_t *frame, ANativeWindow **window) {
    int result = 0;
    if (LIKELY(*window)) {
        ANativeWindow_Buffer buffer;
        if (LIKELY(ANativeWindow_lock(*window, &buffer, NULL) == 0)) {
            //buffer is the output parameter
            // source = frame data
            const uint8_t *src = (uint8_t *)frame->data;
            const int src_w = frame->width * PREVIEW_PIXEL_BYTES;
            const int src_step = frame->width * PREVIEW_PIXEL_BYTES;
            // destination = Surface(ANativeWindow)
            uint8_t *dest = (uint8_t *)buffer.bits;
            const int dest_w = buffer.width * PREVIEW_PIXEL_BYTES;
            const int dest_step = buffer.stride * PREVIEW_PIXEL_BYTES;
            // use lower transfer bytes
            const int w = src_w < dest_w ? src_w : dest_w;
            // use lower height
            const int h = frame->height < buffer.height ? frame->height : buffer.height;
            // transfer from frame data to the Surface
            copyFrame(src, dest, w, h, src_step, dest_step);
            ANativeWindow_unlockAndPost(*window);
        } else {
            result = -1;
        }
    } else {
        result = -1;
    }
    return result;
}

/**
 * 调用copyToSurface将帧绘制到surface
 * @param frame
 * @param window
 * @param convert_func
 * @param pixcelBytes
 * @return
 */
uvc_frame_t *UVCPreview::draw_preview_one(uvc_frame_t *frame, ANativeWindow **window, convFunc_t convert_func, int pixcelBytes) {
    int b = 0;
    pthread_mutex_lock(&preview_mutex);
    {
        b = *window != NULL;
    }
    pthread_mutex_unlock(&preview_mutex);
    if (LIKELY(b)) {
        uvc_frame_t *converted;
        if (convert_func) {
            //初始化内存空间
            converted = get_frame(frame->width * frame->height * pixcelBytes);
            if LIKELY(converted) {
                b = convert_func(frame, converted);
                if (!b) {
                    pthread_mutex_lock(&preview_mutex);
                    //draw to buffer
                    copyToSurface(converted, window);
                    pthread_mutex_unlock(&preview_mutex);
                } else {
                    LOGE("failed converting");
                }
                recycle_frame(converted);
            }
        } else {
            pthread_mutex_lock(&preview_mutex);
            copyToSurface(frame, window);
            pthread_mutex_unlock(&preview_mutex);
        }
    }
    return frame; //RETURN(frame, uvc_frame_t *);
}

int UVCPreview::setCaptureDisplay(ANativeWindow *capture_window) {
    pthread_mutex_lock(&capture_mutex);
    {
        if (isRunning() && isCapturing()) {
            mIsCapturing = false;
            if (mCaptureWindow) {
                pthread_cond_signal(&capture_sync);
                pthread_cond_wait(&capture_sync, &capture_mutex);	// wait finishing capturing
            }
        }
        if (mCaptureWindow != capture_window) {
            // release current Surface if already assigned.
            if (mCaptureWindow){
                ANativeWindow_release(mCaptureWindow);
            }
            mCaptureWindow = capture_window;
            // if you use Surface came from MediaCodec#createInputSurface
            // you could not change window format at least when you use
            // ANativeWindow_lock / ANativeWindow_unlockAndPost
            // to write frame data to the Surface...
            // So we need check here.
            if (mCaptureWindow) {
                int32_t window_format = ANativeWindow_getFormat(mCaptureWindow);
                if ((window_format != WINDOW_FORMAT_RGB_565)
                    && (previewFormat == WINDOW_FORMAT_RGB_565)) {
                    LOGE("window format mismatch, cancelled movie capturing.");
                    ANativeWindow_release(mCaptureWindow);
                    mCaptureWindow = NULL;
                }
            }
        }
    }
    pthread_mutex_unlock(&capture_mutex);
    return 0;
}

/*
 * 更新captureQueue的值，并唤醒capture_sync锁
 */
void UVCPreview::addCaptureFrame(uvc_frame_t *frame) {
    pthread_mutex_lock(&capture_mutex);
    if (LIKELY(isRunning())) {
        if (captureQueue) {
            recycle_frame(captureQueue);
        }
        captureQueue = frame;
        pthread_cond_broadcast(&capture_sync);
    }
    pthread_mutex_unlock(&capture_mutex);
}

/**
 * 同步获取captureQueue的值
 * @return
 */
uvc_frame_t *UVCPreview::waitCaptureFrame() {
    uvc_frame_t *frame = NULL;
    pthread_mutex_lock(&capture_mutex);
    {
        if (!captureQueue) {
            pthread_cond_wait(&capture_sync, &capture_mutex);
        }
        if (LIKELY(isRunning() && captureQueue)) {
            frame = captureQueue;
            captureQueue = NULL;
        }
    }
    pthread_mutex_unlock(&capture_mutex);
    return frame;
}

/**
 * 帧捕获线程
 * @param args
 * @return
 */
void *UVCPreview::capture_thread_func(void *args) {
    int result;
    UVCPreview *preview = reinterpret_cast<UVCPreview *>(args);
    if (LIKELY(preview)) {
        JavaVM *vm = getVM();
        JNIEnv *env;
        // attach to JavaVM
        vm->AttachCurrentThread(&env, NULL);
        preview->do_capture(env);	// never return until finish previewing
        // detach from JavaVM
        vm->DetachCurrentThread();
        MARK("DetachCurrentThread");
    }
    pthread_exit(NULL);
}

/**
 * 不停循环捕获
 * @param env
 */
void UVCPreview::do_capture(JNIEnv *env) {
    clearCaptureFrame();
    callbackPixelFormatChanged();
    for (; isRunning() ;) {
        mIsCapturing = true;
        if (mCaptureWindow) {
            do_capture_surface(env);
        } else {
            do_capture_idle_loop(env);
        }
        pthread_cond_broadcast(&capture_sync);
    }
}

void UVCPreview::do_capture_idle_loop(JNIEnv *env) {
    for (; isRunning() && isCapturing() ;) {
        do_capture_callback(env, waitCaptureFrame());
    }
}

/**
 * 获取捕获帧，转换格式并输出到窗口缓冲区，回调到java层
 * @param env
 */
void UVCPreview::do_capture_surface(JNIEnv *env) {
    uvc_frame_t *frame = NULL;
    uvc_frame_t *converted = NULL;
    char *local_picture_path;
    for (; isRunning() && isCapturing() ;) {
        //获取captureQueue对应的帧
        frame = waitCaptureFrame();
        if (frame) {
            // frame data is always YUYV format.
            if (isCapturing()) {
                if (!converted) {
                    //previewBytes = width * height * 4 (yuyv)
                    converted = get_frame(previewBytes);
                }
                if (converted) {
                    //SUCCESS = 0
                    int b = uvc_any2rgbx(frame, converted);
                    if (!b) {
                        if (mCaptureWindow) {
                            copyToSurface(converted, &mCaptureWindow);
                        }
                    }
                }
            }
            do_capture_callback(env, frame);
        }
    }
    if (converted) {
        recycle_frame(converted);
    }
    if (mCaptureWindow) {
        ANativeWindow_release(mCaptureWindow);
        mCaptureWindow = NULL;
    }
}

/**
 * 将*frame数据回调到java层
 * @param env
 * @param frame
 */
void UVCPreview::do_capture_callback(JNIEnv *env, uvc_frame_t *frame) {
    if (frame) {
        uvc_frame_t *callback_frame = frame;
        if (mFrameCallbackObj) {
            if (mFrameCallbackFunc) {
                callback_frame = get_frame(callbackPixelBytes);
                if (callback_frame) {
                    int b = mFrameCallbackFunc(frame, callback_frame);
                    recycle_frame(frame);
                    //success = 0
                    if (b) {
                        goto SKIP;
                    }
                } else {
                    callback_frame = frame;
                    goto SKIP;
                }
            }
            jobject buf = env->NewDirectByteBuffer(callback_frame->data, callbackPixelBytes);
            env->CallVoidMethod(mFrameCallbackObj, iframecallback_fields.onFrame, buf);
            env->ExceptionClear();
            env->DeleteLocalRef(buf);
        }
        SKIP:
        recycle_frame(callback_frame);
    }
}
