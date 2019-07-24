//
// Created by tangcaigao on 2019/7/18.
//

#ifndef FFMPEGDEMO_FFMPEGVIDEO_H
#define FFMPEGDEMO_FFMPEGVIDEO_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <unistd.h>
#include "Data.h"
#include "util/FF_Log.h"

}


class FFMpegVideo {
public:
    void create(NativePlayerContext *ctx);
    jlong decode(NativePlayerContext *ctx,const char *url);
    void render(NativePlayerContext *ctx,jlong audio_time);
    void release(NativePlayerContext *ctx);
    void reset(NativePlayerContext *ctx);
    void destroy(NativePlayerContext *ctx);
};


#endif //FFMPEGDEMO_FFMPEGVIDEO_H
