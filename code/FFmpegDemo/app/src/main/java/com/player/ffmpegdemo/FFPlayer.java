package com.player.ffmpegdemo;

import android.content.Context;
import android.graphics.PixelFormat;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

/**
 * 使用FFmpeg完成视频播放
 */
public class FFPlayer extends BasePlayer {

    private final Context context;
    private SurfaceView surfaceView;
    private SurfaceHolder holder;

    public FFPlayer(Context context) {
        this.context = context;
    }

    @Override
    public View init(SurfaceView surfaceView) {
        this.surfaceView = surfaceView;
        holder = surfaceView.getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
        return surfaceView;
    }

    @Override
    public boolean play(String url) {
        int result = doFFplay(holder.getSurface(), url);
        return result == 0;
    }

    @Override
    public boolean stop() {
        return false;
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native int doFFplay(Surface surface, String url);

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
