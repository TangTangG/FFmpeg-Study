//
// Created by tangcaigao on 2019/7/18.
//

#ifndef FFMPEGDEMO_FFMPEGAUDIO_H
#define FFMPEGDEMO_FFMPEGAUDIO_H

#include <queue>
#include<vector

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <SLES/OpenSLES_Android.h>
#include <libswresample/swresample.h>

#include "Data.h"
#include "util/FF_Log.h"

}

class FFMpegAudio {
public:

    AVCodec *avCodec;

    AVCodecContext *avCodecCtx;
    int audio_stream_index;

    std::vector<AVPacket*> queue;//数据包队列

    SwrContext *swrContext;//重采样结构体
    uint8_t *out_buffer;
    int out_channer_num;

    //音频当前播放的时间戳
    double audio_time;
    AVRational time_base;

    /**
* SLObjectItf ---> 创建其他SL对象的接口
*/
//引擎对象接口
    SLObjectItf engineIface;
//混音器对象接口
    SLObjectItf outputMixIface;
//音频播放器对象接口
    SLObjectItf audioPlayerIface;

//-----------分割线

    SLEngineItf engineObj;

    SLEnvironmentalReverbItf outputMixObj;

    SLPlayItf playerObj;

    SLVolumeItf playerVolume;

    SLAndroidSimpleBufferQueueItf playerBufferQueue;

    SLEffectSendItf playerEffectSend;

/**
 * 函数执行结果
 */
    SLresult result;

    void create(NativePlayerContext *ctx);

    jlong decode(NativePlayerContext *ctx, const char *url);

    void render(NativePlayerContext *ctx, jlong video_time);

    void release(NativePlayerContext *ctx);

    void reset(NativePlayerContext *ctx);

    void destroy(NativePlayerContext *ctx);

private:

    bool invalidResult();

    void createSlEs(NativePlayerContext *pContext);

    void createFF(NativePlayerContext *pContext);
};


#endif //FFMPEGDEMO_FFMPEGAUDIO_H
