package com.gu.player;

import android.view.Surface;

public abstract class BasePlayer implements SimplePlayerIface {

    @Override
    public boolean pause() {
        return false;
    }

    @Override
    public boolean destroy() {
        return false;
    }

    @Override
    public void test() {
        doClangTest();
    }

    private static final boolean LOAD = getLoad();

    protected static boolean getLoad() {
        return true;
    }

    static {
        /*if (LOAD) {
            System.loadLibrary("avutil-56");
            System.loadLibrary("avcodec-58");
            System.loadLibrary("avformat-58");
            System.loadLibrary("avdevice-58");
            System.loadLibrary("swresample-3");
            System.loadLibrary("swscale-5");
            System.loadLibrary("postproc-55");
            System.loadLibrary("avfilter-7");
            System.loadLibrary("native-lib");
        }*/
    }


    protected native void doClangTest();

    protected native void doFFInit();

    protected native void doFFPrepare();

    protected native void doFFStart();

    /**
     * @return pause position
     */
    protected native long doFFPause();

    /**
     * @return stop position
     */
    protected native long doFFStop();

    protected native long doFFSeekTo(long target);

    protected native void doFFDestroy();

    protected native void doFFRest();

    /**
     * start and play
     *
     * @return total duration
     */
    protected native long doFFPlay(String playUrl);

    /**
     * @return total duration
     */
    protected native long setFFDataSource(String playUrl);

    protected native void attachFFView(Surface surface);

    protected native long getFFPos();

}
