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

}
