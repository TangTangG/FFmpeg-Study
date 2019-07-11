package com.player.ffmpegdemo;

import android.content.Context;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * 使用FFmpeg完成视频播放
 */
public class FFPlayer extends BasePlayer {

    private SurfaceView surfaceView;
    private SurfaceHolder holder;

    public FFPlayer(Context context) {
        surfaceView = new SurfaceView(context);
        holder = surfaceView.getHolder();
    }

    @Override
    public void init() {

    }

    @Override
    public boolean play(String url) {
        return false;
    }

    @Override
    public boolean stop() {
        return false;
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
//    public native String doFFplay(String url,SurfaceView surface);

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("avutil-56");
        System.loadLibrary("avcodec-58");
        System.loadLibrary("avformat-58");
        System.loadLibrary("avdevice-58");
        System.loadLibrary("swresample-3");
        System.loadLibrary("swscale-5");
        System.loadLibrary("postproc-55");
        System.loadLibrary("avfilter-7");
        System.loadLibrary("native-lib");
    }
}
