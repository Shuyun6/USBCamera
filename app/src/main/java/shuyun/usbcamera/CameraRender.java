package shuyun.usbcamera;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.os.Handler;
import android.util.Log;
import android.view.TextureView;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.concurrent.ExecutorService;

import static android.opengl.GLES20.GL_CLAMP_TO_EDGE;
import static android.opengl.GLES20.GL_COLOR_BUFFER_BIT;
import static android.opengl.GLES20.GL_FLOAT;
import static android.opengl.GLES20.GL_LINEAR;
import static android.opengl.GLES20.GL_LUMINANCE;
import static android.opengl.GLES20.GL_LUMINANCE_ALPHA;
import static android.opengl.GLES20.GL_RGB;
import static android.opengl.GLES20.GL_TEXTURE0;
import static android.opengl.GLES20.GL_TEXTURE1;
import static android.opengl.GLES20.GL_TEXTURE2;
import static android.opengl.GLES20.GL_TEXTURE_2D;
import static android.opengl.GLES20.GL_TEXTURE_MAG_FILTER;
import static android.opengl.GLES20.GL_TEXTURE_MIN_FILTER;
import static android.opengl.GLES20.GL_TEXTURE_WRAP_S;
import static android.opengl.GLES20.GL_TEXTURE_WRAP_T;
import static android.opengl.GLES20.GL_TRIANGLE_STRIP;
import static android.opengl.GLES20.GL_UNSIGNED_BYTE;
import static android.opengl.GLES20.glActiveTexture;
import static android.opengl.GLES20.glBindTexture;
import static android.opengl.GLES20.glClear;
import static android.opengl.GLES20.glClearColor;
import static android.opengl.GLES20.glDrawArrays;
import static android.opengl.GLES20.glEnableVertexAttribArray;
import static android.opengl.GLES20.glFlush;
import static android.opengl.GLES20.glGetAttribLocation;
import static android.opengl.GLES20.glGetError;
import static android.opengl.GLES20.glGetUniformLocation;
import static android.opengl.GLES20.glTexImage2D;
import static android.opengl.GLES20.glTexParameteri;
import static android.opengl.GLES20.glUniform1i;
import static android.opengl.GLES20.glUseProgram;
import static android.opengl.GLES20.glVertexAttribPointer;
import static android.opengl.GLES20.glViewport;
import static java.lang.Thread.sleep;
import static java.util.concurrent.Executors.*;

/**
 * Created by Shuyun on 2017/12/5 0005.
 * Render YUV data with OpenGl ES
 */

public class CameraRender implements TextureView.SurfaceTextureListener {

    private Context context;
    private SurfaceTexture surfaceTexture;
    private int program_buffer, program_display;
    private FloatBuffer vertextData_buffer, vertextData_display,
            coordinationData_buffer, coordinationData_display;
    private ByteBuffer buffer_y, buffer_uv;
    private int display_width, display_height;
    private int buffer_width, buffer_height;
    private float VERTEX[] = {
        1.0f, -1.0f,
        -1.0f, -1.0f,
        1.0f, 1.0f,
        -1.0f, 1.0f
    };
    private float COORD1[] = {
        1.0f, 0.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    private float COORD2[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };
    private GLHelper glHelper;
    private int texture_Y, texture_UV;
    private USBCamera usbCamera;
    private boolean isPreviewing;
    private Bitmap bitmap;
    private ExecutorService pool;
    private Handler handler;

    public CameraRender(Context context){
        this.context = context;
        this.buffer_width = USBCamera.IMG_WIDTH;
        this.buffer_height = USBCamera.IMG_HEIGHT;

        usbCamera = new USBCamera();
        glHelper = new GLHelper(context);

        texture_Y = glHelper.genTexture();
        texture_UV = glHelper.genTexture();
//        bitmap = BitmapFactory.decodeResource(context.getResources(), R.drawable.image);
        pool = newCachedThreadPool();
        handler = new Handler();
    }

    private void printError(int index){
        int error = glGetError();
        if(error != 0){
            Log.e("test", index+" "+error);
        }
    }

    private synchronized void draw(byte[] data){
        glViewport(0, 0, buffer_width, buffer_height);
        glClear(GL_COLOR_BUFFER_BIT);
        buffer_y.clear();
        buffer_uv.clear();

        buffer_y.put(data, 0, buffer_width * buffer_height);
        buffer_y.position(0);
        buffer_uv.put(data, buffer_width * buffer_height, buffer_width * buffer_height / 2);
        buffer_uv.position(0);

        //render data into frame buffer
        glUseProgram(program_buffer);
        glBindTexture(GL_TEXTURE_2D, texture_Y);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, buffer_width, buffer_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer_y);
//        GLUtils.texImage2D(GL_TEXTURE_2D, 0, bitmap, 0);
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindTexture(GL_TEXTURE_2D, texture_UV);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, buffer_width/2, buffer_height/2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buffer_uv);
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glBindTexture(GL_TEXTURE_2D, 0);

