//
// Created by tangcaigao on 2019/7/24.
//

#ifndef FFMPEGDEMO_DATA_H
#define FFMPEGDEMO_DATA_H

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <android/native_window_jni.h>

typedef struct NativePlayerContext {
    AVFormatContext *formatCtx;


};

#endif //FFMPEGDEMO_DATA_H
