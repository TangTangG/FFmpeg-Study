package com.player.ffmpegdemo;

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
