//
// Created by tangcaigao on 2019/7/18.
//

#ifndef FFMPEGDEMO_FFMPEGVIDEO_H
#define FFMPEGDEMO_FFMPEGVIDEO_H
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "Data.h"
#include <android/native_window_jni.h>


class FFMpegVideo {
public:
    void create(NativePlayerContext *ctx);
    void prepare(NativePlayerContext *ctx);
    void *decode(NativePlayerContext *ctx,const char *url);

};


#endif //FFMPEGDEMO_FFMPEGVIDEO_H
