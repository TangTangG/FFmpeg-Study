package com.gu.player;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public final class SimpleThreadPoolExecutor {

    private static ExecutorService pool;

    public static ExecutorService getPool() {
        if (pool == null) {
            synchronized (SimpleThreadPoolExecutor.class) {
                if (pool == null) {
                    pool = new ThreadPoolExecutor(0, 100,
                            60L, TimeUnit.SECONDS,
                            new SynchronousQueue<Runnable>());
                }
            }
        }
        return pool;
    }

    public static void stop() {
        pool.shutdownNow();
    }
}
