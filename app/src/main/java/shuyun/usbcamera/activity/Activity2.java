package shuyun.usbcamera.activity;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.view.TextureView;

import shuyun.usbcamera.CameraRender;
import shuyun.usbcamera.R;

/**
 * Created by Shuyun on 2017/12/6 0006.
 */

public class Activity2 extends AppCompatActivity {

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_2);
        TextureView textureView = (TextureView) findViewById(R.id.tv_camera);
        textureView.setSurfaceTextureListener(new CameraRender(this));
    }
}
