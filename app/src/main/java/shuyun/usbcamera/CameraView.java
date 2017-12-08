package shuyun.usbcamera;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.nio.ByteBuffer;

/**
 * Created by Shuyun on 2017/10/25 0025.
 * Attach with bitmap, low performance and hard to create filter
 */

public class CameraView extends SurfaceView implements SurfaceHolder.Callback, Runnable{

    private SurfaceHolder holder1;
    private Bitmap bitmap = null;
    private static boolean isCameraExist = false;
    private boolean isStop = false;
    public static final int IMG_WIDTH = 640;
    public static final int IMG_HEIGHT = 480;
    private int winWidth = 0;
    private int winHeight = 0;
    private Rect rect;
    private int index = 0;
    private byte[] data;
    private byte[] rgbData;
    private ByteBuffer imageBuffer;
    private Handler handler;
    private int displayOrientation = 0;
    private boolean startPreview = false;
    private boolean isMirror = false;
    private Context context;
    private boolean isViewCreated = false;

    private USBCamera usbCamera;

    public CameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.context = context;
        usbCamera = new USBCamera();
        handler = new Handler();
        holder1 = getHolder();
        holder1.addCallback(this);

        data = new byte[IMG_WIDTH * IMG_HEIGHT * 2];
        rgbData = new byte[IMG_WIDTH * IMG_HEIGHT * 4];

    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        holder1 = holder;
        isViewCreated = true;
        int res = -1;
        res = usbCamera.openCamera();
        Log.e("test", "openCamera "+res);
        if (res < 0) {
            return;
        }
        res = usbCamera.prepare(IMG_WIDTH, IMG_HEIGHT, 1);
        Log.e("test", "prepare "+res);
        if (res < 0) {
            return;
        }
        res = usbCamera.streamOn();
        Log.e("test", "streamOn "+res);
        if (res < 0) {
            return;
        }
        isCameraExist = true;
        new Thread(this).start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        isStop = true;
        release();
    }

    public void release(){
        usbCamera.streamOff();
        usbCamera.releaseCamera();
        if (isCameraExist) {
            isCameraExist = false;
            isStop = true;
            while (isStop) {
                try {
                    Thread.sleep(100);
                } catch (Exception e) {
                }
            }
        }
        holder1 = null;
        data = null;
        rgbData = null;
        winWidth = 0;
        winHeight = 0;
    }

    @Override
    public void run() {
        while(isCameraExist){
            if(0 == winWidth){
                winWidth = getWidth();
                winHeight = getHeight();
                Log.e("test", "w h "+winWidth+", "+winHeight);
                rect = new Rect(0, 0, winWidth, winHeight);
            }
            if(isStop){
                isStop = false;
                break;
            }
            index = usbCamera.frameBuffer(data);
            Log.e("test", "index "+index);
            if(index < 0){
                break;
            }
            usbCamera.yuv2rgb(data, rgbData, IMG_WIDTH, IMG_HEIGHT);
            usbCamera.queueBuffer(index);
            if(!isViewCreated){
                Log.e("test", "res isCreated "+isViewCreated);
                return;
            }
            handler.post(new Runnable() {
                @Override
                public void run() {
                    try{
                        imageBuffer = ByteBuffer.wrap(rgbData);
                        bitmap = Bitmap.createBitmap(IMG_WIDTH, IMG_HEIGHT,
                                Bitmap.Config.RGB_565);
                        bitmap.copyPixelsFromBuffer(imageBuffer);
                        Canvas canvas = holder1.lockCanvas();
                        if(null != canvas){
                            canvas.drawBitmap(bitmap, null, rect, null);
                            holder1.unlockCanvasAndPost(canvas);
                        }
                        imageBuffer.clear();
                    }catch (Exception e){
                        Log.e("test", "res getMessage "+e.getMessage());
                    }
                }
            });
        }
    }

}
