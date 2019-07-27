//
// Created by tangcaigao on 2019/7/18.
//
#include "util/FFThreadPool.h"
#include "FFMpegVideo.h"


//error define ----begin
char errorBuf[] = {0};
int errorState;
//error define ----end


static int audio_queue_callback(AVPacket *dst, AVPacket *src) {
    if (src == NULL) {
        av_free(dst);
        return 0;
    }
    return av_packet_ref(dst, src);
}

void pop(FFMpegVideo *pVideo, AVPacket *pPacket) {
    LOGD("tang获取一个视频包");
    pVideo->queue->pop(pPacket, audio_queue_callback);
}

void push(FFMpegVideo *pVideo, AVPacket *pPacket) {
    AVPacket *avPacket = static_cast<AVPacket *>(av_mallocz(sizeof(AVPacket)));
    pVideo->queue->push(pPacket, avPacket, audio_queue_callback);
}

static void start_render_notify(void *pVideo, void *out) {
    FFMpegVideo *video = static_cast<FFMpegVideo *>(pVideo);
    AVPacket *avPacket = video->render_pkt;
    int result;
    int height;
    ANativeWindow_Buffer nativeWindow_buffer;
    AVFrame *avFrame = video->avFrame;
    while (video->ctx->play_state > 0) {
        //可能阻塞
        pop(video,avPacket);
        if (avPacket->pts != AV_NOPTS_VALUE) {
            //qts --> double 校正时间
            video->ctx->video_time = av_q2d(video->time_base) * avPacket->pts;
        }
        //解码
        result = avcodec_send_packet(video->avCodecCtx, avPacket);
        if (result != 0) {
            if (result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                LOGE("audio avcodec_send_packet error %d", result);
            }
            continue;
        }

        result = avcodec_receive_frame(video->avCodecCtx, avFrame);
        if (result != 0) {
            if (result != AVERROR_EOF) {
                LOGE("audio avcodec_receive_frame error %d", result);
            }
            continue;
        }
        height = video->avCodecCtx->height;
        sws_scale(video->swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize, 0,
                  height, video->rgb_frame->data, video->rgb_frame->linesize);
        if (ANativeWindow_lock(video->pNativeWindow, &nativeWindow_buffer, NULL) < 0) {
            LOGD("FFMPEG Player: can not lock window. ");
            usleep(16000);
        } else {
            uint8_t *dst = (uint8_t *) nativeWindow_buffer.bits;
            //像素数据的首地址
            uint8_t *src = video->rgb_frame->data[0];
            //一行包含多少RGBA ---> 多少个像素
            int destStride = nativeWindow_buffer.stride * 4;
            //实际一行包含的像素点内存量
            int srcStride = video->rgb_frame->linesize[0];
            for (int i = 0; i < height; ++i) {
                memcpy(dst + i * destStride, src + i * srcStride, srcStride);
            }
            ANativeWindow_unlockAndPost(video->pNativeWindow);
        }
    }
    av_free(avPacket);
    av_frame_free(&avFrame);
}


void FFMpegVideo::create(NativePlayerContext *ctx) {
    avFrame = av_frame_alloc();
    rgb_frame = av_frame_alloc();
    this->ctx = ctx;
    queue = new FFLockedQueue<AVPacket>();
    queue->init();
    flush_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
    render_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(flush_pkt);
}

jlong FFMpegVideo::decode(NativePlayerContext *ctx,const char *url) {
    AVFormatContext *formatCtx = ctx->formatCtx;

    //查找视频流对应解码器
    video_stream_index = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL,
                                             0);

    if (video_stream_index == -1) {
        LOGD("FFMPEG Player : Can not find video stream");
        return 0L;
    }

    //初始化视频流编码器上下文
    AVCodecParameters *codecpar = formatCtx->streams[video_stream_index]->codecpar;
    avCodec = avcodec_find_decoder(codecpar->codec_id);
    avCodecCtx = avcodec_alloc_context3(avCodec);
    time_base = ctx->formatCtx->streams[video_stream_index]->time_base;

    if (ctx->debug) {
        LOGD("---------- dumping video stream info begin ----------");

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
        LOGD("---------- dumping video stream info end ----------");
    }
    avcodec_parameters_to_context(avCodecCtx, codecpar);
    ff_threadpool_add(ctx->threadPoolCtx, start_render_notify, this,NULL);
    return formatCtx->duration / AV_TIME_BASE;
}

void FFMpegVideo::render(NativePlayerContext *ctx,jlong audio_time) {
    if (pNativeWindow == 0) {
        pNativeWindow = static_cast<ANativeWindow *>(ctx->display);
    }
    if (pNativeWindow == 0) {
        LOGE("FFMPEG Player Error: ANativeWindow get failed");
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
        queue->stop();
        LOGE("FFMPEG Player Error: Couldn't set buffers geometry.");
        return;
    }

    int ret;
    //开始取帧 渲染流程
    while (av_read_frame(ctx->formatCtx, flush_pkt) >= 0) {
        if (flush_pkt->stream_index == video_stream_index) {
            //解码
            ret = avcodec_send_packet(avCodecCtx, flush_pkt);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                LOGE("video avcodec_send_packet error %d", ret);
                continue;
            } else if (ret == AVERROR_EOF) {
                //读取完毕 但是不一定播放完毕
                while (ctx->play_state > 0) {
                    if (ctx->audio_down && ctx->video_down) {
                        break;
                    }
                    // LOGE("等待播放完成");
                    usleep(10000);
                }
            } else {
//                LOGD("tang push 视频包");
                push(this, flush_pkt);
                av_packet_unref(flush_pkt);
            }
        }
    }
    release();
}

void FFMpegVideo::release() {
    ANativeWindow_release(pNativeWindow);
    sws_freeContext(swsContext);
    av_frame_free(&avFrame);
    av_frame_free(&rgb_frame);
    avcodec_close(avCodecCtx);
    avformat_close_input(&ctx->formatCtx);
}

void FFMpegVideo::reset() {

}

void FFMpegVideo::destroy() {
    queue->free();
    delete queue;
    queue = NULL;
}


