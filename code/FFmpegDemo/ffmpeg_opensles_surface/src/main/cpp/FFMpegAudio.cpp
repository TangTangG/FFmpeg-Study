//
// Created by tangcaigao on 2019/7/18.
//

#include <unistd.h>
#include "FFMpegAudio.h"

#define DEFAULT_SAMPLING_RATE 44100

int decode2PCM(FFMpegAudio *audio);

/**1、创建接口对象

        2、设置混音器

        3、创建播放器（录音器）

        4、设置缓冲队列和回调函数

        5、设置播放状态

        6、启动回调函数

        其中4和6是播放PCM等数据格式的音频是需要用到的。
！*/



bool FFMpegAudio::invalidResult() {
    bool b = SL_RESULT_SUCCESS != result;
    if (!b) {
        LOGE("");
    }
    return b;
}

bool pop(FFMpegAudio *audio ,AVPacket *pPacket) {
    AVPacket *avPacket = audio->queue->pop();
    if (avPacket == NULL) {
        return false;
    }
    if (!av_packet_ref(pPacket, avPacket)) {
        av_packet_unref(avPacket);
        AVPacket *remove = audio->queue->releaseHead();
        av_free(remove);
    }
    return true;
}

void FFMpegAudio::push(FFMpegAudio *audio ,AVPacket *pPacket) {
    AVPacket *avPacket = static_cast<AVPacket *>(av_mallocz(sizeof(AVPacket)));
    if (!av_packet_ref(avPacket, pPacket)) {
        audio->queue->push( avPacket);
    }

}

//音频缓存回调函数
void playerBQCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    //得到pcm数据
    FFMpegAudio *audio = (FFMpegAudio *) context;
    int data_size = decode2PCM(audio);
    if (data_size > 0) {
        //该帧所需要时间 = 采样字节/采样率
        double time = data_size / (DEFAULT_SAMPLING_RATE * 2 * 2);
        audio->pCtx->audio_time = time + audio->pCtx->audio_time;
        LOGE("当前帧声音时间%f   播放时间%f", time, audio->pCtx->audio_time);

        (*bq)->Enqueue(bq, audio->out_buffer, data_size);
        LOGE("待播放 %d ", audio->queue->size());
    }
}

int decode2PCM(FFMpegAudio *audio) {
    AVPacket *avPacket = static_cast<AVPacket *>(av_mallocz(sizeof(AVPacket)));
    AVFrame *avFrame = av_frame_alloc();
    int data_size;
    int result;
    while (audio->pCtx->play_state > 0) {
        data_size = 0;
        if (!pop(audio, avPacket)) {
            usleep(16000);
            break;
        }
        LOGD("TANG 获取一个音频包裹");
        if (avPacket->pts != AV_NOPTS_VALUE) {
            //qts --> double 校正时间
            audio->pCtx->audio_time = av_q2d(audio->time_base) * avPacket->pts;
        }
        //解码
        result = avcodec_send_packet(audio->avCodecCtx, avPacket);
        if (result != 0) {
            if (result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                LOGE("audio avcodec_send_packet error %d", result);
            }
            continue;
        }

        result = avcodec_receive_frame(audio->avCodecCtx, avFrame);
        if (result != 0) {
            if (result != AVERROR_EOF) {
                LOGE("audio avcodec_receive_frame error %d", result);
            }
            continue;
        }
        //重采样
        swr_convert(audio->swrContext, &(audio->out_buffer), DEFAULT_SAMPLING_RATE * 2,
                    (const uint8_t **) avFrame->data, avFrame->nb_samples);
        data_size = av_samples_get_buffer_size(NULL, audio->out_channer_num, avFrame->nb_samples,
                                               AV_SAMPLE_FMT_S16, 1);
        break;
    }
    av_free(avPacket);
    av_frame_free(&avFrame);
    return data_size;
}

void FFMpegAudio::create(NativePlayerContext *ctx) {
    //初始化用于队列同步的锁
    queue = new FFLockedQueue<AVPacket>();
    queue->init();
    pCtx = ctx;
    createSlEs(ctx);
    createFF(ctx);
}

jlong FFMpegAudio::decode(const char *url) {
    //---------找到对应解码器
    AVFormatContext *formatCtx = pCtx->formatCtx;
    audio_stream_index = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audio_stream_index == -1) {
        ff_notify_msg(1, "can not find audio stream.");
        return 0L;
    }

    time_base = pCtx->formatCtx->streams[audio_stream_index]->time_base;
    //初始化视频流编码器上下文
    AVCodecParameters *codecpar = formatCtx->streams[audio_stream_index]->codecpar;
    avCodec = avcodec_find_decoder(codecpar->codec_id);
    avCodecCtx = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(avCodecCtx, codecpar);
    //---------设置重采样参数
    //声道分布
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //输出采样位数 16
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
    //输入和输出的采样率要一致
    int out_sample_rate = avCodecCtx->sample_rate;
    swr_alloc_set_opts(swrContext, out_ch_layout, out_format, out_sample_rate,
                       avCodecCtx->channel_layout, avCodecCtx->sample_fmt, out_sample_rate, 0,
                       NULL);

    swr_init(swrContext);

    out_channer_num = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

    return out_channer_num;
}

