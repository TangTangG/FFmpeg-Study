//
// Created by tangcaigao on 2019/7/18.
//

#include "FFMpegAudio.h"

#define DEFAULT_SAMPLING_RATE 44100

int decodePCM(FFMpegAudio *audio);

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
        LOGE(result + "");
    }
    return b;
}

//音频缓存回调函数
void playerBQCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    //得到pcm数据
    LOGE("回调pcm数据")
    FFMpegAudio *audio = (FFMpegAudio *) context;
    int data_size = decodePCM(audio);
    if (data_size > 0) {
        //第一针所需要时间 = 采样字节/采样率
        double time = data_size / (DEFAULT_SAMPLING_RATE * 2 * 2);
        //
        audio->audio_time = time + audio->audio_time;
        LOGE("当前一帧声音时间%f   播放时间%f", time, audio->audio_time);

        (*bq)->Enqueue(bq, audio->out_buffer, data_size);
        LOGE("播放 %d ", audio->queue.size());
    }
}

int decodePCM(FFMpegAudio *audio) {
    std::vector<AVPacket *> &packet = audio->queue;
    AVPacket *avPacket = static_cast<AVPacket *>(av_mallocz(sizeof(AVPacket)));
    AVFrame *avFrame = av_frame_alloc();
    int data_size;
    int result;
    for(;;){

    }
    return 0;
}

void FFMpegAudio::create(NativePlayerContext *ctx) {
    createSlEs(ctx);
    createFF(ctx);
}

jlong FFMpegAudio::decode(NativePlayerContext *ctx, const char *url) {
    //---------找到对应解码器
    AVFormatContext *formatCtx = ctx->formatCtx;
    audio_stream_index = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (audio_stream_index == -1) {
        ff_notify_msg(1, "can not find audio stream.");
        return 0L;
    }
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

    int out_sample_rate = avCodecCtx->sample_rate;
    swr_alloc_set_opts(swrContext, out_ch_layout, out_format, out_sample_rate,
                       avCodecCtx->channel_layout, avCodecCtx->sample_fmt, out_sample_rate, 0,
                       NULL);

    swr_init(swrContext);

    out_channer_num = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);

    return out_channer_num;
}

void FFMpegAudio::render(NativePlayerContext *ctx, jlong video_time) {
    //开始从队列获取 packet

}

void FFMpegAudio::release(NativePlayerContext *ctx) {

}

void FFMpegAudio::reset(NativePlayerContext *ctx) {

}

void FFMpegAudio::destroy(NativePlayerContext *ctx) {

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
    result = pEngineIface->GetInterface(engineIface, SL_IID_ENGINE, &engineIface);
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
    //注册回调缓冲队列
    (*playerBufferQueue)->RegisterCallback(playerBufferQueue, playerBQCallback, this);
    //获取音量接口
    pAudioPlayerIface->GetInterface(audioPlayerIface, SL_IID_VOLUME, &playerVolume);
    //获取播放状态接口
    (*playerObj)->SetPlayState(playerObj, SL_PLAYSTATE_PLAYING);
    //启用回调函数
    playerBQCallback(playerBufferQueue, this);
}

void FFMpegAudio::createFF(NativePlayerContext *ctx) {
    //ff重采样设置
    swrContext = swr_alloc();
    out_buffer = static_cast<uint8_t *>(av_mallocz(DEFAULT_SAMPLING_RATE * 2));
}


