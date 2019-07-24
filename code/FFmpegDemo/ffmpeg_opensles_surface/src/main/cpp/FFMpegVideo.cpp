//
// Created by tangcaigao on 2019/7/18.
//

#include "FFMpegVideo.h"

//video decoder
AVCodec *avCodec;
AVCodecContext *avCodecCtx;
int video_stream_index;
//for display
SwsContext *swsContext;
ANativeWindow *pNativeWindow;
AVFrame *avFrame;
AVFrame *rgb_frame;

void *FFMpegVideo::decode(NativePlayerContext *ctx, const char *url) {
    return NULL;
}

void FFMpegVideo::prepare(NativePlayerContext *ctx) {

}

void FFMpegVideo::create(NativePlayerContext *ctx) {

}
