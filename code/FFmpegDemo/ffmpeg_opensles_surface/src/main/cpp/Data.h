//
// Created by tangcaigao on 2019/7/24.
//

#ifndef FFMPEGDEMO_DATA_H
#define FFMPEGDEMO_DATA_H

#define PLAYER_STATE_PALYING 1
#define PLAYER_STATE_PLAY 2
#define PLAYER_STATE_INITIAL -2
#define PLAYER_STATE_IDE -1
#define PLAYER_STATE_PAUSE -3
#define PLAYER_STATE_STOP -4
#define PLAYER_STATE_DESTORY -5

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "util/FFThreadPool.h"
#include <android/native_window_jni.h>

typedef struct NativePlayerContext {

    int play_state;

    void *display;

    AVFormatContext *formatCtx;
    FFThreadPoolContext *threadPoolCtx;
    jobject *callback;
    bool debug;
    double video_clock;
    double audio_clock;
    bool video_down;
    bool audio_down;
}NativePlayerContext;

#endif //FFMPEGDEMO_DATA_H
