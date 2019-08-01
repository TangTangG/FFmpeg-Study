package com.gu.android_mediacodec_acc;

import android.content.Context;
import android.graphics.PixelFormat;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import com.gu.player.BasePlayer;

public class AndroidMediaCodec extends BasePlayer {

    private final Context context;
    private SurfaceView surfaceView;
    private SurfaceHolder holder;

    private String playUrl;

    public AndroidMediaCodec(Context context) {
        this.context = context;
    }

    @Override
    public View init(SurfaceView surfaceView) {
        this.surfaceView = new SurfaceView(context);
        this.surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(final SurfaceHolder holder) {
                if (AndroidMediaCodec.this.holder != null) {
                    return;
                }
                AndroidMediaCodec.this.holder = holder;
                holder.setFormat(PixelFormat.RGBA_8888);
                final Surface surface = holder.getSurface();
                if (surface == null || !surface.isValid()) {
                    return;
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
        return false;
    }

    @Override
    public boolean stop() {
        return false;
    }
}
