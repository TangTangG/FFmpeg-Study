package com.player.ffmpegdemo;

import android.view.SurfaceView;
import android.view.View;

public abstract class BasePlayer implements SimplePlayerIface {

    @Override
    public boolean pause() {
        return false;
    }

    @Override
    public boolean destroy() {
        return false;
    }

}
