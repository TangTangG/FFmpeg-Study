package com.gu.player;

public abstract class BasePlayer implements SimplePlayerIface {

    @Override
    public boolean pause() {
        return false;
    }

    @Override
    public boolean destroy() {
        return false;
    }

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
