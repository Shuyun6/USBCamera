//
// Created by Shuyun on 2017/10/19 0019.
//
#include <jni.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>
#include <linux/fb.h>
#include <jni.h>
#include <syslog.h>
#include <android/log.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

static int id = -1;
static int buffer_number = 1;
static int mwidth, mheight;
#define LOG_TAG "USBCamera_JNI"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
struct fimc_buffer{
    unsigned char *start;
    size_t length;
};
struct fimc_buffer *buffers = NULL;
struct v4l2_buffer v4l2_buf;

JNIEXPORT jint JNICALL
/**
 * open device by Android native interface
 * @param env
 * @param type
 * @return
 */
Java_shuyun_usbcamera_USBCamera_openCamera(JNIEnv *env, jclass type) {
    // TODO
    char device_name[16];
    int pid = -1;
    struct v4l2_capability cap;
    for(int i = 0; i <10; i++){
        LOGW("device start %d \n", i);
        sprintf(device_name, "/dev/video%d", i);
        //device_name: "/dev/video" + i
        pid = open(device_name, O_RDWR, i);
        LOGW("device %d res %d\n", i, pid);
        if(pid > 0){
            int res = ioctl(pid, VIDIOC_QUERYCAP, &cap);
            LOGW("ioctl res %d\n", res);
            if(res >= 0) {
                if((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
                    break;
            }
        }
    }
    if(pid > 0){
        id = pid;
    }
    return pid;
}

JNIEXPORT jint JNICALL
               Java_shuyun_usbcamera_USBCamera_releaseCamera(JNIEnv *env, jclass type) {
    // TODO
    enum v4l2_buf_type type1 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int res;
    res = ioctl(id, VIDIOC_STREAMOFF, &type1);
    LOGW("ioctl failed\n");
    if(res < 0){
        return res;
    }
    for(int i = 0; i < buffer_number; i++){
        res = munmap(buffers[i].start, buffers[i].length);
        if (res < 0) {
            LOGW("munmap failed\n");
            return res;
        }
    }
    free (buffers);
    close(id);
    return 0;
}

JNIEXPORT jint JNICALL
/**
 * prepare a buffer for data from device
 * @param env
 * @param type
 * @param width
 * @param height
 * @param number buffer array index count
 * @return
 */
Java_shuyun_usbcamera_USBCamera_prepare(JNIEnv *env, jclass type, jint width, jint height,
                                        jint number) {
    // TODO
    int res;
    buffer_number = number;
    mwidth = width;
    mheight = height;
    struct v4l2_format fmt;
    struct v4l2_capability cap;
    res = ioctl(id, VIDIOC_QUERYCAP, &cap);
    LOGW("ioctl query cap %d", res);
    if(res < 0) {
//        return -1;
    }
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGW("V4L2_CAP_VIDEO_CAPTURE");
        return -1;
    }
    //set 0
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    if(ioctl(id, VIDIOC_S_FMT, &fmt) < 0) {
        return -1;
    }
    struct v4l2_requestbuffers req;
    req.count = number;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    res = ioctl(id, VIDIOC_REQBUFS, &req);
    LOGW("ioctl reqbufs %d",res);
    if(res < 0) {
        return -1;
    }
    buffers = calloc(req.count, sizeof(*buffers));
    LOGW("calloc %d",res);
    if(!buffers) {
        return  -1;
    }
    for(int i = 0; i < buffer_number; ++i){
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        v4l2_buf.index = i;
        res = ioctl(id , VIDIOC_QUERYBUF, &v4l2_buf);
        LOGW("ioctl %d %d", i, res);
        if(res < 0) {
            return -1;
        }
        buffers[i].length = v4l2_buf.length;
        if((buffers[i].start = (char *)mmap(0, v4l2_buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, id, v4l2_buf.m.offset)) < 0){
            LOGW("buffers -1");
            return -1;
        }
    }
    return 0;
}

JNIEXPORT jint JNICALL
/**
 * open a stream for buffer from devcie
 * @param env
 * @param type
 * @return
 */
Java_shuyun_usbcamera_USBCamera_streamOn(JNIEnv *env, jclass type) {
    // TODO
    int res;
    enum v4l2_buf_type type1 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for(int i = 0; i< buffer_number; ++i) {
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        v4l2_buf.index = i;
        res = ioctl(id, VIDIOC_QBUF, &v4l2_buf);
        if (res < 0) {
            return res;
        }
    }
    res = ioctl(id, VIDIOC_STREAMON, &type1);
    if (res < 0) {
        LOGW("%d : VIDIOC_STREAMON failed\n",__LINE__);
        return res;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_streamOff(JNIEnv *env, jclass type) {
    // TODO
    enum v4l2_buf_type type1 = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int res;
    res = ioctl(id, VIDIOC_STREAMOFF, &type1);
    if (res < 0) {
        LOGW("%s : VIDIOC_STREAMOFF failed\n",__func__);
        return res;
    }
    return 0;
}

JNIEXPORT jint JNICALL
/**
 * copy data from v412_buf to data_
 * @param env
 * @param type
 * @param data_
 * @return
 */
Java_shuyun_usbcamera_USBCamera_frameBuffer(JNIEnv *env, jclass type, jbyteArray data_) {
    jbyte *data = (*env)->GetByteArrayElements(env, data_, NULL);
    // TODO
    int res;
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    res = ioctl(id, VIDIOC_DQBUF, &v4l2_buf);
    if (res < 0) {
        LOGW("%s : VIDIOC_DQBUF failed, dropped frame\n",__func__);
        return res;
    }
    memcpy(data, buffers[v4l2_buf.index].start, buffers[v4l2_buf.index].length);
    (*env)->ReleaseByteArrayElements(env, data_, data, 0);
    return v4l2_buf.index;
}

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_yuv2rgb(JNIEnv *env, jclass type, jbyteArray yuv_, jbyteArray rgb_,
                                        jint width, jint height) {
    jbyte *yuv = (*env)->GetByteArrayElements(env, yuv_, NULL);
    jbyte *rgb = (*env)->GetByteArrayElements(env, rgb_, NULL);
    // TODO
    AVFrame * rpicture=NULL;
    AVFrame * ypicture=NULL;
    struct SwsContext *swsctx = NULL;
    rpicture = av_frame_alloc();
    ypicture = av_frame_alloc();
    avpicture_fill((AVPicture *) rpicture, (uint8_t *)rgb, AV_PIX_FMT_RGB565, width, height);
    avpicture_fill((AVPicture *) ypicture, (uint8_t *)yuv, AV_PIX_FMT_YUYV422,mwidth,mheight);
    swsctx = sws_getContext(mwidth,mheight, AV_PIX_FMT_YUYV422,	width, height, AV_PIX_FMT_RGB565, SWS_BICUBIC, NULL, NULL, NULL);
    sws_scale(swsctx,(const uint8_t* const*)ypicture->data,ypicture->linesize,0,mheight,rpicture->data,rpicture->linesize);
    sws_freeContext(swsctx);
    av_free(rpicture);
    av_free(ypicture);
    (*env)->ReleaseByteArrayElements(env, yuv_, yuv, 0);
    (*env)->ReleaseByteArrayElements(env, rgb_, rgb, 0);
    return 0;
}

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_yuv422To420(JNIEnv *env, jclass type, jbyteArray yuv422_,
                                            jbyteArray yuv420_, jint width, jint height) {
    jbyte *yuv422 = (*env)->GetByteArrayElements(env, yuv422_, NULL);
    jbyte *yuv420 = (*env)->GetByteArrayElements(env, yuv420_, NULL);
    // TODO
    int ynum=width*height;
    int i,j,k=0;
    for(i=0;i<ynum;i++){
        yuv420[i]=yuv422[i*2];
    }
    for(i=0;i<height;i++){
        if((i%2)!=0)continue;
        for(j=0;j<(width/2);j++){
            if((4*j+1)>(2*width))break;
            yuv420[ynum+k*2*width/4+j]=yuv422[i*2*width+4*j+1];
        }
        k++;
    }
    k=0;
    for(i=0;i<height;i++){
        if((i%2)==0)continue;
        for(j=0;j<(width/2);j++){
            if((4*j+3)>(2*width))break;
            yuv420[ynum+ynum/4+k*2*width/4+j]=yuv422[i*2*width+4*j+3];
        }
        k++;
    }
    (*env)->ReleaseByteArrayElements(env, yuv422_, yuv422, 0);
    (*env)->ReleaseByteArrayElements(env, yuv420_, yuv420, 0);
    return 0;
}

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_yuv422To420p(JNIEnv *env, jclass type, jbyteArray yuv422_,
                                             jbyteArray yuv420p_, jint width, jint height) {
    jbyte *yuv422 = (*env)->GetByteArrayElements(env, yuv422_, NULL);
    jbyte *yuv420p = (*env)->GetByteArrayElements(env, yuv420p_, NULL);
    // TODO
    AVFrame * rpicture=NULL;
    AVFrame * ypicture=NULL;
    struct SwsContext *swsctx = NULL;
    rpicture=av_frame_alloc();
    ypicture=av_frame_alloc();
    avpicture_fill((AVPicture *) rpicture, (uint8_t *)yuv420p, AV_PIX_FMT_YUV420P, width, height);
    avpicture_fill((AVPicture *) ypicture, (uint8_t *)yuv422, AV_PIX_FMT_YUYV422, mwidth, mheight);
    swsctx = sws_getContext(mwidth, mheight, AV_PIX_FMT_YUYV422,	width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    sws_scale(swsctx,(const uint8_t* const*)ypicture->data,ypicture->linesize,0,mheight,rpicture->data,rpicture->linesize);
    sws_freeContext(swsctx);
    av_free(rpicture);
    av_free(ypicture);
    (*env)->ReleaseByteArrayElements(env, yuv422_, yuv422, 0);
    (*env)->ReleaseByteArrayElements(env, yuv420p_, yuv420p, 0);
    return 0;
}

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_yuv2nv21(JNIEnv *env, jclass type, jbyteArray yuv422_,
                                          jbyteArray nv21_, jint width, jint height) {
    jbyte *yuv422 = (*env)->GetByteArrayElements(env, yuv422_, NULL);
    jbyte *nv21 = (*env)->GetByteArrayElements(env, nv21_, NULL);
    // TODO
    AVFrame * rpicture=NULL;
    AVFrame * ypicture=NULL;
    struct SwsContext *swsctx = NULL;
    rpicture=av_frame_alloc();
    ypicture=av_frame_alloc();
    avpicture_fill((AVPicture *) rpicture, (uint8_t *)nv21, AV_PIX_FMT_NV21, width, height);
    avpicture_fill((AVPicture *) ypicture, (uint8_t *)yuv422, AV_PIX_FMT_YUYV422, mwidth, mheight);
    swsctx = sws_getContext(mwidth, mheight, AV_PIX_FMT_YUYV422, width, height, AV_PIX_FMT_NV21, SWS_BICUBIC, NULL, NULL, NULL);
    sws_scale(swsctx,(const uint8_t* const*)ypicture->data, ypicture->linesize, 0, mheight, rpicture->data, rpicture->linesize);
    sws_freeContext(swsctx);
    av_free(rpicture);
    av_free(ypicture);
    (*env)->ReleaseByteArrayElements(env, yuv422_, yuv422, 0);
    (*env)->ReleaseByteArrayElements(env, nv21_, nv21, 0);
    return 0;
}

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_mirror(JNIEnv *env, jclass type, jbyteArray nv21_, jint destWidth,
                                       jint destHeight, jint SrcWidth, jint SrcHeight) {
    jbyte *nv21 = (*env)->GetByteArrayElements(env, nv21_, NULL);
    // TODO
    int deleteW = (destWidth - SrcHeight) / 2;
    int deleteH = (destHeight - SrcWidth) / 2;
    int i, j;
    int a, b;
    uint8_t temp;
    //mirror y
    for (i = 0; i < SrcHeight; i++){
        a= i * SrcWidth;
        b= (i + 1) * SrcWidth - 1;
        while (a < b) {
            temp = nv21[a];
            nv21[a]= nv21[b];
            nv21[b]= temp;
            a++;
            b--;
        }
    }
    //mirror u
    int uindex = SrcWidth * SrcHeight;
    for (i = 0; i < SrcHeight / 2;i++) {
        a = i * SrcWidth / 2;
        b= (i + 1) * SrcWidth / 2 - 1;
        while (a < b) {
            temp= nv21[a + uindex];
            nv21[a+ uindex] = nv21[b + uindex];
            nv21[b+ uindex] = temp;
            a++;
            b--;
        }
    }
    //mirror v
    uindex= SrcWidth * SrcHeight / 4 * 5;
    for (i = 0; i < SrcHeight / 2;i++) {
        a= i * SrcWidth / 2;
        b= (i + 1) * SrcWidth / 2 - 1;
        while (a < b) {
            temp= nv21[a + uindex];
            nv21[a+ uindex] = nv21[b + uindex];
            nv21[b+ uindex] = temp;
            a++;
            b--;
        }
    }
    (*env)->ReleaseByteArrayElements(env, nv21_, nv21, 0);
    return 0;
}

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_queueBuffer(JNIEnv *env, jclass type, jint index) {
    // TODO
    int res;
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;
    res = ioctl(id, VIDIOC_QBUF, &v4l2_buf);
    if (res < 0) {
        LOGW("%s : VIDIOC_QBUF failed\n",__func__);
        return res;
    }
    return 0;
}

AVCodecContext *pCodecCtx= NULL;
AVPacket avpkt;
FILE * video_file;
unsigned char *outbuf=NULL;
unsigned char *yuv420buf=NULL;
static int outsize=0;

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_videoPrepare(JNIEnv *env, jclass type, jbyteArray filename_) {
    jbyte *filename = (*env)->GetByteArrayElements(env, filename_, NULL);
    // TODO
    AVCodec * pCodec=NULL;
    avcodec_register_all();
    pCodec=avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);
    if(pCodec == NULL) {
        LOGW("++++++++++++codec not found\n");
        return -1;
    }
    pCodecCtx=avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL) {
        LOGW("++++++Could not allocate video codec context\n");
        return -1;
    }
    /* put sample parameters */
    pCodecCtx->bit_rate = 400000;
    /* resolution must be a multiple of two */
    pCodecCtx->width = mwidth;
    pCodecCtx->height = mheight;
    /* frames per second */
    pCodecCtx->time_base= (AVRational){1,25};
    pCodecCtx->gop_size = 10; /* emit one intra frame every ten frames */
    pCodecCtx->max_b_frames=1;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;//AV_PIX_FMT_YUYV422;
    /* open it */
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGW("+++++++Could not open codec\n");
        return -1;
    }
    outsize = mwidth * mheight*2;
    outbuf = malloc(outsize*sizeof(char));
    yuv420buf = malloc(outsize*sizeof(char));
    jbyte *filedir = (jbyte*)(*env)->GetByteArrayElements(env, filename, 0);
    if ((video_file = fopen(filedir, "wb")) == NULL) {
        LOGW("++++++++++++open %s failed\n",filedir);
        return -1;
    }
    (*env)->ReleaseByteArrayElements(env, filename_, filename, 0);
    return 0;
}

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_videoStart(JNIEnv *env, jclass type, jbyteArray yuvData_) {
    jbyte *yuvData = (*env)->GetByteArrayElements(env, yuvData_, NULL);
    // TODO
    int frameFinished=0,size=0;
    jbyte *ydata = (jbyte*)(*env)->GetByteArrayElements(env, yuvData, 0);
    AVFrame * yuv420pframe=NULL;
    AVFrame * yuv422frame=NULL;
    struct SwsContext *swsctx = NULL;
    yuv420pframe=av_frame_alloc();
    yuv422frame=av_frame_alloc();
    avpicture_fill((AVPicture *) yuv420pframe, (uint8_t *)yuv420buf, AV_PIX_FMT_YUV420P,mwidth,mheight);
    avpicture_fill((AVPicture *) yuv422frame, (uint8_t *)ydata, AV_PIX_FMT_YUYV422,mwidth,mheight);
    swsctx = sws_getContext(mwidth,mheight, AV_PIX_FMT_YUYV422,	mwidth, mheight,AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    sws_scale(swsctx,(const uint8_t* const*)yuv422frame->data,yuv422frame->linesize,0,mheight,yuv420pframe->data,yuv420pframe->linesize);
    av_init_packet(&avpkt);
    size = avcodec_encode_video2(pCodecCtx, &avpkt, yuv420pframe, &frameFinished);
    if (size < 0) {
        LOGW("+++++Error encoding frame\n");
        return -1;
    }
    if(frameFinished)
        fwrite(avpkt.data,avpkt.size,1,video_file);
    av_free_packet(&avpkt);
    sws_freeContext(swsctx);
    av_free(yuv420pframe);
    av_free(yuv422frame);
    (*env)->ReleaseByteArrayElements(env, yuvData_, yuvData, 0);
    return 0;
}

JNIEXPORT jint JNICALL
Java_shuyun_usbcamera_USBCamera_videoClose(JNIEnv *env, jclass type) {
    fclose(video_file);
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    free(outbuf);
    // TODO
    return 0;
}