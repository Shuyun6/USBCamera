package com.usbcamera;

import android.app.Activity;
import android.graphics.SurfaceTexture;
import android.hardware.usb.UsbDevice;
import android.os.Environment;

import com.serenegiant.usb.DeviceFilter;
import com.serenegiant.usb.Size;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.UVCCamera;
import com.serenegiant.usb.common.AbstractUVCCameraHandler;
import com.serenegiant.usb.common.UVCCameraHandler;
import com.serenegiant.usb.encoder.RecordParams;
import com.serenegiant.usb.widget.CameraViewInterface;

import java.io.File;
import java.util.List;

public class USBCameraManager{
    public static final String ROOT_PATH = Environment.getExternalStorageDirectory().getAbsolutePath()
            + File.separator;
    public static final String SUFFIX_PNG = ".png";
    public static final String SUFFIX_MP4 = ".mp4";
    private static final String TAG = "USBCameraManager";
    private int previewWidth = 640;
    private int previewHeight = 480;
    public static int MODE_BRIGHTNESS = UVCCamera.PU_BRIGHTNESS;
    public static int MODE_CONTRAST = UVCCamera.PU_CONTRAST;
    private static final int ENCODER_TYPE = 2;
    //0: YUYV，1: MJPEG
    private static final int PREVIEW_FORMAT = 0;

    private static USBCameraManager mUsbCamManager;
    private USBMonitor mUSBMonitor;
    private UVCCameraHandler mCameraHandler;
    private Activity mActivity;

    private USBMonitor.UsbControlBlock mCtrlBlock;
    private CameraViewInterface cameraView;

    private USBCameraManager(){}

    public static USBCameraManager getInstance(){
        if(mUsbCamManager == null){
            mUsbCamManager = new USBCameraManager();
        }
        return mUsbCamManager;
    }

    public void closeCamera() {
        if(mCameraHandler != null){
            mCameraHandler.close();
        }
    }

    public interface OnDevConnectListener{
        void onAttachDev(UsbDevice device);
        void onDettachDev(UsbDevice device);
        void onConnectDev(UsbDevice device, boolean isConnected);
        void onDisConnectDev(UsbDevice device);
    }

    public interface OnPreviewListener{
        void onPreviewResult(boolean isSuccess);
    }

    public void initUSBMonitor(Activity activity, final OnDevConnectListener listener){
        this.mActivity = activity;

        mUSBMonitor = new USBMonitor(activity.getApplicationContext(), new USBMonitor.OnDeviceConnectListener() {

            @Override
            public void onAttach(UsbDevice device) {
                if(listener != null){
                    listener.onAttachDev(device);
                }
            }

            @Override
            public void onDettach(UsbDevice device) {
                if(listener != null){
                    listener.onDettachDev(device);
                }
            }

            @Override
            public void onConnect(final UsbDevice device, USBMonitor.UsbControlBlock ctrlBlock, boolean createNew) {
                mCtrlBlock = ctrlBlock;
                openCamera(ctrlBlock);
                startPreview(cameraView, new AbstractUVCCameraHandler.OnPreViewResultListener() {
                    @Override
                    public void onPreviewResult(boolean isConnected) {
                        if(listener != null){
                            listener.onConnectDev(device, isConnected);
                        }
                    }
                });
            }

            @Override
            public void onDisconnect(UsbDevice device, USBMonitor.UsbControlBlock ctrlBlock) {
                if(listener != null){
                    listener.onDisConnectDev(device);
                }
            }

            @Override
            public void onCancel(UsbDevice device) {
            }
        });
    }

    public void createUVCCamera(CameraViewInterface cameraView) {
        if(cameraView == null)
            throw new NullPointerException("CameraViewInterface cannot be null!");

        if(mCameraHandler != null){
            mCameraHandler.release();
            mCameraHandler = null;
        }
        this.cameraView = cameraView;
        cameraView.setAspectRatio(previewWidth / (float) previewHeight);
        mCameraHandler = UVCCameraHandler.createHandler(mActivity, cameraView, ENCODER_TYPE,
                previewWidth, previewHeight, PREVIEW_FORMAT);
    }

    public void updateResolution(int width, int height, final OnPreviewListener mPreviewListener){
        if(previewWidth == width && previewHeight == height){
            return;
        }
        this.previewWidth = width;
        this.previewHeight = height;
        if(mCameraHandler != null){
            mCameraHandler.release();
            mCameraHandler = null;
        }
        cameraView.setAspectRatio(previewWidth / (float)previewHeight);
        mCameraHandler = UVCCameraHandler.createHandler(mActivity,cameraView,ENCODER_TYPE,
                previewWidth,previewHeight,PREVIEW_FORMAT);
        openCamera(mCtrlBlock);
        startPreview(cameraView, new AbstractUVCCameraHandler.OnPreViewResultListener() {
            @Override
            public void onPreviewResult(boolean result) {
                if(mPreviewListener != null){
                    mPreviewListener.onPreviewResult(result);
                }
            }
        });
    }

