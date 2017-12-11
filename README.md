# USBCamera
Android USB camera, convert and stream via FFmpeg.

## USBCamera class:

Load native library to convert media data format and stream video data by FFmpeg.

## GLHepler class:

Initialized OpenGLES and EGL environment, methods for genarating texture、shader and set up the FBO(Frame Buffer Object) to render media data.

## CameraView class:（Activity1）

Get frame data and convert YUV to RGB through USBCamera, create a bitmap from RGB data then show by SurfaceHolder.(Low performance)

## CameraRender class:（Activity2）

Create a texture with frame data that come from USBCamera, use OpenGLES to render texture to FBO then convert YUV to RGB in fragment shader(High performce)

PS: This project is do well in the Android Device without system camera module which attack USB camera, but it can find usb camera in some Android phone.

---- 2017-12-08

## USBCameraManager:(Activity3)

With Activity1 and Activity2 use FFmpeg library that makes apk file too large, and the USB camera did not work in the some smart phone like "Redmi Note 4X". So I use a new way to handle it, module from (https://github.com/saki4510t/UVCCamera).

---- 2017-12-11
