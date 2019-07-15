package com.gu.ffmpeg_decode;

import android.os.Environment;
import android.view.SurfaceView;
import android.view.View;

import com.gu.player.BasePlayer;

public class FFDecodePlayer extends BasePlayer {
    @Override
    public View init(SurfaceView surfaceView) {
        return null;
    }

    @Override
    public boolean play(final String url) {
        final String out = Environment.getExternalStorageDirectory() + "/simple_decode.yuv";
        new Thread(new Runnable() {
            @Override
            public void run() {
                decode(url,out);
            }
        }).start();
        return false;
    }

    @Override
    public boolean stop() {
        return false;
    }

    public native void decode(String url,String out);
}
