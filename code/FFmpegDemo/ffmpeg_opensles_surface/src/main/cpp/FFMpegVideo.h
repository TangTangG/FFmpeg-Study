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
#include "libavutil/time.h"
#include <unistd.h>
#include "Data.h"
#include "util/FF_Log.h"
#include "util/FFLockedQueue.h"
}


class FFMpegVideo {
public:

    AVRational time_base;

    FFLockedQueue<AVPacket> *queue;
    NativePlayerContext *pCtx;
    //video decoder
    AVCodec *avCodec;

    AVCodecContext *avCodecCtx;
    int video_stream_index;

//for display
    SwsContext *swsContext;
    ANativeWindow *pNativeWindow;
    AVFrame *avFrame;

    AVFrame *rgb_frame;

    void create(NativePlayerContext *ctx);
    jlong decode( const char *string);
    bool renderInit();
    void release();
    void reset();
    void destroy();
    void push(AVPacket *pPacket);
};


#endif //FFMPEGDEMO_FFMPEGVIDEO_H
