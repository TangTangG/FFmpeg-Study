package com.player.ffmpegdemo;

import android.view.SurfaceView;
import android.view.View;

public interface SimplePlayerIface {


    View init(SurfaceView surfaceView);

    boolean play(String url);

    boolean pause();

    boolean stop();

    boolean destroy();

}
