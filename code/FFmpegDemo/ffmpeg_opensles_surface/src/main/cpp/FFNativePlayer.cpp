//
// Created by tangcaigao on 2019/7/19.
//

#include "FFNativePlayer.h"
#include "util/FFThreadPool.h"


#define FF_THREAD_POOL_CORE 8
#define FF_THREAD_POOL_QUEUE 64

//error define ----begin
char errorBuf[] = {0};
int errorState;
//error define ----end

AVPacket *flush_pkt;
static bool ff_debug = true;
static bool ff_inited = false;
FFMpegVideo *video;
FFMpegAudio *audio;
AVFormatContext *formatCtx;
//video decoder
AVCodec *avCodec;
AVCodecContext *avCodecCtx;
int video_stream_index;
//surface
SwsContext *swsContext;
ANativeWindow *pNativeWindow;


AVFrame *avFrame;
AVFrame *rgb_frame;

FFThreadPoolContext *threadPoolCtx;

void FFNativePlayer::ff_notify_msg(int tag, const char *msg) {

}

void FFNativePlayer::ff_register() {
    //we can omit this function call in ffmpeg 4.0 and later.
    //av_register_all();
    //avcodec_register_all();
    avformat_network_init();

//    av_log_set_callback(ffmpeg_log);
}

void FFNativePlayer::ff_uninit() {
    avformat_network_deinit();
}

void FFNativePlayer::ff_prepare() {

    formatCtx = avformat_alloc_context();
    flush_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(flush_pkt);
    avFrame = av_frame_alloc();
    rgb_frame = av_frame_alloc();
    //分配线程池
    threadPoolCtx = ff_threadpool_create(FF_THREAD_POOL_CORE,FF_THREAD_POOL_QUEUE,0);
    //分配解码音频和视频的queue
}

void FFNativePlayer::ff_destroy() {
    ff_uninit();
    delete video;
    video = NULL;
    delete audio;
    audio = NULL;
    delete &flush_pkt;
    flush_pkt = NULL;
}

int FFNativePlayer::ff_state() {
    return 0;
}

static void ff_do_render(void* in,void* out){
    int height = avCodecCtx->height;


}

void FFNativePlayer::ff_init(JNIEnv *env) {
    if (ff_inited) {
        return;
    }
    video = (FFMpegVideo *) av_malloc(sizeof(FFMpegVideo));
    audio = (FFMpegAudio *) av_malloc(sizeof(FFMpegAudio));
    ff_register();
    ff_inited = true;
}

void FFNativePlayer::ff_start() {
    if (!playerCheck()) {
        return;
    }
    errorState = avcodec_open2(avCodecCtx, avCodec, NULL);
    if (errorState < 0) {
        LOGE("FFMPEG Player Error: Can not open video file");
    }

    //分配缓存区域，取帧 渲染
    int width = avCodecCtx->width;
    int height = avCodecCtx->height;
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
    //建立缓存区
    uint8_t *out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    //填充rgb_frame
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, out_buffer,
                         AV_PIX_FMT_RGBA,
                         width, height, 1);

    swsContext = sws_getContext(width, height,
                                avCodecCtx->pix_fmt,
                                width, height,
                                AV_PIX_FMT_RGBA,
                                SWS_BICUBIC, NULL, NULL, NULL);
    if (ANativeWindow_setBuffersGeometry(pNativeWindow, width, height, WINDOW_FORMAT_RGBA_8888) <
        0) {
        LOGE("FFMPEG Player Error: Couldn't set buffers geometry.");
        return;
    }
//    ff_threadpool_add(threadPoolCtx,ff_do_render,NULL,NULL);

    int ret;
    ANativeWindow_Buffer nativeWindow_buffer;
    //开始取帧 渲染流程
    while (av_read_frame(formatCtx, flush_pkt) >= 0) {
        if (flush_pkt->stream_index == video_stream_index) {
            //解码
            ret = avcodec_send_packet(avCodecCtx, flush_pkt);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                LOGE("video avcodec_send_packet error %d", ret);
                return;
            }

            ret = avcodec_receive_frame(avCodecCtx, avFrame);
            if (ret < 0 && ret != AVERROR_EOF) {
                LOGE("video avcodec_receive_frame error %d", ret);
                av_packet_unref(flush_pkt);
                continue;
            }

            sws_scale(swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize, 0,
                      avCodecCtx->height, rgb_frame->data, rgb_frame->linesize);
            if (ANativeWindow_lock(pNativeWindow, &nativeWindow_buffer, NULL) < 0) {
                LOGD("FFMPEG Player: can not lock window. ");
                usleep(16000);
            } else {
                uint8_t *dst = (uint8_t *) nativeWindow_buffer.bits;
                //像素数据的首地址
                uint8_t *src = rgb_frame->data[0];
                //一行包含多少RGBA ---> 多少个像素
                int destStride = nativeWindow_buffer.stride * 4;
                //实际一行包含的像素点内存量
                int srcStride = rgb_frame->linesize[0];
                for (int i = 0; i < height; ++i) {
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(pNativeWindow);
            }
        }
        av_packet_unref(flush_pkt);
    }
    ff_release();
}

