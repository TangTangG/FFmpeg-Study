package com.player.ffmpegdemo;

import android.content.Context;
import android.graphics.PixelFormat;
import android.text.TextUtils;
import android.util.Log;
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

    private String playUrl;

    public FFPlayer(Context context) {
        this.context = context;
    }

    @Override
    public View init(SurfaceView surfaceView) {
        this.surfaceView = new SurfaceView(context);
        this.surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                FFPlayer.this.holder = holder;
                holder.setFormat(PixelFormat.RGBA_8888);
                final Surface surface = holder.getSurface();
                if (surface == null || !surface.isValid()) {
                    Log.d("tang ", "surfaceCreated: ");
                    return;
                }
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        if (!TextUtils.isEmpty(playUrl)) {
                            doFFplay(FFPlayer.this.holder.getSurface(), playUrl);
                        }
                    }
                });

            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
        return this.surfaceView;
    }

    @Override
    public boolean play(String url) {
       /* if (holder == null){

        } else {

        }
        int result = doFFplay(holder.getSurface(), url);*/
       this.playUrl = url;
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
