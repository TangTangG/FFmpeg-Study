package com.player.ffmpegdemo;

public interface SimplePlayerIface {

    void init();

    boolean play(String url);

    boolean pause();

    boolean stop();

    boolean destroy();

}
