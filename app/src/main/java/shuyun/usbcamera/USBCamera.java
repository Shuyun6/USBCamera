package shuyun.usbcamera;

/**
 * Created by Shuyun on 2017/10/19 0019.
 */

public class USBCamera{

    public static final int IMG_WIDTH = 640;
    public static final int IMG_HEIGHT = 480;

    static{
        System.loadLibrary("USBCamera-lib");
    }

    public USBCamera(){
    }

    /**
     * Open camera via USB port
     * @return 0 open success, others failed
     */
    public native int openCamera();

    /**
     * release usb camera
     * @return 0 release success, others failed
     */
    public native int releaseCamera();

    /**
     * prepare for previewing
     * @param width commend using {@link #IMG_WIDTH}
     * @param height commend using {@link #IMG_HEIGHT}
     * @param number number of frame
     * @return 0 success, others failed
     */
    public native int prepare(int width, int height, int number);

    /**
     * Turn on frame buffer stream
     * @return 0 success, others failed
     */
    public native int streamOn();

    /**
     * Turn off frame buffer stream
     * @return 0 success, others failed
     */
    public native int streamOff();

    /**
     * get frame data from frame buffer
     * @param data YUYV422 format data
     * @return index of frame buffer
     */
    public native int frameBuffer(byte[] data);

    /**
     *Transform YUYV422 format data to RGB format data
     * @param yuv YUYV422 format data
     * @param rgb RGB format data
     * @param width
     * @param height
     * @return 0 success, others failed
     */
    public native int yuv2rgb(byte[] yuv, byte[] rgb, int width, int height);

    /**
     * Transform YUYV422 format data to YUV420 format data
     * @param yuv422 YUYV422 format data
     * @param yuv420 YUV420 format data
     * @param width
     * @param height
     * @return 0 success, others failed
     */
    public native int yuv422To420(byte[] yuv422, byte[] yuv420, int width, int height);

    /**
     * Transform YUYV422 format data to YUV420p format data
     * @param yuv422 YUYV422 format data
     * @param yuv420p YUV420p format data
     * @param width
     * @param height
     * @return 0 success, others failed
     */
    public native int yuv422To420p(byte[] yuv422, byte[] yuv420p, int width, int height);

    /**
     * Transform YUYV422 format data to NV21 format data
     * @param yuv422 YUYV422 format data
     * @param nv21 NV21 format data
     * @param width
     * @param height
     * @return 0 success, others failed
     */
    public native int yuv2nv21(byte[] yuv422, byte[] nv21, int width, int height);

    @Deprecated
    public native int mirror(byte[] nv21 ,int destWidth , int destHeight , int SrcWidth , int SrcHeight );

    /**
     * Point to the next frame head in queue buffer
     * @param index index in queue buffer, get from {@link #frameBuffer(byte[])}
     * @return 0 success, others failed
     */
    public native int queueBuffer(int index);

    public native int videoPrepare(byte[] filename);

    public native int videoStart(byte[] yuvData);

    public native int videoClose();

}
