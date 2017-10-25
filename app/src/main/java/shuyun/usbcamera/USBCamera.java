package shuyun.usbcamera;

import android.content.Context;

/**
 * Created by Shuyun on 2017/10/19 0019.
 */

public class USBCamera{

    static{
        System.loadLibrary("USBCamera-lib");
    }

    public USBCamera(){

    }

    // Nativa functions

    public native int openCamera();

    public native int releaseCamera();

    public native int prepare(int width, int height, int number);

    public native int streamOn();

    public native int streamOff();

    public native int frameBuffer(byte[] data);

    public native int yuv2rgb(byte[] yuv, byte[] rgb, int width, int height);

    public native int yuv422To420(byte[] yuv422, byte[] yuv420, int width, int height);

    public native int yuv422To420p(byte[] yuv422, byte[] yuv420p, int width, int height);

    public native int yuvTonv21(byte[] yuv422, byte[] nv21, int width, int height);

    public native int mirror(byte[] nv21 ,int destWidth , int destHeight , int SrcWidth , int SrcHeight );

    public native int queueBuffer(int index);

    public native int videoPrepare(byte[] filename);

    public native int videoStart(byte[] yuvData);

    public native int videoClose();

}
