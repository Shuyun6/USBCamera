package shuyun.usbcamera.activity;

import android.hardware.usb.UsbDevice;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.Surface;
import android.view.View;
import android.widget.Toast;

import com.serenegiant.usb.CameraDialog;
import com.serenegiant.usb.USBMonitor;
import com.serenegiant.usb.common.AbstractUVCCameraHandler;
import com.serenegiant.usb.widget.CameraViewInterface;
import com.usbcamera.USBCameraManager;

import shuyun.usbcamera.R;

/**
 * Use UVC by com.serenegiant
 *
 * Created by Shuyun on 2017/12/11 0011.
 */

public class Activity3 extends AppCompatActivity implements CameraDialog.CameraDialogParent{

    private USBCameraManager manager;
    private CameraViewInterface cameraViewInterface;
    private boolean isPreview;
    private boolean isRequest;

    private View cameraView;

    private USBCameraManager.OnDevConnectListener listener = new USBCameraManager.OnDevConnectListener() {
        @Override
        public void onAttachDev(UsbDevice device) {
            if(null == manager || manager.getUsbDeviceCount() == 0){
                toast("No USB device found");
                return;
            }
            if(!isRequest){
                isRequest = true;
                if(null != manager){
                    manager.requestPermission(0);
                }
            }
        }

        @Override
        public void onDettachDev(UsbDevice device) {
            if (isRequest) {
                isRequest = false;
                manager.closeCamera();
                toast("USB device has dettached");
            }
        }

        @Override
        public void onConnectDev(UsbDevice device, boolean isConnected) {
            if (!isConnected) {
                toast("Failed to connect");
                isPreview = false;
            }else{
                isPreview = true;
            }
        }

        @Override
        public void onDisConnectDev(UsbDevice device) {
            toast("Device disconnected");
        }
    };

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_3);
        cameraView = findViewById(R.id.cameraview);
        cameraViewInterface = (CameraViewInterface) cameraView;
        cameraViewInterface.setCallback(new CameraViewInterface.Callback() {
            @Override
            public void onSurfaceCreated(CameraViewInterface view, Surface surface) {
                if(!isPreview && manager.isCameraOpened()){
                    manager.startPreview(cameraViewInterface, new AbstractUVCCameraHandler.OnPreViewResultListener() {
                        @Override
                        public void onPreviewResult(boolean result) {

                        }
                    });
                    isPreview = true;
                }
            }

            @Override
            public void onSurfaceChanged(CameraViewInterface view, Surface surface, int width, int height) {

            }

            @Override
            public void onSurfaceDestroy(CameraViewInterface view, Surface surface) {
                if(isPreview && manager.isCameraOpened()){
                    manager.stopPreview();
                    isPreview = false;
                }
            }
        });

        manager = USBCameraManager.getInstance();
        manager.initUSBMonitor(this, listener);
        manager.createUVCCamera(cameraViewInterface);

    }

    @Override
    protected void onStart() {
        super.onStart();
        if(null == manager){
            return;
        }
        manager.registerUSB();
        cameraViewInterface.onResume();

    }

    @Override
    protected void onStop() {
        super.onStop();
        if (null != manager) {
            manager.unregisterUSB();
        }
        cameraViewInterface.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (null != manager) {
            manager.release();
        }
    }

    private void toast(final String msg){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(Activity3.this, msg, Toast.LENGTH_SHORT).show();
            }
        });
    }


    @Override
    public USBMonitor getUSBMonitor() {
        return manager.getUSBMonitor();
    }

    @Override
    public void onDialogResult(boolean canceled) {

    }
}
