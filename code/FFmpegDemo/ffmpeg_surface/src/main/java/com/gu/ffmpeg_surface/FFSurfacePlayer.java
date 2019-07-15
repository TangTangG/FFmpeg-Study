package com.gu.ffmpeg_surface;

import android.content.Context;
import android.graphics.PixelFormat;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import com.gu.player.BasePlayer;

/**
 * 使用FFmpeg完成视频播放
 */
public final class FFSurfacePlayer extends BasePlayer {

    private final Context context;
    private SurfaceView surfaceView;
    private SurfaceHolder holder;

    private String playUrl;

    public FFSurfacePlayer(Context context) {
        this.context = context;
    }

    @Override
    public View init(SurfaceView surfaceView) {
        this.surfaceView = new SurfaceView(context);
        this.surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                if (FFSurfacePlayer.this.holder != null) {
                    return;
                }
                FFSurfacePlayer.this.holder = holder;
                holder.setFormat(PixelFormat.RGBA_8888);
                final Surface surface = holder.getSurface();
                if (surface == null || !surface.isValid()) {
                    return;
                }
                Log.d("tang ", "surfaceCreated: ");
                if (!TextUtils.isEmpty(playUrl)) {
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            doFFplay(FFSurfacePlayer.this.holder.getSurface(), playUrl);
                        }
                    }).start();
                }
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
        this.playUrl = url;
        return false;
    }

    @Override
    public boolean stop() {
        return false;
    }

    public native int doFFplay(Surface surface, String url);

}
