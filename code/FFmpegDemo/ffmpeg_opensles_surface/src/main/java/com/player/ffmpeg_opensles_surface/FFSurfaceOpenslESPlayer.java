package com.player.ffmpeg_opensles_surface;

import android.content.Context;
import android.graphics.PixelFormat;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import com.gu.player.BasePlayer;

public class FFSurfaceOpenslESPlayer extends BasePlayer {

    private final Context context;
    private SurfaceView surfaceView;
    private SurfaceHolder holder;

    private String playUrl;

    public FFSurfaceOpenslESPlayer(Context context) {
        this.context = context;
    }

    @Override
    public View init(SurfaceView surfaceView) {
        this.surfaceView = new SurfaceView(context);
        this.surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                if (FFSurfaceOpenslESPlayer.this.holder != null) {
                    return;
                }
                FFSurfaceOpenslESPlayer.this.holder = holder;
                holder.setFormat(PixelFormat.RGBA_8888);
                final Surface surface = holder.getSurface();
                if (surface == null || !surface.isValid()) {
                    return;
                }
                attachFFView(surface);
                if (!TextUtils.isEmpty(playUrl)) {
                    doFFPlay(playUrl);
                }
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });
        doFFInit();
        return this.surfaceView;
    }

    @Override
    public boolean play(String url) {
        this.playUrl = url;doFFPrepare();

        return false;
    }

    @Override
    public boolean stop() {
        return false;
    }

}