        int aPosition = glGetAttribLocation(program_buffer, "aPosition");
        glEnableVertexAttribArray(aPosition);
        glVertexAttribPointer(aPosition, 2, GL_FLOAT, false, 0, vertextData_buffer);

        int aTextureCoord = glGetAttribLocation(program_buffer, "aTextureCoord");
        glEnableVertexAttribArray(aTextureCoord);
        glVertexAttribPointer(aTextureCoord, 2, GL_FLOAT, false, 0, coordinationData_buffer);

        int texY = glGetUniformLocation(program_buffer, "u_TextureY");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_Y);
        glUniform1i(texY, 0);

        int texUV = glGetUniformLocation(program_buffer, "u_TextureUV");
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture_UV);
        glUniform1i(texUV, 1);

        glHelper.bindFrameBuffer();
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glHelper.unBindFrameBuffer();
        glClear(GL_COLOR_BUFFER_BIT);

        //draw data from frame buffer
        glUseProgram(program_display);
        int aPosition2 = glGetAttribLocation(program_display, "aPosition");
        glEnableVertexAttribArray(aPosition2);
        glVertexAttribPointer(aPosition2, 2, GL_FLOAT, false, 0, vertextData_display);

        int aTextureCoord2 = glGetAttribLocation(program_display, "aTextureCoord");
        glEnableVertexAttribArray(aTextureCoord2);
        glVertexAttribPointer(aTextureCoord2, 2, GL_FLOAT, false, 0, coordinationData_display);

        int texture2 = glGetUniformLocation(program_display, "u_Texture");
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, glHelper.getTexture2Framebuffer());
        glUniform1i(texture2, 2);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glFlush();
        glHelper.swapBuffer();

    }

    private void makeBufferData(){
        vertextData_buffer = ByteBuffer.allocateDirect(VERTEX.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        coordinationData_buffer = ByteBuffer.allocateDirect(COORD1.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        vertextData_buffer.put(VERTEX);
        vertextData_buffer.position(0);
        coordinationData_buffer.put(COORD1);
        coordinationData_buffer.position(0);
        program_buffer = glHelper.getProgram(R.raw.vertex_shader, R.raw.fragment_shader_yuv);

    }

    private void makeDisplayData(){
        vertextData_display = ByteBuffer.allocateDirect(VERTEX.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        coordinationData_display = ByteBuffer.allocateDirect(COORD2.length * 4)
                .order(ByteOrder.nativeOrder())
                .asFloatBuffer();
        vertextData_display.put(VERTEX);
        vertextData_display.position(0);
        coordinationData_display.put(COORD2);
        coordinationData_display.position(0);
        program_display = glHelper.getProgram(R.raw.vertex_shader, R.raw.fragment_shader);
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        this.surfaceTexture = surface;
        this.display_width = width;
        this.display_height = height;

        buffer_y = ByteBuffer.allocateDirect(buffer_width * buffer_height * 4)
                .order(ByteOrder.nativeOrder());
        buffer_uv = ByteBuffer.allocateDirect(buffer_width * buffer_height * 4)
                .order(ByteOrder.nativeOrder());
//
        glHelper.setDisplaySize(width, height);
        glHelper.initEGL(surfaceTexture);
        makeBufferData();
        makeDisplayData();
        glHelper.setupFrameBuffer();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        int res = -1;
        res = usbCamera.openCamera();
        Log.e("test", "openCamera "+res);
        if (res < 0) {
            return;
        }
        res = usbCamera.prepare(buffer_width, buffer_height, 1);
        Log.e("test", "prepare "+res);
        if (res < 0) {
            return;
        }
        res = usbCamera.streamOn();
        Log.e("test", "streamOn "+res);
        if (res < 0) {
            return;
        }
        isPreviewing = true;
        pool.execute(new Runnable() {
            @Override
            public void run() {
            while (isPreviewing) {
                byte[] data = new byte[buffer_width * buffer_height * 2];
                final byte[] data_NV21 = new byte[buffer_width * buffer_height * 3 / 2];
                int index = usbCamera.frameBuffer(data);
                if (index < 0) {
                    break;
                }
                usbCamera.yuv2nv21(data, data_NV21, buffer_width, buffer_height);
                usbCamera.queueBuffer(index);
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        draw(data_NV21);
                    }
                });
                try {
                    sleep(5);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            }
        });

    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {

    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        isPreviewing = false;
        usbCamera.streamOff();
        usbCamera.releaseCamera();
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {

    }

}
