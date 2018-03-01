package com.shuyun.pupilla;

import android.view.Surface;

/**
 * Created by Shuyun on 2018/1/10 0010.
 */

public class Pupilla {

    static {
        System.loadLibrary("pupilla");
    }

    private native long create();
    private native void destroy(long cameraId);
    private native int connect(long cameraId, int vendorId, int productId,
                               int fileDescriptor, int busNumber,
                               int deviceAddress, String usbFS);
    private native int setPreviewSize(long cameraId, int width, int height, int fps_min,
                                      int fps_max, int mode, int bandWidth);
    private native String getSupportSize(long cameraId);
    public native int startPreview(long cameraId);
    public native int stopPreview(long cameraId);
    public native int setPreviewDisplay(long cameraId, Surface surface);

}