    public void restartUSBCamera(CameraViewInterface cameraView,final OnPreviewListener mPreviewListener){
        if(mCtrlBlock == null || cameraView == null)
           throw new NullPointerException("mCtrlBlock or cameraView is null,please connected to camera");
        createUVCCamera(cameraView);
        openCamera(mCtrlBlock);
        startPreview(cameraView, new AbstractUVCCameraHandler.OnPreViewResultListener() {
            @Override
            public void onPreviewResult(boolean result) {
                if(mPreviewListener != null){
                    mPreviewListener.onPreviewResult(result);
                }
            }
        });
    }

    public void registerUSB(){
        if(mUSBMonitor != null){
            mUSBMonitor.register();
        }
    }

    public void unregisterUSB(){
        if(mUSBMonitor != null){
            mUSBMonitor.unregister();
        }
    }

    public boolean checkSupportFlag(final int flag) {
        return mCameraHandler != null && mCameraHandler.checkSupportFlag(flag);
    }

    public int getModelValue(final int flag) {
        return mCameraHandler != null ? mCameraHandler.getValue(flag) : 0;
    }

    public int setModelValue(final int flag, final int value) {
        return mCameraHandler != null ? mCameraHandler.setValue(flag, value) : 0;
    }

    public int resetModelValue(final int flag) {
        return mCameraHandler != null ? mCameraHandler.resetValue(flag) : 0;
    }

    public void requestPermission(int index){
        List<UsbDevice> devList = getUsbDeviceList();
        if(devList==null || devList.size() ==0){
            return;
        }
        int count = devList.size();
        if(index >= count)
            new IllegalArgumentException("index illegal,should be < devList.size()");
        if(mUSBMonitor != null) {
            mUSBMonitor.requestPermission(getUsbDeviceList().get(index));
        }
    }

    public int getUsbDeviceCount(){
        List<UsbDevice> devList = getUsbDeviceList();
        if(devList==null || devList.size() ==0){
            return 0;
        }
        return devList.size();
    }

    public List<UsbDevice> getUsbDeviceList(){
        List<DeviceFilter> deviceFilters = DeviceFilter.getDeviceFilters(mActivity.getApplicationContext(), R.xml.device_filter);
        if(mUSBMonitor == null || deviceFilters == null)
            return null;
        return mUSBMonitor.getDeviceList(deviceFilters.get(0));
    }

    public void capturePicture(String savePath,AbstractUVCCameraHandler.OnCaptureListener listener){
        if(mCameraHandler != null && mCameraHandler.isOpened()){
            mCameraHandler.captureStill(savePath,listener);
        }
    }

    public void startRecording(RecordParams params, AbstractUVCCameraHandler.OnEncodeResultListener listener){
        if(mCameraHandler != null && ! isRecording()){
            mCameraHandler.startRecording(params,listener);
        }
    }

    public void stopRecording(){
        if(mCameraHandler != null && isRecording()){
            mCameraHandler.stopRecording();
        }
    }

    public boolean isRecording(){
        if(mCameraHandler != null){
            return mCameraHandler.isRecording();
        }
        return false;
    }

    public boolean isCameraOpened(){
        if(mCameraHandler != null){
            return mCameraHandler.isOpened();
        }
        return false;
    }

    public void release(){
        if(mCameraHandler != null){
            mCameraHandler.release();
            mCameraHandler = null;
        }
        if(mUSBMonitor != null){
            mUSBMonitor.destroy();
            mUSBMonitor = null;
        }
    }

    public USBMonitor getUSBMonitor() {
        return mUSBMonitor;
    }


    private void openCamera(USBMonitor.UsbControlBlock ctrlBlock) {
        if(mCameraHandler != null){
            mCameraHandler.open(ctrlBlock);
        }
    }

    public void startPreview(CameraViewInterface cameraView, AbstractUVCCameraHandler.OnPreViewResultListener mPreviewListener) {
        SurfaceTexture st = cameraView.getSurfaceTexture();
        if(mCameraHandler != null){
            mCameraHandler.startPreview(st, mPreviewListener);
        }
    }

    public void stopPreview() {
        if(mCameraHandler != null){
            mCameraHandler.stopPreview();
        }
    }

    public void startCameraFoucs(){
        if(mCameraHandler != null){
            mCameraHandler.startCameraFoucs();
        }
    }

    public List<Size> getSupportedPreviewSizes(){
        if(mCameraHandler == null)
            return null;
        return mCameraHandler.getSupportedPreviewSizes();
    }
}