void FFMpegAudio::renderInit() {
    /*//解码stream获取avpacket
    int ret;
    //开始取帧 渲染流程
    AVPacket *flush_pkt = static_cast<AVPacket *>(av_mallocz(sizeof(AVPacket)));

    while (av_read_frame(pCtx->formatCtx, flush_pkt) >= 0) {
        if (flush_pkt->stream_index == audio_stream_index) {
            //解码
            *//*ret = avcodec_send_packet(avCodecCtx, flush_pkt);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
//                LOGE("video avcodec_send_packet error %d", ret);
                continue;
            } else if (ret == AVERROR_EOF) {
                //读取完毕 但是不一定播放完毕
                while (1) {
                    if (ctx->audio_down && ctx->video_down) {
                        break;
                    }
                    // LOGE("等待播放完成");
                    usleep(10000);
                }
            } else {
            }*//*
            push(this,flush_pkt);
            av_packet_unref(flush_pkt);
            usleep(10000);
        }
    }*/
}

void FFMpegAudio::release() {

}

void FFMpegAudio::reset() {

}

void FFMpegAudio::destroy() {
    queue->free();
    delete queue;
    queue = NULL;
}

void FFMpegAudio::createSlEs(NativePlayerContext *ctx) {
    result = slCreateEngine(&engineIface, 0, NULL, 0, NULL, NULL);
    if (invalidResult()) {
        return;
    }
    //实现引擎对象，是否异步
    const SLObjectItf_ *pEngineIface = *engineIface;
    result = pEngineIface->Realize(engineIface, SL_BOOLEAN_FALSE);
    if (invalidResult()) {
        return;
    }
    //通过接口对象创建具体的对象 ---> SL_IID_ENGINE 标识为创建那种对象
    result = pEngineIface->GetInterface(engineIface, SL_IID_ENGINE, &engineObj);
    if (invalidResult()) {
        return;
    }
    result = (*engineObj)->CreateOutputMix(engineObj, &outputMixIface, 0, NULL, NULL);
    if (invalidResult()) {
        return;
    }
    result = (*outputMixIface)->Realize(outputMixIface, SL_BOOLEAN_FALSE);
    if (invalidResult()) {
        return;
    }
    result = (*outputMixIface)->GetInterface(outputMixIface, SL_IID_ENVIRONMENTALREVERB,
                                             &outputMixObj);
    if (invalidResult()) {
        return;
    }
    //设置混音器------------begin
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    if (SL_RESULT_SUCCESS == result) {
        (*outputMixObj)->SetEnvironmentalReverbProperties(outputMixObj, &settings);
    }
    SLDataLocator_AndroidBufferQueue androidBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                           2};
    //包含音频的采样率等
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
    //新建数据源，将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&androidBufferQueue, &pcm};
    //导入前面的设置到混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixIface};
    //设置混音器------------END

    SLDataSink audioSink = {&outputMix,
                            NULL};
    //SL_IID_MUTESOLO 声道切换
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,/*SL_IID_MUTESOLO,*/
                                  SL_IID_VOLUME};

    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,/*SL_BOOLEAN_TRUE,*/
                              SL_BOOLEAN_TRUE};
    //开始创建播放器
    (*engineObj)->CreateAudioPlayer(engineObj, &audioPlayerIface, &slDataSource, &audioSink, 2, ids,
                                    req);
    //初始化播放器
    const SLObjectItf_ *pAudioPlayerIface = *audioPlayerIface;
    //得到接口收调用 获取player接口
    pAudioPlayerIface->Realize(audioPlayerIface, SL_BOOLEAN_FALSE);
    //获取缓存队列接口
    pAudioPlayerIface->GetInterface(audioPlayerIface, SL_IID_PLAY, &playerObj);
    //缓冲接口回调
    pAudioPlayerIface->GetInterface(audioPlayerIface, SL_IID_BUFFERQUEUE, &playerBufferQueue);
    //注册回调缓冲队列 ,播放队列为空的时候调用
    (*playerBufferQueue)->RegisterCallback(playerBufferQueue, playerBQCallback, this);
    //获取音量接口
    pAudioPlayerIface->GetInterface(audioPlayerIface, SL_IID_VOLUME, &playerVolume);
    //获取播放状态接口
    (*playerObj)->SetPlayState(playerObj, SL_PLAYSTATE_PLAYING);
    //启用回调函数
//    playerBQCallback(playerBufferQueue, this);
    (*playerBufferQueue)->Enqueue(playerBufferQueue, this, 1);
}

void FFMpegAudio::createFF(NativePlayerContext *ctx) {
    //ff重采样设置
    this->pCtx = ctx;
    swrContext = swr_alloc();
    out_buffer = static_cast<uint8_t *>(av_mallocz(DEFAULT_SAMPLING_RATE * 2));
}




