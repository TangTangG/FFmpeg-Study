package com.gu.ffmpeg_decode;

import android.view.SurfaceView;
import android.view.View;

import com.gu.player.BasePlayer;

public class FFDecodePlayer extends BasePlayer {
    @Override
    public View init(SurfaceView surfaceView) {
        return null;
    }

    @Override
    public boolean play(String url) {
        decode(url);
        return false;
    }

    @Override
    public boolean stop() {
        return false;
    }

    public native void decode(String url);
}