/**
 * 暂停
 * 返回暂停时的播放位置
 */
jlong FFNativePlayer::ff_pause() {
    return 0;
}

/**
 * 停止
 * 返回停止时的播放位置
 */
jlong FFNativePlayer::ff_stop() {
    return 0;
}

/**
 * 拖动到指定位置
 * 返回拖动完成后的播放位置
 */
jlong FFNativePlayer::ff_seek_to(jlong targetPos) {
    return 0;
}

void FFNativePlayer::ff_rest(JNIEnv *pEnv) {

}

/**
 * 准备数据流 -------- 解码
 */
jlong FFNativePlayer::ff_set_data_source(JNIEnv *pEnv, const char *url) {
    errorState = avformat_open_input(&formatCtx, url, NULL, NULL);
    if (errorState < 0) {
        av_strerror(errorState, errorBuf, 1024);
        LOGE("Couldn't open file %s: %d(%s)", url, errorState, errorBuf);
        ff_notify_msg(errorState, "FFMPEG Player Error: Can not open video file");
        return 0L;
    }

    //查看文件视频流信息
    errorState = avformat_find_stream_info(formatCtx, NULL);
    if (errorState < 0) {
        ff_notify_msg(errorState, "FFMPEG Player Error: Can not find video file stream info");
        return 0L;
    }
    //查找视频流对应解码器
    video_stream_index = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL,
                                                 0);
    /*for (int i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }*/

    if (video_stream_index == -1) {
        LOGD("FFMPEG Player : Can not find video codec");
        return 0L;
    }

    //初始化视频流编码器上下文
    AVCodecParameters *codecpar = formatCtx->streams[video_stream_index]->codecpar;
    avCodec = avcodec_find_decoder(codecpar->codec_id);
    avCodecCtx = avcodec_alloc_context3(avCodec);
    if (ff_debug) {
        LOGD("---------- dumping stream info begin ----------");

        LOGD("input format: %s", formatCtx->iformat->name);
        LOGD("nb_streams: %d", formatCtx->nb_streams);

        int64_t start_time = formatCtx->start_time / AV_TIME_BASE;
        LOGD("start_time: %lld", start_time);

        int64_t duration = formatCtx->duration / AV_TIME_BASE;
        LOGD("duration: %lld s", duration);

        int video_stream_idx = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL,
                                                   0);
        if (video_stream_idx >= 0) {
            AVStream *video_stream = formatCtx->streams[video_stream_idx];
            LOGD("video nb_frames: %lld", video_stream->nb_frames);
            LOGD("video codec_id: %d", codecpar->codec_id);
            LOGD("video codec_name: %s", avcodec_get_name(codecpar->codec_id));
            LOGD("video width x height: %d x %d", codecpar->width, codecpar->height);
            LOGD("video pix_fmt: %d", avCodecCtx->pix_fmt);
            LOGD("video bitrate %lld kb/s", (int64_t) codecpar->bit_rate / 1000);
            LOGD("video avg_frame_rate: %d fps",
                 video_stream->avg_frame_rate.num / video_stream->avg_frame_rate.den);
        }

        /* int audio_stream_idx = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
         if (audio_stream_idx >= 0) {
             AVStream *audio_stream = formatCtx->streams[audio_stream_idx];
             LOGD("audio codec_id: %d", codecpar->codec_id);
             LOGD("audio codec_name: %s", avcodec_get_name(codecpar->codec_id));
             LOGD("audio sample_rate: %d", codecpar->sample_rate);
             LOGD("audio channels: %d", codecpar->channels);
             LOGD("audio sample_fmt: %d", avCodec->sample_fmt);
             LOGD("audio frame_size: %d", codecpar->frame_size);
             LOGD("audio nb_frames: %lld", audio_stream->nb_frames);
             LOGD("audio bitrate %lld kb/s", (int64_t) codecpar->bit_rate / 1000);
         }*/

        LOGD("---------- dumping stream info end ----------");
    }
    avcodec_parameters_to_context(avCodecCtx, codecpar);
    return formatCtx->duration / AV_TIME_BASE;
}

void FFNativePlayer::ff_attach_window(JNIEnv *pEnv, jobject surface) {
    //获取surface所在window

    pNativeWindow = ANativeWindow_fromSurface(pEnv, surface);
    if (pNativeWindow == 0) {
        LOGE("FFMPEG Player Error: ANativeWindow get failed");
        return;
    }
}

jlong FFNativePlayer::ff_get_current_pos(JNIEnv *pEnv) {
    return 0;
}

bool FFNativePlayer::playerCheck() {
    if (!ff_inited) {
        return false;
    }
    if (avCodecCtx == 0) {
        ff_notify_msg(1, "initial player first.");
        return false;
    }
    if (pNativeWindow == 0){
        ff_notify_msg(1, "attach view.");
        return false;
    }
    return true;
}

void FFNativePlayer::ff_release() {
    ANativeWindow_release(pNativeWindow);
    sws_freeContext(swsContext);
    av_frame_free(&avFrame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecCtx);
    avformat_close_input(&formatCtx);

}

