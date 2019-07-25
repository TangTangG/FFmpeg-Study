//
// Created by tangcaigao on 2019/7/24.
//

#ifndef FFMPEGDEMO_DATA_H
#define FFMPEGDEMO_DATA_H

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "util/FFThreadPool.h"
#include <android/native_window_jni.h>

typedef struct NativePlayerContext {

    void *display;

    AVFormatContext *formatCtx;
    FFThreadPoolContext *threadPoolCtx;
    jobject *callback;
    bool debug;
    double video_time;
    double audio_time;
    bool video_down;
    bool audio_down;
}NativePlayerContext;

#endif //FFMPEGDEMO_DATA_H
