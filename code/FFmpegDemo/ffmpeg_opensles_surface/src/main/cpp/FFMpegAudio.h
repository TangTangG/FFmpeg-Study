//
// Created by tangcaigao on 2019/7/18.
//

#ifndef FFMPEGDEMO_FFMPEGAUDIO_H
#define FFMPEGDEMO_FFMPEGAUDIO_H

#include <SLES/OpenSLES_Android.h>
extern "C" {
#include <pthread.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <libswresample/swresample.h>

#include "Data.h"
#include "util/FF_Log.h"
#include "util/FFLockedQueue.h"

}

class FFMpegAudio {
public:

    NativePlayerContext *ctx;

    FFLockedQueue<AVPacket> *queue;

    //-------FF解码所需参数begin
    AVCodec *avCodec;
    AVCodecContext *avCodecCtx;

    int audio_stream_index;

    SwrContext *swrContext;//重采样结构体
    uint8_t *out_buffer;
    int out_channer_num;
    //-------FF解码所需参数 end
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

    jlong decode( const char *url);

    void render();

    void release();

    void reset();

    void destroy();

    void pop(AVPacket *pPacket);

    void push(AVPacket *pPacket);

private:

    bool invalidResult();

    void createSlEs(NativePlayerContext *pContext);

    void createFF(NativePlayerContext *pContext);
};


#endif //FFMPEGDEMO_FFMPEGAUDIO_H
