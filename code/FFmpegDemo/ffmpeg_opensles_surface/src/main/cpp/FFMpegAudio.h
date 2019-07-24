//
// Created by tangcaigao on 2019/7/18.
//

#ifndef FFMPEGDEMO_FFMPEGAUDIO_H
#define FFMPEGDEMO_FFMPEGAUDIO_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <SLES/OpenSLES_Android.h>

#include "Data.h"
#include "util/FF_Log.h"

}

class FFMpegAudio {
public:
    void create(NativePlayerContext *ctx);
    jlong decode(NativePlayerContext *ctx,const char *url);
    void render(NativePlayerContext *ctx,jlong video_time);
    void release(NativePlayerContext *ctx);
    void reset(NativePlayerContext *ctx);
    void destroy(NativePlayerContext *ctx);


};


#endif //FFMPEGDEMO_FFMPEGAUDIO_H
